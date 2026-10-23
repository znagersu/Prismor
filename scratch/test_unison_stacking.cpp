#include <iostream>
#include <memory>
#include <cmath>
#include <cassert>
#include <JuceHeader.h>
#include "../Source/DSP/SpectralSynthEngine.h"

int main()
{
    std::cout << "--- Testing Single-Note Unison Stacking Architecture ---" << std::endl;
    auto enginePtr = std::make_unique<PrismorDSP::SpectralSynthEngine>();
    auto& engine = *enginePtr;
    engine.prepare(44100.0, 64);

    PrismorDSP::SynthParameters params;
    params.unisonVoices = 16;
    params.unisonDetune = 0.35f;
    params.unisonSpread = 0.85f;
    params.unisonBlend = 0.75f;
    params.masterGainDb = 0.0f;
    params.limiterEnable = true;

    // Trigger ONE single note (MIDI Note 60)
    engine.noteOn(60, 0.9f, params);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    // Render 1 block to process voices and update telemetry
    engine.renderBlock(buffer, midi, params);

    // Verify active voice count is exactly 1 (not 16!)
    int activeVoices = engine.getActiveVoiceCount();
    std::cout << "Active Voices count for 1 note with Unison=16: " << activeVoices << " (Expected: 1, NOT 16)" << std::endl;
    if (activeVoices != 1)
    {
        std::cerr << "FAIL: Polyphonic Voice Explosion detected! activeVoices = " << activeVoices << std::endl;
        return 1;
    }

    // Now trigger a second note (MIDI Note 64 - chord of 2 notes)
    engine.noteOn(64, 0.9f, params);
    engine.renderBlock(buffer, midi, params);
    int activeVoicesChord = engine.getActiveVoiceCount();
    std::cout << "Active Voices count for 2 notes with Unison=16: " << activeVoicesChord << " (Expected: 2, NOT 32)" << std::endl;
    if (activeVoicesChord != 2)
    {
        std::cerr << "FAIL: Chord voice allocation failed! activeVoices = " << activeVoicesChord << std::endl;
        return 1;
    }

    // Release note 64 so only note 60 remains
    engine.noteOff(64, params);
    for (int i = 0; i < 50; ++i)
        engine.renderBlock(buffer, midi, params);

    // Render 100 blocks with 16 unison sub-voices on 1 note
    float maxSample = 0.0f;
    bool hasClipping = false;
    for (int b = 0; b < 100; ++b)
    {
        midi.clear();
        engine.renderBlock(buffer, midi, params);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* r = buffer.getReadPointer(ch);
            for (int i = 0; i < 64; ++i)
            {
                float absVal = std::abs(r[i]);
                maxSample = std::max(maxSample, absVal);
                if (absVal > 1.0f)
                    hasClipping = true;
            }
        }
    }

    std::cout << "Peak Amplitude with Unison=16: " << maxSample << " (Peak <= 1.0 dBFS ceiling)" << std::endl;
    std::cout << "Clipping detected: " << (hasClipping ? "YES (FAIL)" : "NO (PASS)") << std::endl;

    if (!hasClipping && maxSample > 0.01f && maxSample <= 1.0f)
    {
        std::cout << "SUCCESS: Single-note 16-voice unison stacking verified with pristine headroom and zero clipping!" << std::endl;
        return 0;
    }
    return 1;
}
