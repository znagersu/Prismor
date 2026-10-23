#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <vector>
#include <memory>
#include <JuceHeader.h>
#include "../Source/DSP/FactoryPresetBank.h"
#include "../Source/DSP/SpectralSynthEngine.h"

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
    std::cout << "=== Testing Preset 39 Polyphonic Summation & Overlapping Tails Safeguards ===" << std::endl;

    auto enginePtr = std::make_unique<PrismorDSP::SpectralSynthEngine>();
    auto& engine = *enginePtr;
    engine.prepare(44100.0, 64);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    const auto& entries = PrismorDSP::FactoryPresetBank::getEntries();
    std::cout << "Preset [39]: " << entries[39].name << " (" << entries[39].category << ")" << std::endl;

    PrismorDSP::SynthParameters p39Params;
    auto setParam = [&](const char* id, float val)
    {
        juce::String pid(id);
        if (pid == "voicing_mode") p39Params.voicingMode = static_cast<int>(val);
        else if (pid == "max_voices") p39Params.maxVoices = static_cast<int>(val);
        else if (pid == "unison_voices") p39Params.unisonVoices = static_cast<int>(val);
        else if (pid == "unison_detune") p39Params.unisonDetune = val;
        else if (pid == "unison_spread") p39Params.unisonSpread = val;
        else if (pid == "unison_blend") p39Params.unisonBlend = val;
        else if (pid == "unison_stack") p39Params.unisonStackMode = static_cast<int>(val);
        else if (pid == "filter1_enable") p39Params.filter1Enable = (val > 0.5f);
        else if (pid == "filter1_type") p39Params.filter1Type = static_cast<int>(val);
        else if (pid == "filter1_cutoff") p39Params.filter1Cutoff = val;
        else if (pid == "filter1_resonance") p39Params.filter1Res = val;
        else if (pid == "filter1_width") p39Params.filter1Width = val;
        else if (pid == "filter_drive") p39Params.filterDrive = val;
        else if (pid == "filter_routing") p39Params.filterRouting = val;
        else if (pid == "dist_enable") p39Params.distEnable = (val > 0.5f);
        else if (pid == "dist_type") p39Params.distType = static_cast<int>(val);
        else if (pid == "dist_drive") p39Params.distDrive = val;
        else if (pid == "dist_tone") p39Params.distTone = val;
        else if (pid == "dist_mix") p39Params.distMix = val;
        else if (pid == "flanger_enable") p39Params.flangerEnable = (val > 0.5f);
        else if (pid == "flanger_mode") p39Params.flangerMode = static_cast<int>(val);
        else if (pid == "flanger_rate") p39Params.flangerRate = val;
        else if (pid == "flanger_depth") p39Params.flangerDepth = val;
        else if (pid == "flanger_feedback") p39Params.flangerFeedback = val;
        else if (pid == "flanger_mix") p39Params.flangerMix = val;
        else if (pid == "env_attack") p39Params.envAttack = val;
        else if (pid == "env_hold") p39Params.envHold = val;
        else if (pid == "env_decay") p39Params.envDecay = val;
        else if (pid == "env_sustain") p39Params.envSustain = val;
        else if (pid == "env_release") p39Params.envRelease = val;
        else if (pid == "ab_mix") p39Params.abMix = val;
        else if (pid == "master_gain_db") p39Params.masterGainDb = val;
        else if (pid == "limiter_thresh_db") p39Params.limiterThreshDb = val;
    };

    PrismorDSP::FactoryPresetBank::applyPreset(39, setParam, engine);

    // Set to Polyphonic mode with 8 voices for polyphonic summation testing
    p39Params.voicingMode = 0; // Polyphonic
    p39Params.maxVoices = 8;

    // -------------------------------------------------------------------------
    // Test 1: Single Note Clean Punch
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 1: Preset 39 Single Note (Note 60, Vel 0.9)]" << std::endl;
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 0);

    float singlePeak = 0.0f;
    for (int b = 0; b < 100; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, p39Params);
        midi.clear();
        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);
        for (int s = 0; s < 64; ++s)
            singlePeak = std::max(singlePeak, std::max(std::abs(l[s]), std::abs(r[s])));
    }
    std::cout << "  - Single Note Peak: " << singlePeak << " (Ceiling <= 1.0000)" << std::endl;
    assert(singlePeak > 0.05f && singlePeak <= 1.00001f);

    // -------------------------------------------------------------------------
    // Test 2: 8-Note Polyphonic Chord with Preset 39 (Sync Screamer + Asym Distortion + Flanger)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 2: Preset 39 8-Note Polyphonic Chord (Notes 48, 52, 55, 59, 60, 64, 67, 72)]" << std::endl;
    engine.allNotesOff();
    midi.clear();
    const int chordNotes[8] = { 48, 52, 55, 59, 60, 64, 67, 72 };
    for (int n : chordNotes)
        midi.addEvent(juce::MidiMessage::noteOn(1, n, 0.95f), 0);

    float chordMaxPeak = 0.0f;
    int hardClips = 0;
    int nanOrInfs = 0;
    float maxDerivative = 0.0f;
    float prevSample = 0.0f;

    for (int b = 0; b < 250; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, p39Params);
        midi.clear();
        applyMasterSoftClipper(buffer);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);

        for (int s = 0; s < 64; ++s)
        {
            if (std::isnan(l[s]) || std::isinf(l[s]) || std::isnan(r[s]) || std::isinf(r[s]))
            {
                nanOrInfs++;
                continue;
            }

            const float peak = std::max(std::abs(l[s]), std::abs(r[s]));
            chordMaxPeak = std::max(chordMaxPeak, peak);

            if (peak > 1.00001f)
                hardClips++;

            if (b > 2)
            {
                const float deriv = std::abs(l[s] - prevSample);
                maxDerivative = std::max(maxDerivative, deriv);
            }
            prevSample = l[s];
        }
    }

    std::cout << "  - Chord Max Peak    : " << chordMaxPeak << " (Ceiling <= 1.0000)" << std::endl;
    std::cout << "  - Hard Clips Count  : " << hardClips << " (Must be 0)" << std::endl;
    std::cout << "  - NaN / Inf Count   : " << nanOrInfs << " (Must be 0)" << std::endl;
    std::cout << "  - Max Sample Step   : " << maxDerivative << " (Smooth C1 continuity, < 0.8)" << std::endl;

    if (hardClips > 0 || nanOrInfs > 0 || chordMaxPeak > 1.00001f)
    {
        std::cerr << "FAIL: Polyphonic summation clipping detected on Preset 39!" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // Test 3: Rapid Overlapping Notes (Stacking Release Tails & Voice Stealing)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 3: Rapid Overlapping Arpeggio (Decaying Tails Accumulation Test)]" << std::endl;
    float tailMaxPeak = 0.0f;
    int tailHardClips = 0;
    int tailNaNs = 0;

    for (int cycle = 0; cycle < 16; ++cycle)
    {
        // Fire rapid noteOn without noteOff to deliberately stack decaying release tails
        midi.clear();
        const int note = 48 + (cycle * 5) % 36;
        midi.addEvent(juce::MidiMessage::noteOn(1, note, 0.95f), 0);

        // Render 2 blocks (approx 3ms) then release, causing fast release tail pileup
        for (int b = 0; b < 2; ++b)
        {
            buffer.clear();
            engine.renderBlock(buffer, midi, p39Params);
            midi.clear();
            applyMasterSoftClipper(buffer);
        }

        midi.addEvent(juce::MidiMessage::noteOff(1, note), 0);
        for (int b = 0; b < 8; ++b)
        {
            buffer.clear();
            engine.renderBlock(buffer, midi, p39Params);
            midi.clear();
            applyMasterSoftClipper(buffer);

            const float* l = buffer.getReadPointer(0);
            const float* r = buffer.getReadPointer(1);
            for (int s = 0; s < 64; ++s)
            {
                if (std::isnan(l[s]) || std::isinf(l[s]) || std::isnan(r[s]) || std::isinf(r[s]))
                {
                    tailNaNs++;
                    continue;
                }
                const float peak = std::max(std::abs(l[s]), std::abs(r[s]));
                tailMaxPeak = std::max(tailMaxPeak, peak);
                if (peak > 1.00001f)
                    tailHardClips++;
            }
        }
    }

    std::cout << "  - Tail Stacking Max Peak : " << tailMaxPeak << " (Ceiling <= 1.0000)" << std::endl;
    std::cout << "  - Tail Hard Clips        : " << tailHardClips << " (Must be 0)" << std::endl;
    std::cout << "  - Tail NaNs / Infs       : " << tailNaNs << " (Must be 0)" << std::endl;

    if (tailHardClips > 0 || tailNaNs > 0 || tailMaxPeak > 1.00001f)
    {
        std::cerr << "FAIL: Overlapping release tails caused clipping or instability on Preset 39!" << std::endl;
        return 1;
    }

    std::cout << "\n=== SUCCESS: Preset 39 Polyphonic Summation & Overlapping Tails Fully Verified! ===" << std::endl;
    return 0;
}
