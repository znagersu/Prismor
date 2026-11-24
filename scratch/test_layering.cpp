#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <memory>
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/FactoryPresetBank.h"

void testPolyLayering(int presetIdx)
{
    auto procPtr = std::make_unique<PrismorAudioProcessor>();
    auto& processor = *procPtr;

    processor.loadFactoryPreset(presetIdx);

    // Force polyphonic voicing mode with 8 voices
    if (auto* param = processor.getAPVTS().getParameter("voicingMode"))
        param->setValueNotifyingHost(param->convertTo0to1(0.0f));
    if (auto* param = processor.getAPVTS().getParameter("maxVoices"))
        param->setValueNotifyingHost(param->convertTo0to1(8.0f));

    const double sampleRate = 44100.0;
    const int blockSize = 128;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    float peak = 0.0f;
    double sumSq = 0.0;
    int totalSamples = 0;
    int nanCount = 0;
    int infCount = 0;
    int clipCount = 0;

    auto process = [&](int numBlocks, juce::MidiBuffer& m)
    {
        for (int b = 0; b < numBlocks; ++b)
        {
            buffer.clear();
            processor.processBlock(buffer, m);
            m.clear();

            const float* l = buffer.getReadPointer(0);
            const float* r = buffer.getReadPointer(1);
            for (int s = 0; s < blockSize; ++s)
            {
                totalSamples += 2;
                if (std::isnan(l[s]) || std::isnan(r[s])) { nanCount++; continue; }
                if (std::isinf(l[s]) || std::isinf(r[s])) { infCount++; continue; }
                const float aL = std::abs(l[s]);
                const float aR = std::abs(r[s]);
                peak = std::max({peak, aL, aR});
                if (aL >= 0.999f || aR >= 0.999f) clipCount++;
                sumSq += l[s] * l[s] + r[s] * r[s];
            }
        }
    };

    // Staggered dense polyphonic chord: C3, E3, G3, B3, C4, D4, E4, G4
    int chord[] = { 48, 52, 55, 59, 60, 62, 64, 67 };
    for (int i = 0; i < 8; ++i)
    {
        midi.addEvent(juce::MidiMessage::noteOn(1, chord[i], 0.90f), i * 8);
    }
    process(20, midi); // ~58ms full 8-note chord sounding

    // Rapid overlapping release tails: release notes 48, 52, 55 while striking 69, 71, 72
    midi.addEvent(juce::MidiMessage::noteOff(1, 48), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 52), 5);
    midi.addEvent(juce::MidiMessage::noteOff(1, 55), 10);
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, 0.95f), 15);
    midi.addEvent(juce::MidiMessage::noteOn(1, 71, 0.95f), 20);
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, 0.95f), 25);
    process(30, midi); // dense tail overlap & voice stealing

    // Release all notes and process decay tail
    for (int n : chord)
        midi.addEvent(juce::MidiMessage::noteOff(1, n), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 69), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 71), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 72), 0);
    process(40, midi); // decay tails to zero

    const float rms = totalSamples > 0 ? std::sqrt(static_cast<float>(sumSq / totalSamples)) : 0.0f;

    std::cout << "Preset [" << std::setw(2) << presetIdx << "] "
              << "Peak: " << std::fixed << std::setprecision(4) << peak
              << " | RMS: " << rms
              << " | Clips: " << clipCount
              << " | NaN/Inf: " << (nanCount + infCount)
              << " -> " << ((clipCount == 0 && nanCount == 0 && infCount == 0 && peak < 0.99f) ? "PASS (CLEAN)" : "FAIL")
              << std::endl;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    std::cout << "==========================================================" << std::endl;
    std::cout << "  POLYPHONIC SUMMATION & STAGGERED OVERLAP DRESS REHEARSAL " << std::endl;
    std::cout << "==========================================================" << std::endl;

    int testPresets[] = { 39, 17, 33, 58, 59, 91, 1, 10, 25, 45, 60, 75, 100, 120 };
    for (int p : testPresets)
    {
        testPolyLayering(p);
    }

    return 0;
}
