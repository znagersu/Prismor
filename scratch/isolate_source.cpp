#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <memory>
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/FactoryPresetBank.h"

int testWithConfig(int presetIndex, const char* name, std::function<void(PrismorAudioProcessor&)> tweak)
{
    auto procPtr = std::make_unique<PrismorAudioProcessor>();
    auto& processor = *procPtr;

    processor.loadFactoryPreset(presetIndex);
    if (auto* param = processor.getAPVTS().getParameter("voicingMode"))
        param->setValueNotifyingHost(param->convertTo0to1(0.0f)); // Polyphonic
    if (auto* param = processor.getAPVTS().getParameter("maxVoices"))
        param->setValueNotifyingHost(param->convertTo0to1(8.0f));

    if (tweak) tweak(processor);

    const double sampleRate = 44100.0;
    const int blockSize = 128;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    int notes[] = { 48, 52, 55, 59, 60, 64, 67, 71 };
    for (int i = 0; i < 8; ++i)
        midi.addEvent(juce::MidiMessage::noteOn(1, notes[i], 0.90f), i * 10);

    float prevL = 0.0f;
    int discontCount = 0;
    float maxStep = 0.0f;

    for (int b = 0; b < 25; ++b)
    {
        buffer.clear();
        processor.processBlock(buffer, midi);
        midi.clear();

        const float* l = buffer.getReadPointer(0);
        for (int s = 0; s < blockSize; ++s)
        {
            const float diff = std::abs(l[s] - prevL);
            maxStep = std::max(maxStep, diff);
            if (diff > 0.30f) discontCount++;
            prevL = l[s];
        }
    }

    std::cout << "Preset [" << std::setw(2) << presetIndex << "] [" << std::setw(25) << name << "] -> MaxStep: " 
              << std::fixed << std::setprecision(4) << maxStep 
              << " | Discontinuities: " << discontCount << std::endl;
    return discontCount;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    std::cout << "=== TESTING PRESET 39 (PIERCING SYNC SCREAMER) & PRESET 33 ===" << std::endl;

    testWithConfig(39, "Full Preset 39 (8 Notes)", nullptr);
    testWithConfig(33, "Full Preset 33 (8 Notes)", nullptr);
    testWithConfig(17, "Full Preset 17 (8 Notes)", nullptr);
    return 0;
}
