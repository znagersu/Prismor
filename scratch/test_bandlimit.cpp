#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <memory>
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/FactoryPresetBank.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    auto procPtr = std::make_unique<PrismorAudioProcessor>();
    auto& processor = *procPtr;

    processor.loadFactoryPreset(39); // Preset 39!

    if (auto* param = processor.getAPVTS().getParameter("voicingMode"))
        param->setValueNotifyingHost(param->convertTo0to1(0.0f)); // Polyphonic
    if (auto* param = processor.getAPVTS().getParameter("maxVoices"))
        param->setValueNotifyingHost(param->convertTo0to1(8.0f));

    const double sampleRate = 44100.0;
    const int blockSize = 128;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    // Play an 8-note chord on Preset 39 (Notes 48, 52, 55, 59, 60, 64, 67, 72)
    int notes[] = { 48, 52, 55, 59, 60, 64, 67, 72 };
    for (int i = 0; i < 8; ++i)
        midi.addEvent(juce::MidiMessage::noteOn(1, notes[i], 0.90f), i * 5);

    float maxDiff = 0.0f;
    float peak = 0.0f;
    float prevL = 0.0f;
    int disconts = 0;

    for (int b = 0; b < 30; ++b)
    {
        buffer.clear();
        processor.processBlock(buffer, midi);
        midi.clear();

        const float* l = buffer.getReadPointer(0);
        for (int s = 0; s < blockSize; ++s)
        {
            peak = std::max(peak, std::abs(l[s]));
            float diff = std::abs(l[s] - prevL);
            maxDiff = std::max(maxDiff, diff);
            if (diff > 0.30f) disconts++;
            prevL = l[s];
        }
    }

    std::cout << "Preset 39 8-Note Chord -> Peak: " << peak 
              << " | MaxStep: " << maxDiff 
              << " | Discontinuities: " << disconts << std::endl;

    return 0;
}
