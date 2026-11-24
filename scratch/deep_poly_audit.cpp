#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <memory>
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/DSP/FactoryPresetBank.h"

struct AudioAnalysis
{
    float peak = 0.0f;
    float maxDiff = 0.0f;
    int nanCount = 0;
    int infCount = 0;
    int clipCount = 0; // samples >= 0.999
    int discontinuityCount = 0; // step > 0.4
    double rmsSum = 0.0;
    int totalSamples = 0;

    void update(float sample)
    {
        totalSamples++;
        if (std::isnan(sample)) { nanCount++; return; }
        if (std::isinf(sample)) { infCount++; return; }
        const float absS = std::abs(sample);
        peak = std::max(peak, absS);
        if (absS >= 0.999f) clipCount++;
        rmsSum += sample * sample;
    }

    void checkDiff(float prev, float curr, int sampleIndex)
    {
        const float diff = std::abs(curr - prev);
        maxDiff = std::max(maxDiff, diff);
        if (diff > 0.30f)
        {
            discontinuityCount++;
            if (discontinuityCount <= 5)
            {
                std::cout << "  [Discontinuity at sample " << sampleIndex 
                          << "] prev=" << prev << ", curr=" << curr 
                          << ", step=" << diff << std::endl;
            }
        }
    }

    float getRMS() const
    {
        return totalSamples > 0 ? std::sqrt(static_cast<float>(rmsSum / totalSamples)) : 0.0f;
    }
};

void runAuditOnPreset(PrismorAudioProcessor& proc, int presetIdx, int overrideVoicingMode = -1)
{
    proc.loadFactoryPreset(presetIdx);

    if (overrideVoicingMode >= 0)
    {
        if (auto* param = proc.getAPVTS().getParameter("voicingMode"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(overrideVoicingMode)));
    }

    const double sampleRate = 44100.0;
    const int blockSize = 128;
    proc.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    AudioAnalysis analysis;
    float prevL = 0.0f, prevR = 0.0f;

    auto processBlocks = [&](int numBlocks, juce::MidiBuffer& m)
    {
        for (int b = 0; b < numBlocks; ++b)
        {
            buffer.clear();
            proc.processBlock(buffer, m);
            m.clear();

            const float* l = buffer.getReadPointer(0);
            const float* r = buffer.getReadPointer(1);
            for (int s = 0; s < blockSize; ++s)
            {
                const int globalSample = b * blockSize + s;
                analysis.update(l[s]);
                analysis.update(r[s]);
                analysis.checkDiff(prevL, l[s], globalSample);
                analysis.checkDiff(prevR, r[s], globalSample);
                prevL = l[s];
                prevR = r[s];
            }
        }
    };

    // Scenario: Note 60 struck at t=0
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.85f), 0);
    processBlocks(10, midi); // ~29ms

    // Overlapping note 64 struck at t=29ms while 60 is still held
    midi.addEvent(juce::MidiMessage::noteOn(1, 64, 0.85f), 0);
    processBlocks(15, midi); // ~43ms overlapping

    // Overlapping note 67 struck while both are held
    midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.85f), 0);
    processBlocks(15, midi); // ~43ms 3 notes overlapping

    // Release note 60, keep 64 and 67 held (tail overlap)
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    processBlocks(15, midi);

    // Release 64
    midi.addEvent(juce::MidiMessage::noteOff(1, 64), 0);
    processBlocks(15, midi);

    // Release 67
    midi.addEvent(juce::MidiMessage::noteOff(1, 67), 0);
    processBlocks(30, midi); // decay tails

    std::cout << "Preset [" << std::setw(2) << presetIdx << "] "
              << "(Voicing: " << (overrideVoicingMode == 0 ? "Poly" : (overrideVoicingMode == 2 ? "Legato" : "Default")) << ")"
              << " | Peak: " << std::fixed << std::setprecision(4) << analysis.peak
              << " | RMS: " << analysis.getRMS()
              << " | MaxStep: " << analysis.maxDiff
              << " | Discontinuities: " << analysis.discontinuityCount
              << " | Clips: " << analysis.clipCount
              << " | NaN: " << analysis.nanCount << std::endl;
}

