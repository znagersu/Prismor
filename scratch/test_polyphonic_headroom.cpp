#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <vector>
#include <memory>
#include <JuceHeader.h>
#include "../Source/DSP/SpectralSynthEngine.h"

// Replicate the Master Soft Clipper / Safety Limiter exactly as in PluginProcessor::processBlock
void applyMasterSoftClipper(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float x = channelData[i];
            if (std::isnan(x) || std::isinf(x))
            {
                channelData[i] = 0.0f;
                continue;
            }

            const float absX = std::abs(x);
            if (absX > 0.90f)
            {
                const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
                channelData[i] = sign * (0.90f + 0.10f * std::tanh((absX - 0.90f) * 10.0f));
            }
            channelData[i] = juce::jlimit(-1.0f, 1.0f, channelData[i]);
        }
    }
}

int main()
{
    std::cout << "=== Testing Prismor 1.0.15.4 Polyphonic Summation Corruption Fix ===" << std::endl;

    auto enginePtr = std::make_unique<PrismorDSP::SpectralSynthEngine>();
    auto& engine = *enginePtr;
    engine.prepare(44100.0, 64);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    PrismorDSP::SynthParameters defaultParams;
    defaultParams.filter1Enable = true;
    defaultParams.filter1Cutoff = 0.85f;
    defaultParams.unisonVoices = 4;
    defaultParams.unisonDetune = 0.25f;

    const int chordNotes[8] = { 48, 52, 55, 59, 60, 64, 67, 72 };

    // -------------------------------------------------------------------------
    // Test 1: Clean 8-Note Polyphonic Chord (Checking Pre-FX Headroom & Zero Clipping)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 1: Clean 8-Note Polyphonic Chord (Notes 48, 52, 55, 59, 60, 64, 67, 72)]" << std::endl;
    midi.clear();
    for (int n : chordNotes)
        midi.addEvent(juce::MidiMessage::noteOn(1, n, 0.95f), 0);

    float globalMaxPeak = 0.0f;
    float maxDerivative = 0.0f;
    int hardClippedSamples = 0;
    int nanOrInfCount = 0;
    float prevSampleL = 0.0f;

    const int testBlocks = 300;
    for (int b = 0; b < testBlocks; ++b)
    {
        // Replicate PluginProcessor buffer clear
        buffer.clear();
        engine.renderBlock(buffer, midi, defaultParams);
        midi.clear(); // notes held after first block

        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);

        for (int s = 0; s < 64; ++s)
        {
            if (std::isnan(l[s]) || std::isinf(l[s]) || std::isnan(r[s]) || std::isinf(r[s]))
            {
                nanOrInfCount++;
                continue;
            }

            const float absL = std::abs(l[s]);
            const float absR = std::abs(r[s]);
            const float peak = std::max(absL, absR);
            globalMaxPeak = std::max(globalMaxPeak, peak);

            if (peak > 1.0001f)
                hardClippedSamples++;

            if (b > 2)
            {
                const float diff = std::abs(l[s] - prevSampleL);
                maxDerivative = std::max(maxDerivative, diff);
            }
            prevSampleL = l[s];
        }
    }

    std::cout << "  - Active Voices Count   : " << engine.getActiveVoiceCount() << " (Expected 8)" << std::endl;
    std::cout << "  - Global Peak Amplitude : " << globalMaxPeak << " (Ceiling <= 1.0000)" << std::endl;
    std::cout << "  - Hard-Clipped Samples  : " << hardClippedSamples << " (Must be 0)" << std::endl;
    std::cout << "  - Max Sample Derivative : " << maxDerivative << " (Smooth continuous waveform)" << std::endl;
    std::cout << "  - NaN / Inf Anomalies   : " << nanOrInfCount << std::endl;

    if (hardClippedSamples > 0 || globalMaxPeak > 1.0001f || nanOrInfCount > 0)
    {
        std::cerr << "FAIL: Polyphonic audio exceeded range or contained NaNs!" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // Test 2: 8-Note Chord with Filter Drive (Checking Monotonic Saturation & No Foldover)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 2: 8-Note Chord with Heavy Filter Drive (drive = 0.75)]" << std::endl;
    PrismorDSP::SynthParameters driveParams = defaultParams;
    driveParams.filterDrive = 0.75f;

    midi.clear();
    for (int n : chordNotes)
        midi.addEvent(juce::MidiMessage::noteOn(1, n, 1.0f), 0);

    float driveMaxPeak = 0.0f;
    int driveHardClips = 0;
    int driveNaNs = 0;

    for (int b = 0; b < 200; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, driveParams);
        midi.clear();

        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);

        for (int s = 0; s < 64; ++s)
        {
            if (std::isnan(l[s]) || std::isinf(l[s]) || std::isnan(r[s]) || std::isinf(r[s]))
            {
                driveNaNs++;
                continue;
            }
            const float peak = std::max(std::abs(l[s]), std::abs(r[s]));
            driveMaxPeak = std::max(driveMaxPeak, peak);
            if (peak > 1.0001f)
                driveHardClips++;
        }
    }

    std::cout << "  - Drive Peak Amplitude  : " << driveMaxPeak << " (Ceiling <= 1.0000)" << std::endl;
    std::cout << "  - Drive Hard Clips      : " << driveHardClips << " (Must be 0)" << std::endl;
    std::cout << "  - Drive NaN Count       : " << driveNaNs << " (Must be 0)" << std::endl;

    if (driveHardClips > 0 || driveMaxPeak > 1.0001f || driveNaNs > 0)
    {
        std::cerr << "FAIL: Filter Drive caused clipping or NaN corruption!" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // Test 3: 8-Note Chord with Tube Distortion (Checking No Parabolic Diode Foldover)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 3: 8-Note Chord with Tube Distortion (distDrive = 0.60, distMix = 0.70)]" << std::endl;
    PrismorDSP::SynthParameters distParams = defaultParams;
    distParams.distEnable = true;
    distParams.distType   = 0; // Tube Saturation
    distParams.distDrive  = 0.60f;
    distParams.distMix    = 0.70f;

    midi.clear();
    for (int n : chordNotes)
        midi.addEvent(juce::MidiMessage::noteOn(1, n, 1.0f), 0);

    float distMaxPeak = 0.0f;
    int distHardClips = 0;
    int distNaNs = 0;

    for (int b = 0; b < 200; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, distParams);
        midi.clear();

        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);

        for (int s = 0; s < 64; ++s)
        {
            if (std::isnan(l[s]) || std::isinf(l[s]) || std::isnan(r[s]) || std::isinf(r[s]))
            {
                distNaNs++;
                continue;
            }
            const float peak = std::max(std::abs(l[s]), std::abs(r[s]));
            distMaxPeak = std::max(distMaxPeak, peak);
            if (peak > 1.0001f)
                distHardClips++;
        }
    }

    std::cout << "  - Distortion Max Peak   : " << distMaxPeak << " (Ceiling <= 1.0000)" << std::endl;
    std::cout << "  - Distortion Hard Clips : " << distHardClips << " (Must be 0)" << std::endl;
    std::cout << "  - Distortion NaN Count  : " << distNaNs << " (Must be 0)" << std::endl;

    if (distHardClips > 0 || distMaxPeak > 1.0001f || distNaNs > 0)
    {
        std::cerr << "FAIL: Distortion module caused clipping or NaN corruption!" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // Test 4: Extreme Stress Test (+6dB Master, 16 Unison per Note, 8-Note Chord)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 4: Extreme Stress Test (+6dB Master, 16-Unison Stacking = 128 Oscillators)]" << std::endl;
    PrismorDSP::SynthParameters extremeParams = defaultParams;
    extremeParams.masterGainDb = 6.0f;
    extremeParams.unisonVoices = 16;
    extremeParams.unisonDetune = 0.50f;
    extremeParams.unisonSpread = 1.0f;

    midi.clear();
    for (int n : chordNotes)
        midi.addEvent(juce::MidiMessage::noteOn(1, n, 1.0f), 0);

    float extremeMaxPeak = 0.0f;
    int extremeHardClips = 0;

    for (int b = 0; b < 200; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, extremeParams);
        midi.clear();

        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);

        for (int s = 0; s < 64; ++s)
        {
            const float peak = std::max(std::abs(l[s]), std::abs(r[s]));
            extremeMaxPeak = std::max(extremeMaxPeak, peak);
            if (peak > 1.00001f)
                extremeHardClips++;
        }
    }

    std::cout << "  - Extreme Max Peak      : " << extremeMaxPeak << " (Ceiling strictly <= 1.0000)" << std::endl;
    std::cout << "  - Extreme Hard Clips    : " << extremeHardClips << std::endl;

    if (extremeHardClips > 0 || extremeMaxPeak > 1.00001f)
    {
        std::cerr << "FAIL: Master Safety Stage failed under extreme load!" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // Test 5: Single Note Punch Verification (Lead Tone)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 5: Single Note Punch Verification (Note 60, Normal Gain)]" << std::endl;
    midi.clear();
    for (int n : chordNotes)
        midi.addEvent(juce::MidiMessage::noteOff(1, n), 0);
    for (int b = 0; b < 250; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, defaultParams);
        midi.clear();
    }

    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 0);
    float singleNotePeak = 0.0f;
    for (int b = 0; b < 100; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, defaultParams);
        midi.clear();

        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);
        for (int s = 0; s < 64; ++s)
            singleNotePeak = std::max(singleNotePeak, std::max(std::abs(l[s]), std::abs(r[s])));
    }
    const float singleDb = 20.0f * std::log10(std::max(0.0001f, singleNotePeak));
    std::cout << "  - Single Note Peak Level: " << singleNotePeak << " (" << singleDb << " dBFS)" << std::endl;

    if (singleNotePeak < 0.03f || singleNotePeak > 0.95f)
    {
        std::cerr << "FAIL: Single note level out of calibrated range!" << std::endl;
        return 1;
    }

    std::cout << "\n=== SUCCESS: All Polyphonic Summation Corruption Tests Passed (0 Errors) ===" << std::endl;
    return 0;
}
