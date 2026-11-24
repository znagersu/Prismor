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

    processor.loadFactoryPreset(33);

    if (auto* param = processor.getAPVTS().getParameter("voicingMode"))
        param->setValueNotifyingHost(param->convertTo0to1(0.0f)); // Polyphonic
    if (auto* param = processor.getAPVTS().getParameter("maxVoices"))
        param->setValueNotifyingHost(param->convertTo0to1(8.0f));

    const double sampleRate = 44100.0;
    const int blockSize = 128;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    // Strum 8 notes
    int notes[] = { 48, 52, 55, 59, 60, 64, 67, 71 };
    for (int i = 0; i < 8; ++i)
        midi.addEvent(juce::MidiMessage::noteOn(1, notes[i], 0.90f), i * 10);

    float prevL = 0.0f;
    int globalSample = 0;

    for (int b = 0; b < 20; ++b)
    {
        buffer.clear();
        processor.processBlock(buffer, midi);
        midi.clear();

        const float* l = buffer.getReadPointer(0);
        for (int s = 0; s < blockSize; ++s)
        {
            const float currL = l[s];
            const float diff = std::abs(currL - prevL);
            if (diff > 0.25f)
            {
                std::cout << "[Sample " << globalSample << " (Block " << b << ", s=" << s << ")] "
                          << "prev=" << prevL << ", curr=" << currL << ", step=" << diff << std::endl;
                // Print surrounding 10 samples
                int start = std::max(0, s - 5);
                int end = std::min(blockSize - 1, s + 5);
                std::cout << "  Window: ";
                for (int w = start; w <= end; ++w)
                {
                    std::cout << l[w] << (w == s ? "* " : " ");
                }
                std::cout << std::endl;
            }
            prevL = currL;
            globalSample++;
        }
    }

    return 0;
}