void runChordAudit(PrismorAudioProcessor& proc, int presetIdx, int numVoices)
{
    proc.loadFactoryPreset(presetIdx);

    if (auto* param = proc.getAPVTS().getParameter("voicingMode"))
        param->setValueNotifyingHost(param->convertTo0to1(0.0f)); // Polyphonic
    if (auto* param = proc.getAPVTS().getParameter("maxVoices"))
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(numVoices)));

    const double sampleRate = 44100.0;
    const int blockSize = 128;
    proc.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    AudioAnalysis analysis;
    float prevL = 0.0f, prevR = 0.0f;

    auto processBlocks = [&](int numBlocks, juce::MidiBuffer& m)
    {
        for (int b = 0; b < numBlocks; ++b)
        {
            buffer.clear();
            proc.processBlock(buffer, m);
            m.clear();

            const float* l = buffer.getReadPointer(0);
            const float* r = buffer.getReadPointer(1);
            for (int s = 0; s < blockSize; ++s)
            {
                const int globalSample = b * blockSize + s;
                analysis.update(l[s]);
                analysis.update(r[s]);
                analysis.checkDiff(prevL, l[s], globalSample);
                analysis.checkDiff(prevR, r[s], globalSample);
                prevL = l[s];
                prevR = r[s];
            }
        }
    };

    // Strum 8 notes into buffer
    int notes[] = { 48, 52, 55, 59, 60, 64, 67, 71 };
    for (int i = 0; i < std::min(8, numVoices); ++i)
        midi.addEvent(juce::MidiMessage::noteOn(1, notes[i], 0.90f), i * 10);
    processBlocks(40, midi); // sustain chord for ~116ms

    // Release first 4 notes, keep 4 notes sustained
    for (int i = 0; i < 4; ++i)
        midi.addEvent(juce::MidiMessage::noteOff(1, notes[i]), 0);
    processBlocks(30, midi); // release tails decaying while 4 held

    // Now re-strike new notes over the decaying tails
    int newNotes[] = { 62, 65, 69, 72 };
    for (int i = 0; i < 4; ++i)
        midi.addEvent(juce::MidiMessage::noteOn(1, newNotes[i], 0.90f), i * 10);
    processBlocks(40, midi);

    // Release all
    for (int i = 4; i < std::min(8, numVoices); ++i)
        midi.addEvent(juce::MidiMessage::noteOff(1, notes[i]), 0);
    for (int i = 0; i < 4; ++i)
        midi.addEvent(juce::MidiMessage::noteOff(1, newNotes[i]), 0);
    processBlocks(50, midi);

    if (analysis.discontinuityCount > 0 || analysis.clipCount > 0 || analysis.nanCount > 0)
    {
        std::cout << ">>> ANOMALY in Preset [" << std::setw(3) << presetIdx << "] "
                  << "Peak: " << analysis.peak
                  << " | RMS: " << analysis.getRMS()
                  << " | MaxStep: " << analysis.maxDiff
                  << " | Discont: " << analysis.discontinuityCount
                  << " | Clips: " << analysis.clipCount
                  << " | NaN: " << analysis.nanCount << std::endl;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    std::cout << "=== PRISMOR DETAILED DISCONTINUITY AUDIT ===" << std::endl;

    auto procPtr = std::make_unique<PrismorAudioProcessor>();
    auto& processor = *procPtr;

    int testPresets[] = { 17, 33, 39, 147, 162 };
    for (int p : testPresets)
    {
        std::cout << "\n>>> TESTING PRESET " << p << " <<<" << std::endl;
        runChordAudit(processor, p, 8);
    }

    return 0;
}
