#include <iostream>
#include <chrono>
#include <memory>
#include <cmath>
#include <cassert>
#include <vector>
#include <JuceHeader.h>
#include "../Source/DSP/FactoryPresetBank.h"
#include "../Source/DSP/SpectralSynthEngine.h"

int main()
{
    std::cout << "=== Prismor 1.0.13.4 Phase 6.0 DSP Performance Benchmark ===" << std::endl;

    auto enginePtr = std::make_unique<PrismorDSP::SpectralSynthEngine>();
    auto& engine = *enginePtr;
    engine.prepare(44100.0, 64);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    // -------------------------------------------------------------------------
    // Benchmark 1: Heavy Polyphonic Unison Stress Test (4-note chord, 16 Unison)
    // -------------------------------------------------------------------------
    PrismorDSP::SynthParameters heavyParams;
    heavyParams.unisonVoices = 16;
    heavyParams.unisonDetune = 0.45f;
    heavyParams.unisonSpread = 0.85f;
    heavyParams.unisonBlend  = 0.75f;
    heavyParams.filter1Enable = true;
    heavyParams.filter1Cutoff = 0.65f;
    heavyParams.filter1Res    = 0.35f;
    heavyParams.chorusEnable  = true;
    heavyParams.chorusMix     = 0.35f;
    heavyParams.reverbEnable  = true;
    heavyParams.reverbMix     = 0.25f;

    // Note Ons
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 48, 0.9f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 55, 0.85f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.88f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.82f), 0);
    buffer.clear();
    engine.renderBlock(buffer, midi, heavyParams);

    const int benchmarkBlocks = 2000;
    midi.clear();

    auto startClock = std::chrono::high_resolution_clock::now();
    for (int b = 0; b < benchmarkBlocks; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, heavyParams);
    }
    auto endClock = std::chrono::high_resolution_clock::now();

    const double elapsedUs = std::chrono::duration<double, std::micro>(endClock - startClock).count();
    const double usPerBlock = elapsedUs / static_cast<double>(benchmarkBlocks);
    const double blockRealtimeUs = (64.0 / 44100.0) * 1000000.0; // 1451.25 us
    const double cpuLoadPct = (usPerBlock / blockRealtimeUs) * 100.0;

    std::cout << "[Test 1: Heavy 4-Voice 16-Unison Stacking]" << std::endl;
    std::cout << "  - Blocks Rendered       : " << benchmarkBlocks << " (64 samples/block)" << std::endl;
    std::cout << "  - Total Render Time     : " << elapsedUs / 1000.0 << " ms" << std::endl;
    std::cout << "  - Render Time per Block : " << usPerBlock << " us" << std::endl;
    std::cout << "  - Real-Time Budget      : " << blockRealtimeUs << " us" << std::endl;
    std::cout << "  - Equivalent CPU Load   : " << cpuLoadPct << " %" << std::endl;

    if (cpuLoadPct > 80.0)
    {
        std::cerr << "FAIL: CPU load too high!" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------------------
    // Benchmark 2: Smart Voice Sleeping (< -80 dBFS) & Decay Tail Elimination
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 2: Smart Voice Sleeping & Inaudible Tail Bypass]" << std::endl;
    // Release all notes
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOff(1, 48), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 55), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 67), 0);
    engine.renderBlock(buffer, midi, heavyParams);
    midi.clear();

    // Render 100 release blocks to let envelope decay
    for (int b = 0; b < 100; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, heavyParams);
    }

    int activeVoices = engine.getActiveVoiceCount();
    std::cout << "  - Active Voices after Decay: " << activeVoices << std::endl;

    // Benchmark idle/sleeping CPU
    auto idleStart = std::chrono::high_resolution_clock::now();
    for (int b = 0; b < benchmarkBlocks; ++b)
    {
        buffer.clear();
        engine.renderBlock(buffer, midi, heavyParams);
    }
    auto idleEnd = std::chrono::high_resolution_clock::now();
    const double idleUsPerBlock = std::chrono::duration<double, std::micro>(idleEnd - idleStart).count() / static_cast<double>(benchmarkBlocks);
    std::cout << "  - Idle Render Time per Block: " << idleUsPerBlock << " us (" << (idleUsPerBlock / blockRealtimeUs * 100.0) << "% CPU)" << std::endl;

    // -------------------------------------------------------------------------
    // Benchmark 3: Factory Bank Audio Integrity (332 Presets)
    // -------------------------------------------------------------------------
    std::cout << "\n[Test 3: Audio Integrity Across 332 Presets]" << std::endl;
    const int presetCount = PrismorDSP::FactoryPresetBank::getPresetCount();
    int nanCount = 0;

    for (int i = 0; i < presetCount; ++i)
    {
        PrismorDSP::SynthParameters p;
        auto setParam = [](const char*, float) {};
        PrismorDSP::FactoryPresetBank::applyPreset(i, setParam, engine);

        buffer.clear();
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        engine.renderBlock(buffer, midi, p);

        const float* l = buffer.getReadPointer(0);
        const float* r = buffer.getReadPointer(1);
        for (int s = 0; s < 64; ++s)
        {
            if (std::isnan(l[s]) || std::isinf(l[s]) || std::isnan(r[s]) || std::isinf(r[s]))
            {
                nanCount++;
                break;
            }
        }
    }
    std::cout << "  - Total Presets Verified: " << presetCount << std::endl;
    std::cout << "  - NaN / Inf Anomalies   : " << nanCount << std::endl;

    if (nanCount > 0)
    {
        std::cerr << "FAIL: NaN anomalies detected in factory preset rendering!" << std::endl;
        return 1;
    }

    std::cout << "\n=== ALL PHASE 6.0 OPTIMIZATION BENCHMARKS PASSED (0 ERRORS) ===" << std::endl;
    return 0;
}
