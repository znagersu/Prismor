#include <iostream>
#include <chrono>
#include <memory>
#include <cmath>
#include <cassert>
#include <vector>
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

int main()
{
    std::cout << "=== Prismor Phase 8.1 Bode Frequency Shifter Full Processor Integration Test ===" << std::endl;

    auto processor = std::make_unique<PrismorAudioProcessor>();
    const double sampleRate = 44100.0;
    const int blockSize = 128;
    processor->prepareToPlay(sampleRate, blockSize);

    // Access APVTS parameters and configure Bode
    auto& apvts = processor->getAPVTS();
    if (auto* p = apvts.getParameter("bodeEnable"))
        p->setValueNotifyingHost(1.0f); // Enabled
    if (auto* p = apvts.getParameter("bodeShift"))
        p->setValueNotifyingHost(apvts.getParameterRange("bodeShift").convertTo0to1(250.0f)); // +250 Hz shift
    if (auto* p = apvts.getParameter("bodeFeedback"))
        p->setValueNotifyingHost(apvts.getParameterRange("bodeFeedback").convertTo0to1(0.40f)); // 40% feedback
    if (auto* p = apvts.getParameter("bodeMix"))
        p->setValueNotifyingHost(0.80f); // 80% wet

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    // Trigger polyphonic notes (C4, E4, G4, B4)
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.90f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 64, 0.85f), 10);
    midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.80f), 20);
    midi.addEvent(juce::MidiMessage::noteOn(1, 71, 0.75f), 30);

    const int numBlocks = 400; // ~1.16 seconds
    float maxAbs = 0.0f;
    bool hasNaN = false;
    double totalEnergy = 0.0;

    auto startT = std::chrono::high_resolution_clock::now();

    for (int block = 0; block < numBlocks; ++block)
    {
        buffer.clear();
        processor->processBlock(buffer, midi);
        midi.clear();

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);
        for (int i = 0; i < blockSize; ++i)
        {
            if (std::isnan(l[i]) || std::isinf(l[i]) || std::isnan(r[i]) || std::isinf(r[i]))
            {
                hasNaN = true;
            }
            maxAbs = std::max(maxAbs, std::max(std::abs(l[i]), std::abs(r[i])));
            totalEnergy += l[i] * l[i] + r[i] * r[i];
        }
    }

    auto endT = std::chrono::high_resolution_clock::now();
    double elapsedUs = std::chrono::duration<double, std::micro>(endT - startT).count();
    double audioTimeUs = (static_cast<double>(numBlocks * blockSize) / sampleRate) * 1e6;
    double cpuPct = (elapsedUs / audioTimeUs) * 100.0;

    std::cout << "Rendered " << (numBlocks * blockSize) << " samples (" << (numBlocks * blockSize / sampleRate) << "s) of polyphonic Bode audio:\n";
    std::cout << "  - Peak Sample Amplitude: " << maxAbs << " (bounded, clean)\n";
    std::cout << "  - Total Audio Energy:    " << totalEnergy << "\n";
    std::cout << "  - NaN / Inf Detected:    " << (hasNaN ? "YES (FAIL)" : "NO (PASS)") << "\n";
    std::cout << "  - Processing Time:       " << elapsedUs / 1000.0 << " ms\n";
    std::cout << "  - CPU Budget Used:       " << cpuPct << " %\n";

    assert(!hasNaN);
    assert(totalEnergy > 0.05);
    assert(maxAbs <= 1.0f);

    std::cout << ">>> FULL PROCESSOR INTEGRATION TEST PASSED! <<<\n";
    return 0;
}
