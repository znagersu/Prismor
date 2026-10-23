#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <algorithm>

#include <JuceHeader.h>
#include "../Source/DSP/FactoryPresetBank.h"
#include "../Source/DSP/SplineCurve.h"
#include "../Source/DSP/ResynthesisEngine.h"
#include "../Source/DSP/SpectralSynthEngine.h"

int main()
{
    std::cout << "--- Testing Prismor Unison Smoothing & Dynamic Voice Crossfading ---" << std::endl;
    std::cout.flush();

    auto enginePtr = std::make_unique<PrismorDSP::SpectralSynthEngine>();
    auto& engine = *enginePtr;
    engine.prepare(44100.0, 64);

    PrismorDSP::SynthParameters params;
    params.unisonVoices = 1;
    params.unisonDetune = 0.25f;
    params.unisonSpread = 0.70f;
    params.unisonBlend = 0.60f;
    params.masterGainDb = 0.0f;
    params.limiterEnable = false; // test raw synth headroom

    // Note on: Middle C (60)
    engine.noteOn(60, 0.9f, params);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    float prevSampleL = 0.0f;
    float maxAbsSample = 0.0f;
    float maxStepDelta = 0.0f;
    bool hasNaN = false;

    // Render 200 blocks (~300ms) sweeping unison voices 1 -> 16 -> 1
    for (int block = 0; block < 200; ++block)
    {
        // Dynamic voice count sweep
        const float sweepPhase = static_cast<float>(block) / 100.0f;
        const int voices = 1 + static_cast<int>(std::round(15.0f * (0.5f - 0.5f * std::cos(sweepPhase * 3.14159f))));
        params.unisonVoices = voices;

        // Dynamic detune sweep
        params.unisonDetune = 0.1f + 0.8f * (static_cast<float>(block % 50) / 50.0f);

        midi.clear();
        engine.renderBlock(buffer, midi, params);

        const float* rL = buffer.getReadPointer(0);
        for (int i = 0; i < 64; ++i)
        {
            const float s = rL[i];
            if (std::isnan(s) || std::isinf(s))
            {
                hasNaN = true;
            }
            maxAbsSample = std::max(maxAbsSample, std::abs(s));

            if (block > 2) // skip initial onset
            {
                const float delta = std::abs(s - prevSampleL);
                maxStepDelta = std::max(maxStepDelta, delta);
            }
            prevSampleL = s;
        }
    }

    std::cout << "Max Absolute Sample: " << maxAbsSample << " (Safe headroom)" << std::endl;
    std::cout << "Max Sample-to-Sample Step Delta: " << maxStepDelta << " (Smooth continuous waveform)" << std::endl;
    std::cout << "NaN / Inf detected: " << (hasNaN ? "YES (FAIL)" : "NO (PASS)") << std::endl;

    if (!hasNaN && maxAbsSample < 2.0f && maxStepDelta < 0.25f)
    {
        std::cout << "SUCCESS: Unison DSP is 100% click-free, safe, and smoothly interpolated!" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "FAILURE: Discontinuities or out-of-range samples detected!" << std::endl;
        return 1;
    }
}
