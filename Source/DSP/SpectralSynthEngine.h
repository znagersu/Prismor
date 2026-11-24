#pragma once
#include <JuceHeader.h>
#include "DspUtils.h"
#include "SplineCurve.h"
#include "ResynthesisEngine.h"
#include "FxRack.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <memory>

namespace PrismorDSP
{

class SpectralSynthEngine
{
public:
    static constexpr int NUM_PARTIALS        = 128;
    static constexpr int ADDITIVE_PARTIALS   = 48;
    static constexpr int MAX_VOICES          = 16;
    static constexpr int MAX_UNISON          = 16;
    static constexpr int CONTROL_RATE_SAMPLES = 32;
    static constexpr int SINE_LUT_SIZE       = 8192;
    static constexpr int NUM_ARP_STEPS       = 16;
    static constexpr int MAX_FX_CHAIN_LEN    = FxRack::kMaxFxChainLen;

    SpectralSynthEngine()
        : voices(std::make_unique<std::array<Voice, MAX_VOICES>>())
    {
        initSineLUT();
        timeModCurve.applyTimeModPreset(0);
        resetFxChainOrder();
        resetArpStepsToDefault(0);
        for (int i = 0; i < static_cast<int>(ArticulatorTarget::Count); ++i)
            articulators[static_cast<size_t>(i)].resetToDefault(static_cast<ArticulatorTarget>(i));
    }

    void prepare(double newSampleRate, int samplesPerBlock)
    {
        sampleRate    = std::max(22050.0, newSampleRate);
        invSampleRate = 1.0f / static_cast<float>(sampleRate);

        const double rampSeconds = 0.015;
        smoothedDetune.reset(sampleRate, rampSeconds);
        smoothedSpread.reset(sampleRate, rampSeconds);
        smoothedBlend.reset(sampleRate, rampSeconds);
        smoothedUnisonVoices.reset(sampleRate, rampSeconds);

        smoothedDetune.setCurrentAndTargetValue(0.18f);
        smoothedSpread.setCurrentAndTargetValue(0.65f);
        smoothedBlend.setCurrentAndTargetValue(0.55f);
        smoothedUnisonVoices.setCurrentAndTargetValue(1.0f);
        smoothedPolyHeadroom.reset(sampleRate, 0.008);
        smoothedPolyHeadroom.setCurrentAndTargetValue(1.0f);

        fxRack.prepare(sampleRate, std::max(512, samplesPerBlock));
        resetAllVoices();
    }

    void resetAllVoices()
    {
        for (auto& v : *voices)
            v.reset();
        heldNotes.clear();
        arpHeldNotes.clear();
        activeArpTriggeredNotes.clear();
        arpWasEnabled = false;
        pendingSnapshot.activeVoiceCount = 0;
        pendingSnapshot.voiceActive = false;
        activeVoicesAtomic.store(0, std::memory_order_relaxed);
        smoothedPolyHeadroom.setCurrentAndTargetValue(1.0f);
        fxRack.reset();
    }

    // -------------------------------------------------------------------------
    // Dynamic Add / Remove / Drag-and-Drop Reorderable FX Rack API (13 Modules)
    // Lock-free fixed-size cache & bitmask for zero audio-thread contention!
    // -------------------------------------------------------------------------
    void syncActiveFxChainArray() noexcept
    {
        const int count = std::min(static_cast<int>(activeFxChain.size()), MAX_FX_CHAIN_LEN);
        uint32_t mask = 0u;
        for (int i = 0; i < count; ++i)
        {
            const int id = activeFxChain[static_cast<size_t>(i)];
            activeFxChainArray[static_cast<size_t>(i)] = id;
            if (id >= 0 && id < 32)
                mask |= (1u << static_cast<uint32_t>(id));
        }
        activeFxMask.store(mask, std::memory_order_release);
        activeFxChainCount.store(count, std::memory_order_release);
    }

    bool isFxModuleActive(FxModuleId id) const noexcept
    {
        const uint32_t mask = activeFxMask.load(std::memory_order_acquire);
        const int bit = static_cast<int>(id);
        return (bit >= 0 && bit < 32) ? ((mask & (1u << static_cast<uint32_t>(bit))) != 0u) : false;
    }

    void resetFxChainOrder() noexcept
    {
        juce::ScopedLock sl(fxOrderLock);
        // Default 12 active modules in musical studio order (ParametricEQ can be swapped/added anytime; wait: include EQ too? Check 12 vs 13!)
        // Keep all 13 modules in default active chain so EQ, Bode, and all Zenology packs work out of the box!
        activeFxChain = {
            static_cast<int>(FxModuleId::Blur),
            static_cast<int>(FxModuleId::Tremolo),
            static_cast<int>(FxModuleId::Distortion),
            static_cast<int>(FxModuleId::RingMod),
            static_cast<int>(FxModuleId::Bitcrusher),
            static_cast<int>(FxModuleId::Flanger),
            static_cast<int>(FxModuleId::Bode),
            static_cast<int>(FxModuleId::ParametricEQ),
            static_cast<int>(FxModuleId::Compressor),
            static_cast<int>(FxModuleId::Phaser),
            static_cast<int>(FxModuleId::Chorus),
            static_cast<int>(FxModuleId::Delay),
            static_cast<int>(FxModuleId::Reverb)
        };
        syncActiveFxChainArray();
    }

    std::vector<int> getActiveFxChain() const
    {
        juce::ScopedLock sl(fxOrderLock);
        return activeFxChain;
    }

    void setActiveFxChain(const std::vector<int>& newChain)
    {
        juce::ScopedLock sl(fxOrderLock);
        activeFxChain.clear();
        for (int id : newChain)
        {
            if (id >= 0 && id < static_cast<int>(FxModuleId::Count))
            {
                if (std::find(activeFxChain.begin(), activeFxChain.end(), id) == activeFxChain.end())
                    activeFxChain.push_back(id);
            }
        }
        syncActiveFxChainArray();
    }

    bool addFxModule(int moduleId)
    {
        if (moduleId < 0 || moduleId >= static_cast<int>(FxModuleId::Count))
            return false;
        juce::ScopedLock sl(fxOrderLock);
        if (std::find(activeFxChain.begin(), activeFxChain.end(), moduleId) != activeFxChain.end())
            return false;
        activeFxChain.push_back(moduleId);
        syncActiveFxChainArray();
        return true;
    }

    bool removeFxSlot(int slotIndex)
    {
        juce::ScopedLock sl(fxOrderLock);
        if (slotIndex < 0 || slotIndex >= static_cast<int>(activeFxChain.size()))
            return false;
        activeFxChain.erase(activeFxChain.begin() + slotIndex);
        syncActiveFxChainArray();
        return true;
    }

    void swapFxSlots(int slotA, int slotB) noexcept
    {
        juce::ScopedLock sl(fxOrderLock);
        const int n = static_cast<int>(activeFxChain.size());
        if (slotA < 0 || slotA >= n || slotB < 0 || slotB >= n || slotA == slotB)
            return;
        std::swap(activeFxChain[static_cast<size_t>(slotA)], activeFxChain[static_cast<size_t>(slotB)]);
        syncActiveFxChainArray();
    }

    void moveFxSlot(int fromSlot, int toSlot) noexcept
    {
        juce::ScopedLock sl(fxOrderLock);
        const int n = static_cast<int>(activeFxChain.size());
        if (fromSlot < 0 || fromSlot >= n || toSlot < 0 || toSlot >= n || fromSlot == toSlot)
            return;
        const int modId = activeFxChain[static_cast<size_t>(fromSlot)];
        activeFxChain.erase(activeFxChain.begin() + fromSlot);
        activeFxChain.insert(activeFxChain.begin() + toSlot, modId);
        syncActiveFxChainArray();
    }

    // -------------------------------------------------------------------------
    // Lock-Free Double-Buffered Arpeggiator & 16-Step Sequencer API
    // -------------------------------------------------------------------------
    ArpStep getArpStep(int stepIdx) const noexcept
    {
        const int idx = activeArpBufIndex.load(std::memory_order_acquire);
        return arpStepsBuf[static_cast<size_t>(idx)][static_cast<size_t>(juce::jlimit(0, NUM_ARP_STEPS - 1, stepIdx))];
    }

    void setArpStep(int stepIdx, const ArpStep& step) noexcept
    {
        juce::ScopedLock sl(arpLock);
        const int cur = activeArpBufIndex.load(std::memory_order_relaxed);
        const int nxt = 1 - cur;
        arpStepsBuf[static_cast<size_t>(nxt)] = arpStepsBuf[static_cast<size_t>(cur)];
        auto& s = arpStepsBuf[static_cast<size_t>(nxt)][static_cast<size_t>(juce::jlimit(0, NUM_ARP_STEPS - 1, stepIdx))];
        s.enabled = step.enabled;
        s.velocity = juce::jlimit(0.05f, 1.0f, step.velocity);
        s.octaveOffset = juce::jlimit(-1, 2, step.octaveOffset);
        s.semitoneTranspose = juce::jlimit(-12, 12, step.semitoneTranspose);
        activeArpBufIndex.store(nxt, std::memory_order_release);
    }

    void resetArpStepsToDefault(int patternPreset = 0) noexcept
    {
        juce::ScopedLock sl(arpLock);
        const int cur = activeArpBufIndex.load(std::memory_order_relaxed);
        const int nxt = 1 - cur;
        auto& steps = arpStepsBuf[static_cast<size_t>(nxt)];

        for (int i = 0; i < NUM_ARP_STEPS; ++i)
        {
            auto& s = steps[static_cast<size_t>(i)];
            s.enabled = true;
            s.velocity = (i % 4 == 0) ? 0.95f : ((i % 2 == 0) ? 0.78f : 0.65f);
            s.octaveOffset = 0;
            s.semitoneTranspose = 0;

            if (patternPreset == 1)
            {
                s.enabled = (i != 3 && i != 11);
                s.octaveOffset = (i % 4 == 2) ? 1 : ((i == 7 || i == 15) ? 2 : 0);
                s.semitoneTranspose = (i == 5 || i == 13) ? 7 : 0;
                s.velocity = (i % 2 == 0) ? 0.92f : 0.68f;
            }
            else if (patternPreset == 2)
            {
                s.enabled = (i != 6 && i != 14);
                s.octaveOffset = (i % 2 == 1) ? 1 : ((i == 4 || i == 12) ? -1 : 0);
                s.semitoneTranspose = (i == 15) ? 3 : 0;
                s.velocity = (i % 4 == 0) ? 1.0f : 0.74f;
            }
            else if (patternPreset == 3)
            {
                static constexpr int acidSemi[16] = { 0, 0, 12, 0, 3, 7, 0, -5, 0, 10, 7, 3, 0, 12, 7, 3 };
                static constexpr int acidOct[16]  = { 0, 0, 1,  0, 0, 1, -1, 0, 0, 1,  0, 0, -1, 1, 0, 0 };
                s.enabled = (i != 1 && i != 8);
                s.octaveOffset = acidOct[i];
                s.semitoneTranspose = acidSemi[i];
                s.velocity = (i % 3 == 0) ? 0.98f : 0.62f;
            }
            else if (patternPreset == 4)
            {
                static constexpr bool gateOn[16] = { true, true, false, true, true, false, true, true, true, false, true, true, false, true, true, true };
                s.enabled = gateOn[i];
                s.octaveOffset = (i >= 12) ? 1 : 0;
                s.semitoneTranspose = (i == 7) ? 7 : ((i == 15) ? 12 : 0);
                s.velocity = s.enabled ? (0.72f + 0.25f * ((i % 4 == 0) ? 1.0f : 0.0f)) : 0.5f;
            }
        }
        activeArpBufIndex.store(nxt, std::memory_order_release);
    }

    int getCurrentArpStep() const noexcept { return currentArpStepAtomic.load(std::memory_order_relaxed); }

    SplineCurve& getArticulator(ArticulatorTarget target) noexcept { return articulators[static_cast<size_t>(target)]; }
    const SplineCurve& getArticulator(ArticulatorTarget target) const noexcept { return articulators[static_cast<size_t>(target)]; }

    SplineCurve& getTimeModCurve() noexcept { return timeModCurve; }
    const SplineCurve& getTimeModCurve() const noexcept { return timeModCurve; }

    void notifyArticulatorRateEdited(ArticulatorTarget target) noexcept
    {
        const size_t tIdx = static_cast<size_t>(target);
        for (auto& v : *voices)
        {
            if (v.active && v.artPhase[tIdx] >= 0.98f)
                v.artPhase[tIdx] = 0.0f;
        }
    }

    ResynthesisEngine& getResynthesisEngine() noexcept { return resynthEngine; }
    const ResynthesisEngine& getResynthesisEngine() const noexcept { return resynthEngine; }

    int getActiveVoiceCount() const noexcept { return activeVoicesAtomic.load(std::memory_order_relaxed); }

    VisualizerSnapshot getLatestVisualizerSnapshot() const noexcept
    {
        const int idx = activeVisIndex.load(std::memory_order_acquire);
        return visSnapshots[static_cast<size_t>(idx)];
    }

    static float evaluateParametricEQGain(float freqHz, const SynthParameters& params) noexcept
    {
        if (!params.eqEnable || freqHz <= 10.0f)
            return 1.0f;

        float totalDb = 0.0f;
        if (std::abs(params.eqLowGain) > 0.05f)
        {
            const float ratio = freqHz / std::max(20.0f, params.eqLowFreq);
            const float shelf = 1.0f / (1.0f + std::pow(ratio, 2.4f));
            totalDb += params.eqLowGain * shelf;
        }
        if (std::abs(params.eqMidGain) > 0.05f)
        {
            const float octDiff = std::log2(freqHz / std::max(40.0f, params.eqMidFreq));
            const float bwOct = 1.1f / std::max(0.25f, params.eqMidQ);
            const float bell = std::exp(-0.5f * std::pow(octDiff / bwOct, 2.0f));
            totalDb += params.eqMidGain * bell;
        }
        if (std::abs(params.eqHighGain) > 0.05f)
        {
            const float ratio = freqHz / std::max(400.0f, params.eqHighFreq);
            const float shelf = 1.0f - (1.0f / (1.0f + std::pow(ratio, 2.4f)));
            totalDb += params.eqHighGain * shelf;
        }
        return juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, totalDb));
    }

    static float evaluateFilterMask(int filterType,
                                    float freqHz,
                                    float cutoffHz,
                                    float resonance,
                                    float width,
                                    const SplineCurve& customShape) noexcept
    {
        if (freqHz <= 10.0f)
            return 1.0f;

        const float safeRes   = juce::jlimit(0.0f, 0.94f, resonance);
        const float octDiff   = std::log2(freqHz / std::max(20.0f, cutoffHz));
        const float bandwidth = std::max(0.15f, 0.2f + width * 2.0f);
        float gain = 1.0f;

        switch (filterType)
        {
            case 0: // SVF Lowpass 12dB
            {
                if (octDiff > -0.5f)
                {
                    const float ratio = std::min(64.0f, freqHz / std::max(20.0f, cutoffHz));
                    gain = 1.0f / std::sqrt(1.0f + std::pow(ratio, std::min(20.0f, 4.0f / bandwidth)));
                }
                const float resBell = std::exp(-0.5f * std::pow(octDiff / (0.28f * bandwidth), 2.0f));
                gain += safeRes * 2.2f * resBell * std::min(1.0f, gain * 2.0f);
                break;
            }
            case 1: // SVF Lowpass 24dB
            {
                if (octDiff > -0.3f)
                {
                    const float ratio = std::min(64.0f, freqHz / std::max(20.0f, cutoffHz));
                    gain = 1.0f / (1.0f + std::pow(ratio, std::min(24.0f, 8.0f / bandwidth)));
                }
                const float resBell = std::exp(-0.5f * std::pow(octDiff / (0.20f * bandwidth), 2.0f));
                gain += safeRes * 2.6f * resBell * std::min(1.0f, gain * 2.2f);
                break;
            }
            case 2: // Crude Low Pass
            {
                if (octDiff > 0.0f)
                {
                    const float slope = octDiff / (0.06f + width * 0.45f);
                    gain = std::exp(-slope * slope * 2.8f);
                }
                const float resBell = std::exp(-0.5f * std::pow((octDiff + 0.04f) / 0.14f, 2.0f));
                gain += safeRes * 2.4f * resBell;
                break;
            }
            case 3: // SVF Highpass 24dB
            {
                const float ratio = std::min(64.0f, std::max(20.0f, cutoffHz) / freqHz);
                gain = 1.0f / (1.0f + std::pow(ratio, std::min(24.0f, 6.0f / bandwidth)));
                const float resBell = std::exp(-0.5f * std::pow(octDiff / (0.22f * bandwidth), 2.0f));
                gain += safeRes * 2.2f * resBell * std::min(1.0f, gain * 2.0f);
                break;
            }
            case 4: // Bandpass SVF
            {
                const float bw = 0.25f + width * 1.4f;
                gain = std::exp(-0.5f * std::pow(octDiff / bw, 2.0f));
                const float resPeak = std::exp(-0.5f * std::pow(octDiff / (bw * 0.35f), 2.0f));
                gain += safeRes * 1.9f * resPeak;
                break;
            }
            case 5: // Spectral Comb
            {
                const float combFreq = std::max(40.0f, cutoffHz * 0.25f);
                const float phase = (freqHz / combFreq) * juce::MathConstants<float>::pi;
                const float comb = std::max(0.0f, 0.5f + 0.5f * std::cos(phase));
                const float depth = 0.45f + 0.55f * (1.0f - width * 0.5f);
                gain = (1.0f - depth) + depth * std::pow(comb, 1.0f + safeRes * 2.6f) * (1.0f + safeRes * 1.2f);
                break;
            }
            case 6: // Vox Formant
            {
                static constexpr float vF1[5] = { 730.0f, 530.0f, 270.0f, 570.0f, 300.0f };
                static constexpr float vF2[5] = { 1090.0f, 1840.0f, 2290.0f, 840.0f, 870.0f };
                static constexpr float vF3[5] = { 2440.0f, 2480.0f, 3010.0f, 2410.0f, 2240.0f };
                const float vowelPos = juce::jlimit(0.0f, 1.0f, std::log2(std::max(25.0f, cutoffHz) / 25.0f) / 9.5f) * 4.0f;
                const int idx0 = juce::jlimit(0, 3, static_cast<int>(vowelPos));
                const int idx1 = idx0 + 1;
                const float frac = vowelPos - static_cast<float>(idx0);
                const float formantShift = 0.55f + width * 1.05f;
                const float f1 = (vF1[idx0] + frac * (vF1[idx1] - vF1[idx0])) * formantShift;
                const float f2 = (vF2[idx0] + frac * (vF2[idx1] - vF2[idx0])) * formantShift;
                const float f3 = (vF3[idx0] + frac * (vF3[idx1] - vF3[idx0])) * formantShift;
                const float q = 0.11f + (1.0f - safeRes * 0.72f) * 0.22f;
                const float p1 = std::exp(-0.5f * std::pow(std::log2(freqHz / f1) / q, 2.0f));
                const float p2 = 0.90f * std::exp(-0.5f * std::pow(std::log2(freqHz / f2) / q, 2.0f));
                const float p3 = 0.65f * std::exp(-0.5f * std::pow(std::log2(freqHz / f3) / (q * 1.15f), 2.0f));
                const float baseRolloff = 1.0f / (1.0f + std::pow(freqHz / 5200.0f, 2.0f));
                gain = (0.08f + (1.35f + safeRes * 1.60f) * (p1 + p2 + p3)) * (0.35f + 0.65f * baseRolloff);
                break;
            }
            case 7: // Custom Shape
            {
                const float normFreq = juce::jlimit(0.0f, 1.0f,
                    (std::log2(freqHz / std::max(25.0f, cutoffHz * 0.12f))) / (4.5f + width * 5.0f));
                gain = customShape.evaluateLUT(normFreq);
                const float resBell = std::exp(-0.5f * std::pow(octDiff / 0.25f, 2.0f));
                gain += safeRes * 1.5f * resBell * gain;
                break;
            }
            case 8: // Vocoder Band-Bank
            {
                const float centerShift = std::pow(2.0f, (std::log2(std::max(25.0f, cutoffHz) / 600.0f)) * 0.45f);
                const float shiftedFreq = freqHz / std::max(0.25f, centerShift);
                const float bandDensity = 2.0f + (1.0f - width * 0.75f) * 3.5f;
                const float logBandPos = std::log2(std::max(40.0f, shiftedFreq) / 65.0f) * bandDensity;
                const float bandPhase = logBandPos * juce::MathConstants<float>::twoPi;
                const float chBase = std::max(0.0f, 0.5f + 0.5f * std::cos(bandPhase));
                const float channelTransmission = std::pow(chBase, 1.8f + safeRes * 7.5f);
                const float voxEnv = std::exp(-0.5f * std::pow(std::log2(shiftedFreq / 520.0f) / 1.4f, 2.0f))
                                   + 0.85f * std::exp(-0.5f * std::pow(std::log2(shiftedFreq / 1950.0f) / 1.1f, 2.0f));
                const float bandFloor = 0.04f * (1.0f - safeRes * 0.85f);
                gain = (bandFloor + channelTransmission * (1.20f + safeRes * 1.50f)) * juce::jlimit(0.15f, 1.35f, voxEnv);
                break;
            }
            case 9: // Talkbox Mode
            default:
            {
                const float morph = juce::jlimit(0.0f, 1.0f, std::log2(std::max(25.0f, cutoffHz) / 25.0f) / 9.2f);
                const float throatScale = 0.70f + width * 0.65f;
                const float f1 = (290.0f + 510.0f * std::sin(morph * juce::MathConstants<float>::pi)) * throatScale;
                const float f2 = (720.0f + 1680.0f * std::pow(morph, 1.25f)) * throatScale;
                const float f3 = (2150.0f + 780.0f * morph) * throatScale;
                const float throatChest = 195.0f * throatScale;
                const float q = 0.13f + (1.0f - safeRes * 0.70f) * 0.18f;
                const float pChest = 0.55f * std::exp(-0.5f * std::pow(std::log2(freqHz / throatChest) / 0.35f, 2.0f));
                const float p1 = 1.15f * std::exp(-0.5f * std::pow(std::log2(freqHz / f1) / q, 2.0f));
                const float p2 = 1.10f * std::exp(-0.5f * std::pow(std::log2(freqHz / f2) / (q * 0.9f), 2.0f));
                const float p3 = 0.70f * std::exp(-0.5f * std::pow(std::log2(freqHz / f3) / (q * 1.1f), 2.0f));
                const float driverPresence = 1.0f + 0.45f * std::exp(-0.5f * std::pow(std::log2(freqHz / 1850.0f) / 0.85f, 2.0f));
                const float tubeRolloff = 1.0f / (1.0f + std::pow(freqHz / 5400.0f, 3.2f));
                gain = (0.06f + (1.15f + safeRes * 1.65f) * (pChest + p1 + p2 + p3)) * driverPresence * tubeRolloff;
                break;
            }
        }

        if (!std::isfinite(gain))
            gain = 1.0f;
        return juce::jlimit(0.0f, 2.4f, gain);
    }

    void noteOn(int midiNote, float velocity, const SynthParameters& params) noexcept
    {
        if (params.arpEnable)
        {
            arpHeldNotes.eraseIf([midiNote](const HeldNote& n) { return n.note == midiNote; });
            arpHeldNotes.push_back({ midiNote, velocity });
            std::sort(arpHeldNotes.begin(), arpHeldNotes.end(),
                [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

            if (arpHeldNotes.size() == 1)
            {
                arpStepIndex = 0;
                arpNoteIndex = 0;
                arpDirection = 1;
                arpPhase = 1.0f;
            }
            return;
        }
        triggerInternalNoteOn(midiNote, velocity, params);
    }

    void noteOff(int midiNote, const SynthParameters& params) noexcept
    {
        if (params.arpEnable)
        {
            arpHeldNotes.eraseIf([midiNote](const HeldNote& n) { return n.note == midiNote; });
            if (arpHeldNotes.empty())
                stopActiveArpNotes(params);
            return;
        }
        triggerInternalNoteOff(midiNote, params);
    }

    void allNotesOff() noexcept
    {
        heldNotes.clear();
        arpHeldNotes.clear();
        activeArpTriggeredNotes.clear();
        for (auto& v : *voices)
        {
            if (v.active)
                v.beginRelease();
        }
    }

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const SynthParameters& params)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        if (numSamples <= 0 || numChannels < 2)
            return;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);

        if (params.arpEnable != arpWasEnabled)
        {
            if (!params.arpEnable)
            {
                stopActiveArpNotes(params);
                arpHeldNotes.clear();
            }
            else
            {
                arpHeldNotes = heldNotes;
                std::sort(arpHeldNotes.begin(), arpHeldNotes.end(),
                    [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
                for (size_t i = 0; i < heldNotes.size(); ++i)
                    triggerInternalNoteOff(heldNotes[i].note, params);
                arpStepIndex = 0;
                arpNoteIndex = 0;
                arpDirection = 1;
                arpPhase = 1.0f;
            }
            arpWasEnabled = params.arpEnable;
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getWritePointer(1);

        smoothedDetune.setTargetValue(params.unisonDetune);
        smoothedSpread.setTargetValue(params.unisonSpread);
        smoothedBlend.setTargetValue(params.unisonBlend);
        smoothedUnisonVoices.setTargetValue(static_cast<float>(juce::jlimit(1, MAX_UNISON, params.unisonVoices)));

        int currentSample = 0;
        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            const int msgSample = juce::jlimit(0, numSamples, metadata.samplePosition);

            while (currentSample < msgSample)
            {
                const int chunk = std::min(CONTROL_RATE_SAMPLES, msgSample - currentSample);
                if (params.arpEnable)
                    advanceArpeggiator(chunk, params);
                renderChunk(outL + currentSample, outR + currentSample, chunk, params);
                currentSample += chunk;
            }

            if (msg.isNoteOn())
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), params);
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber(), params);
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allNotesOff();
        }

        while (currentSample < numSamples)
        {
            const int chunk = std::min(CONTROL_RATE_SAMPLES, numSamples - currentSample);
            if (params.arpEnable)
                advanceArpeggiator(chunk, params);
            renderChunk(outL + currentSample, outR + currentSample, chunk, params);
            currentSample += chunk;
        }

        // Pre-FX polyphonic headroom scaling (1 / sqrt(activeVoices))
        int activeVoicesCount = 0;
        for (const auto& v : *voices)
        {
            if (v.active)
                ++activeVoicesCount;
        }
        const int reportedVoices = activeVoicesCount;
        activeVoicesCount = std::max(1, activeVoicesCount);

        const float targetPreFxScale = 1.0f / std::sqrt(static_cast<float>(activeVoicesCount));
        smoothedPolyHeadroom.setTargetValue(targetPreFxScale);

        for (int i = 0; i < numSamples; ++i)
        {
            const float preFxGain = smoothedPolyHeadroom.getNextValue();
            outL[i] *= preFxGain;
            outR[i] *= preFxGain;
        }

        // Process Noise + 4x Oversampled Filter Drive + TimeMod + Dynamic 13-Slot FX Rack + Limiter
        const int chainCount = activeFxChainCount.load(std::memory_order_acquire);
        fxRack.process(outL, outR, numSamples, params, activeFxChainArray, chainCount,
                       latestVoiceEnvGain, pendingSnapshot.activePitchHz, reportedVoices,
                       timeModCurve, pendingSnapshot);

        pendingSnapshot.activeVoiceCount = reportedVoices;
        activeVoicesAtomic.store(reportedVoices, std::memory_order_relaxed);

        const int writeVis = 1 - activeVisIndex.load(std::memory_order_relaxed);
        visSnapshots[static_cast<size_t>(writeVis)] = pendingSnapshot;
        activeVisIndex.store(writeVis, std::memory_order_release);
    }

private:
    struct HeldNote
    {
        int note = 60;
        float velocity = 0.8f;
    };

    void triggerInternalNoteOn(int midiNote, float velocity, const SynthParameters& params) noexcept
    {
        midiNote = juce::jlimit(0, 127, midiNote);
        const float targetFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        const int effectiveMaxVoices = juce::jlimit(1, MAX_VOICES,
            (params.voicingMode == 0) ? params.maxVoices : 1);

        const bool hadHeldNotes = !heldNotes.empty();

        heldNotes.eraseIf([midiNote](const HeldNote& n) { return n.note == midiNote; });
        heldNotes.push_back({ midiNote, velocity });

        const bool shouldGlide = (params.portaTime > 0.005f) && (!params.autoGlide || hadHeldNotes);
        const float startFreq = (shouldGlide && lastPlayedFreq > 20.0f) ? lastPlayedFreq : targetFreq;
        lastPlayedFreq = targetFreq;

        if (params.voicingMode == 1 || params.voicingMode == 2)
        {
            for (int i = 1; i < MAX_VOICES; ++i)
                (*voices)[static_cast<size_t>(i)].reset();

            auto& monoVoice = (*voices)[0];
            const bool isLegatoTransition = (params.voicingMode == 2) && hadHeldNotes && monoVoice.active && !monoVoice.releasing;

            if (isLegatoTransition)
                monoVoice.triggerLegatoNote(midiNote, velocity, targetFreq, params);
            else
                monoVoice.startNote(midiNote, velocity, startFreq, targetFreq, params, rng);
            return;
        }

        for (int i = 0; i < effectiveMaxVoices; ++i)
        {
            auto& v = (*voices)[static_cast<size_t>(i)];
            if (v.active && v.midiNote == midiNote)
            {
                v.startNote(midiNote, velocity, startFreq, targetFreq, params, rng);
                return;
            }
        }

        Voice* chosen = nullptr;

        for (int i = 0; i < effectiveMaxVoices; ++i)
        {
            auto& v = (*voices)[static_cast<size_t>(i)];
            if (!v.active) { chosen = &v; break; }
        }

        if (chosen == nullptr)
        {
            float lowestRelGain = 999.0f;
            for (int i = 0; i < effectiveMaxVoices; ++i)
            {
                auto& v = (*voices)[static_cast<size_t>(i)];
                if (v.releasing && v.curGain < lowestRelGain)
                {
                    lowestRelGain = v.curGain;
                    chosen = &v;
                }
            }
        }

        if (chosen == nullptr)
        {
            double oldestHeldAge = -1.0;
            for (int i = 0; i < effectiveMaxVoices; ++i)
            {
                auto& v = (*voices)[static_cast<size_t>(i)];
                if (v.noteAgeSeconds > oldestHeldAge)
                {
                    oldestHeldAge = v.noteAgeSeconds;
                    chosen = &v;
                }
            }
        }

        for (int i = effectiveMaxVoices; i < MAX_VOICES; ++i)
        {
            auto& v = (*voices)[static_cast<size_t>(i)];
            if (v.curGain < 0.001f)
                v.reset();
        }

        if (chosen == nullptr)
            chosen = &(*voices)[0];

        chosen->startNote(midiNote, velocity, startFreq, targetFreq, params, rng);
    }

    void triggerInternalNoteOff(int midiNote, const SynthParameters& params) noexcept
    {
        heldNotes.eraseIf([midiNote](const HeldNote& n) { return n.note == midiNote; });

        if ((params.voicingMode == 1 || params.voicingMode == 2) && !heldNotes.empty())
        {
            const auto& prevHeld = heldNotes.back();
            auto& monoVoice = (*voices)[0];
            if (monoVoice.active && !monoVoice.releasing)
            {
                const float prevFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(prevHeld.note));
                monoVoice.midiNote = prevHeld.note;
                monoVoice.velocity = prevHeld.velocity;
                monoVoice.glideStartFreq = monoVoice.currentBaseFreq;
                monoVoice.targetBaseFreq = prevFreq;
                monoVoice.glideProgress = (params.portaTime > 0.005f) ? 0.0f : 1.0f;
                if (monoVoice.glideProgress >= 1.0f)
                    monoVoice.currentBaseFreq = prevFreq;
                lastPlayedFreq = prevFreq;
                return;
            }
        }

        for (auto& v : *voices)
        {
            if (v.active && v.midiNote == midiNote)
                v.beginRelease();
        }
    }

    void stopActiveArpNotes(const SynthParameters& params) noexcept
    {
        for (size_t i = 0; i < activeArpTriggeredNotes.size(); ++i)
            triggerInternalNoteOff(activeArpTriggeredNotes[i], params);
        activeArpTriggeredNotes.clear();
    }

    void advanceArpeggiator(int numSamples, const SynthParameters& params) noexcept
    {
        if (arpHeldNotes.empty())
            return;

        const float safeBpm = static_cast<float>(juce::jlimit(30.0, 300.0, params.hostBpm));
        const float beatsPerSec = safeBpm / 60.0f;
        float stepsPerSec = beatsPerSec * 4.0f;
        switch (juce::jlimit(0, 5, params.arpRate))
        {
            case 0: stepsPerSec = beatsPerSec * 1.0f; break;
            case 1: stepsPerSec = beatsPerSec * 2.0f; break;
            case 2: stepsPerSec = beatsPerSec * 3.0f; break;
            case 3: stepsPerSec = beatsPerSec * 4.0f; break;
            case 4: stepsPerSec = beatsPerSec * 6.0f; break;
            case 5: stepsPerSec = beatsPerSec * 8.0f; break;
            default: break;
        }

        const float swing = juce::jlimit(0.0f, 0.75f, params.arpSwing);
        const float swingMult = ((arpStepIndex & 1) == 0) ? (1.0f + swing * 0.45f) : (1.0f - swing * 0.45f);
        const float effectiveStepHz = stepsPerSec / std::max(0.25f, swingMult);

        const float dt = static_cast<float>(numSamples) * invSampleRate;
        arpPhase += dt * effectiveStepHz;

        const float gateLen = juce::jlimit(0.08f, 0.99f, params.arpGate);
        if (arpPhase >= gateLen && !activeArpTriggeredNotes.empty() && arpPhase < 1.0f)
            stopActiveArpNotes(params);

        if (arpPhase >= 1.0f)
        {
            arpPhase -= std::floor(arpPhase);
            stopActiveArpNotes(params);

            const int curStep = arpStepIndex % NUM_ARP_STEPS;
            currentArpStepAtomic.store(curStep, std::memory_order_relaxed);
            const ArpStep step = getArpStep(curStep);
            arpStepIndex = (arpStepIndex + 1) % NUM_ARP_STEPS;

            if (!step.enabled)
                return;

            const int numHeld = static_cast<int>(arpHeldNotes.size());
            const int octRange = juce::jlimit(1, 4, params.arpOctaves);
            const int totalPool = std::max(1, numHeld * octRange);

            if (params.arpMode == 4)
            {
                for (size_t i = 0; i < arpHeldNotes.size(); ++i)
                {
                    const auto& hn = arpHeldNotes[i];
                    const int noteToPlay = juce::jlimit(12, 115, hn.note + step.octaveOffset * 12 + step.semitoneTranspose);
                    const float vel = juce::jlimit(0.05f, 1.0f, hn.velocity * step.velocity);
                    triggerInternalNoteOn(noteToPlay, vel, params);
                    activeArpTriggeredNotes.push_back(noteToPlay);
                }
            }
            else
            {
                int poolIdx = 0;
                switch (params.arpMode)
                {
                    case 0:
                        poolIdx = arpNoteIndex % totalPool;
                        arpNoteIndex = (arpNoteIndex + 1) % totalPool;
                        break;
                    case 1:
                        poolIdx = (totalPool - 1) - (arpNoteIndex % totalPool);
                        arpNoteIndex = (arpNoteIndex + 1) % totalPool;
                        break;
                    case 2:
                        if (totalPool <= 1) { poolIdx = 0; }
                        else
                        {
                            arpNoteIndex = juce::jlimit(0, totalPool - 1, arpNoteIndex);
                            poolIdx = arpNoteIndex;
                            arpNoteIndex += arpDirection;
                            if (arpNoteIndex >= totalPool) { arpNoteIndex = std::max(0, totalPool - 2); arpDirection = -1; }
                            else if (arpNoteIndex < 0)     { arpNoteIndex = std::min(totalPool - 1, 1); arpDirection = 1; }
                        }
                        break;
                    case 3:
                        poolIdx = rng.nextInt(totalPool);
                        break;
                    case 5:
                    default:
                        poolIdx = curStep % totalPool;
                        break;
                }

                const int baseIdx = poolIdx % numHeld;
                const int octIdx  = (params.arpMode == 5) ? 0 : (poolIdx / numHeld);
                const auto& baseNote = arpHeldNotes[static_cast<size_t>(baseIdx)];
                const int finalNote = juce::jlimit(12, 115, baseNote.note + (octIdx + step.octaveOffset) * 12 + step.semitoneTranspose);
                const float finalVel = juce::jlimit(0.05f, 1.0f, baseNote.velocity * step.velocity);

                triggerInternalNoteOn(finalNote, finalVel, params);
                activeArpTriggeredNotes.push_back(finalNote);
            }
        }
    }

    struct Voice
    {
        enum class EnvStage : int { Off = 0, Attack, Hold, Decay, Sustain, Release };

        bool     active = false;
        bool     releasing = false;
        int      midiNote = 60;
        float    velocity = 0.8f;
        float    glideStartFreq = 130.81f;
        float    currentBaseFreq = 130.81f;
        float    targetBaseFreq = 130.81f;
        float    glideProgress = 1.0f;
        double   noteAgeSeconds = 0.0;
        float    vibPhase = 0.0f;

        EnvStage envStage = EnvStage::Off;
        float    ahdsrLevel = 0.0f;
        float    envStageTime = 0.0f;

        std::array<float, static_cast<size_t>(ArticulatorTarget::Count)> artPhase {};
        float curVolEnv = 1.0f;
        float targetVolEnv = 1.0f;

        // Dual-path oscillator state: Fast wavetable (Path A) + Inharmonic per-partial additive (Path B)
        static constexpr int WAVE_TABLE_SIZE = 1024;
        std::array<float, WAVE_TABLE_SIZE> waveTableA {};
        std::array<float, WAVE_TABLE_SIZE> waveTableB {};
        std::array<float, WAVE_TABLE_SIZE> prevWaveTableA {};
        std::array<float, WAVE_TABLE_SIZE> prevWaveTableB {};
        bool hasPrevTable = false;

        bool useAdditiveA = false, wasAdditiveA = false;
        bool useAdditiveB = false, wasAdditiveB = false;

        std::array<uint32_t, MAX_UNISON> subPhaseA {};
        std::array<uint32_t, MAX_UNISON> subPhaseB {};
        std::array<uint32_t, MAX_UNISON> subPhaseIncA {};
        std::array<uint32_t, MAX_UNISON> subPhaseIncB {};

        // Per-unison, per-partial additive phase accumulators (48 partials x 16 unison = 3 KB per voice)
        std::array<std::array<uint32_t, ADDITIVE_PARTIALS>, MAX_UNISON> addPhaseA {};
        std::array<std::array<uint32_t, ADDITIVE_PARTIALS>, MAX_UNISON> addPhaseB {};
        std::array<float, MAX_UNISON> unisonPitchRatio {};
        std::array<float, ADDITIVE_PARTIALS> prevAddAmpA {};
        std::array<float, ADDITIVE_PARTIALS> prevAddAmpB {};

        std::array<float, MAX_UNISON> uWeightL {};
        std::array<float, MAX_UNISON> uWeightR {};
        std::array<float, MAX_UNISON> curUniVoiceWeight {};
        std::array<float, MAX_UNISON> targetUniVoiceWeight {};

        std::array<float, MAX_UNISON> initNormPhaseA {};
        std::array<float, MAX_UNISON> initNormPhaseB {};

        float curGain = 0.0f, targetGain = 0.0f;

        std::array<float, NUM_PARTIALS> curAmpA {};
        std::array<float, NUM_PARTIALS> curAmpB {};
        std::array<float, NUM_PARTIALS> targetAmpA {};
        std::array<float, NUM_PARTIALS> targetAmpB {};

        std::array<float, NUM_PARTIALS> blurStateA {};
        std::array<float, NUM_PARTIALS> blurStateB {};
        std::array<float, NUM_PARTIALS> pluckState {};

        std::array<float, NUM_PARTIALS> freqHzA {};
        std::array<float, NUM_PARTIALS> freqHzB {};
        std::array<float, NUM_PARTIALS> filterGainTable {};

        void beginRelease() noexcept
        {
            if (active && !releasing)
            {
                releasing = true;
                envStage = EnvStage::Release;
                envStageTime = 0.0f;
            }
        }

        void reset() noexcept
        {
            active = false;
            releasing = false;
            envStage = EnvStage::Off;
            ahdsrLevel = 0.0f;
            envStageTime = 0.0f;
            noteAgeSeconds = 0.0;
            vibPhase = 0.0f;
            artPhase.fill(0.0f);
            curAmpA.fill(0.0f);
            curAmpB.fill(0.0f);
            targetAmpA.fill(0.0f);
            targetAmpB.fill(0.0f);
            prevAddAmpA.fill(0.0f);
            prevAddAmpB.fill(0.0f);
            blurStateA.fill(0.0f);
            blurStateB.fill(0.0f);
            pluckState.fill(1.0f);

            subPhaseA.fill(0u);
            subPhaseB.fill(0u);
            subPhaseIncA.fill(0u);
            subPhaseIncB.fill(0u);
            unisonPitchRatio.fill(1.0f);
            uWeightL.fill(0.0f);
            uWeightR.fill(0.0f);
            curUniVoiceWeight.fill(0.0f);
            targetUniVoiceWeight.fill(0.0f);
            waveTableA.fill(0.0f);
            waveTableB.fill(0.0f);
            prevWaveTableA.fill(0.0f);
            prevWaveTableB.fill(0.0f);
            hasPrevTable = false;
            useAdditiveA = wasAdditiveA = false;
            useAdditiveB = wasAdditiveB = false;
            curGain = 0.0f;
            targetGain = 0.0f;
        }

        void startNote(int note, float vel, float startFreq, float endFreq,
                       const SynthParameters& params, juce::Random& randomGen) noexcept
        {
            active = true;
            releasing = false;
            midiNote = note;
            velocity = vel;
            glideStartFreq = startFreq;
            currentBaseFreq = startFreq;
            targetBaseFreq = endFreq;
            glideProgress = (std::abs(endFreq - startFreq) > 0.01f && params.portaTime > 0.005f) ? 0.0f : 1.0f;
            noteAgeSeconds = 0.0;
            vibPhase = 0.0f;

            envStage = EnvStage::Attack;
            ahdsrLevel = 0.0f;
            envStageTime = 0.0f;
            artPhase.fill(0.0f);
            curVolEnv = 1.0f;
            targetVolEnv = 1.0f;

            const int initVoices = juce::jlimit(1, MAX_UNISON, params.unisonVoices);
            const float randUnisonPh = juce::jlimit(0.0f, 1.0f, params.unisonPhase);
            const float basePhA      = juce::jlimit(0.0f, 1.0f, params.startPhase);
            const float basePhB      = juce::jlimit(0.0f, 1.0f, params.oscBPhase);
            const float globalRndPh  = juce::jlimit(0.0f, 1.0f, params.phaseRandom);

            for (int u = 0; u < MAX_UNISON; ++u)
            {
                const float w = (u < initVoices) ? 1.0f : 0.0f;
                curUniVoiceWeight[static_cast<size_t>(u)] = w;
                targetUniVoiceWeight[static_cast<size_t>(u)] = w;

                const float uDetSpread = (u == 0) ? 0.0f : (static_cast<float>(u) / static_cast<float>(MAX_UNISON)) * randUnisonPh;
                const float rndA = randomGen.nextFloat() * globalRndPh;
                const float rndB = randomGen.nextFloat() * globalRndPh;
                const float uJitterA = (u == 0) ? 0.0f : randomGen.nextFloat() * 0.5f * randUnisonPh;
                const float uJitterB = (u == 0) ? 0.0f : randomGen.nextFloat() * 0.5f * randUnisonPh;

                float phNormA = basePhA + uDetSpread * 0.5f + uJitterA + rndA;
                float phNormB = basePhB + uDetSpread * 0.5f + uJitterB + rndB;
                phNormA -= std::floor(phNormA);
                phNormB -= std::floor(phNormB);

                initNormPhaseA[static_cast<size_t>(u)] = phNormA;
                initNormPhaseB[static_cast<size_t>(u)] = phNormB;

                subPhaseA[static_cast<size_t>(u)] = static_cast<uint32_t>(phNormA * 4294967295.0f);
                subPhaseB[static_cast<size_t>(u)] = static_cast<uint32_t>(phNormB * 4294967295.0f);
            }

            for (int k = 0; k < NUM_PARTIALS; ++k)
            {
                const size_t idx = static_cast<size_t>(k);
                curAmpA[idx] = 0.0f;
                curAmpB[idx] = 0.0f;
                blurStateA[idx] = 0.0f;
                blurStateB[idx] = 0.0f;
                pluckState[idx] = 1.0f;
            }
            prevAddAmpA.fill(0.0f);
            prevAddAmpB.fill(0.0f);

            waveTableA.fill(0.0f);
            waveTableB.fill(0.0f);
            prevWaveTableA.fill(0.0f);
            prevWaveTableB.fill(0.0f);
            hasPrevTable = false;
            useAdditiveA = wasAdditiveA = false;
            useAdditiveB = wasAdditiveB = false;
            curGain = 0.0f;
            targetGain = 0.0f;
        }

        void triggerLegatoNote(int note, float vel, float newTargetFreq, const SynthParameters& params) noexcept
        {
            midiNote = note;
            velocity = vel;
            glideStartFreq = currentBaseFreq;
            targetBaseFreq = newTargetFreq;
            glideProgress = (params.portaTime > 0.005f) ? 0.0f : 1.0f;
            if (glideProgress >= 1.0f)
                currentBaseFreq = newTargetFreq;
            releasing = false;
            if (envStage == EnvStage::Release || envStage == EnvStage::Off)
                envStage = (ahdsrLevel >= 0.99f) ? EnvStage::Sustain : EnvStage::Attack;
        }
    };

    void initSineLUT()
    {
        for (int i = 0; i < SINE_LUT_SIZE; ++i)
        {
            const double angle = (static_cast<double>(i) / static_cast<double>(SINE_LUT_SIZE)) * juce::MathConstants<double>::twoPi;
            sineLUT[static_cast<size_t>(i)] = static_cast<float>(std::sin(angle));
        }
    }

    inline float fastSin(float rad) const noexcept
    {
        const uint32_t ph = static_cast<uint32_t>(static_cast<int64_t>(rad * 683565275.5764316));
        return sineLUT[ph >> 19];
    }

    static void applyHarmonicWarp(int warpMode, float warpAmount, float normK,
                                  float& outSourceNormK, float& outFreqWarpMult) noexcept
    {
        outSourceNormK = normK;
        outFreqWarpMult = 1.0f;
        if (warpMode == 0 || std::abs(warpAmount) < 0.005f)
            return;

        const float w = juce::jlimit(-1.0f, 1.0f, warpAmount);
        switch (warpMode)
        {
            case 1: // Bend +/-
                outSourceNormK = SplineCurve::applyTensionCurve(normK, w * 0.85f);
                outFreqWarpMult = 1.0f + w * 0.12f * (normK - 0.5f);
                break;
            case 2: // Mirror
            {
                const float pivot = juce::jlimit(0.15f, 0.85f, 0.5f + w * 0.35f);
                outSourceNormK = (normK <= pivot) ? (normK / pivot) : (1.0f - (normK - pivot) / (1.0f - pivot));
                break;
            }
            case 3: // Quantize
            {
                const float steps = std::max(2.0f, 24.0f * (1.0f - std::abs(w) * 0.85f));
                outSourceNormK = std::round(normK * steps) / steps;
                if (w > 0.0f)
                    outFreqWarpMult = 1.0f + 0.08f * w * std::sin(normK * steps * 3.14159f);
                break;
            }
            case 4: // Asymmetric
            {
                const float skew = std::pow(normK, std::pow(2.0f, -w * 1.6f));
                outSourceNormK = juce::jlimit(0.0f, 1.0f, skew);
                outFreqWarpMult = 1.0f + 0.06f * w * std::cos(normK * 6.28318f);
                break;
            }
            case 5: // Sync Fold
            {
                const float syncRatio = 1.0f + std::abs(w) * 3.5f;
                outSourceNormK = std::fmod(normK * syncRatio, 1.0f);
                break;
            }
            default:
                break;
        }
    }

    void syncAdditivePhasesFromWavetable(Voice& v, bool syncA, bool syncB) const noexcept
    {
        for (int u = 0; u < MAX_UNISON; ++u)
        {
            const size_t uIdx = static_cast<size_t>(u);
            if (syncA)
            {
                const float uPh = static_cast<float>(v.subPhaseA[uIdx]) * (1.0f / 4294967296.0f);
                for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
                {
                    const float pOff = resynthEngine.getHarmonicPhaseRaw(0, k);
                    float ph = uPh * static_cast<float>(k + 1) + pOff;
                    ph -= std::floor(ph);
                    v.addPhaseA[uIdx][static_cast<size_t>(k)] = static_cast<uint32_t>(ph * 4294967295.0f);
                }
            }
            if (syncB)
            {
                const float uPh = static_cast<float>(v.subPhaseB[uIdx]) * (1.0f / 4294967296.0f);
                for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
                {
                    const float pOff = resynthEngine.getHarmonicPhaseRaw(1, k) + 0.25f;
                    float ph = uPh * static_cast<float>(k + 1) + pOff;
                    ph -= std::floor(ph);
                    v.addPhaseB[uIdx][static_cast<size_t>(k)] = static_cast<uint32_t>(ph * 4294967295.0f);
                }
            }
        }
    }

    void syncWavetablePhasesFromAdditive(Voice& v, bool syncA, bool syncB) const noexcept
    {
        for (int u = 0; u < MAX_UNISON; ++u)
        {
            const size_t uIdx = static_cast<size_t>(u);
            if (syncA)
            {
                const float ph0 = static_cast<float>(v.addPhaseA[uIdx][0]) * (1.0f / 4294967296.0f);
                float uPh = ph0 - resynthEngine.getHarmonicPhaseRaw(0, 0);
                uPh -= std::floor(uPh);
                v.subPhaseA[uIdx] = static_cast<uint32_t>(uPh * 4294967295.0f);
            }
            if (syncB)
            {
                const float ph0 = static_cast<float>(v.addPhaseB[uIdx][0]) * (1.0f / 4294967296.0f);
                float uPh = ph0 - (resynthEngine.getHarmonicPhaseRaw(1, 0) + 0.25f);
                uPh -= std::floor(uPh);
                v.subPhaseB[uIdx] = static_cast<uint32_t>(uPh * 4294967295.0f);
            }
        }
    }

    void updateVoiceSpectralTables(Voice& v, float dtSeconds, const SynthParameters& params,
                                   float uDetune, float uSpread, float uBlend, float uVoicesCount,
                                   float mixA, float mixB, bool blurActiveInChain, int releasingCount)
    {
        v.noteAgeSeconds += static_cast<double>(dtSeconds);

        // 1. Advance All 6 Shaper Articulators
        for (size_t t = 0; t < static_cast<size_t>(ArticulatorTarget::Count); ++t)
        {
            const auto& art = articulators[t];
            const float effectiveHz = art.getEffectiveRateHz(params.hostBpm);
            v.artPhase[t] += dtSeconds * effectiveHz;
            if (art.getMode() == ArticulatorMode::Envelope)
                v.artPhase[t] = std::min(1.0f, v.artPhase[t]);
            else
                v.artPhase[t] -= std::floor(v.artPhase[t]);
        }

        // 2. Per-Voice True AHDSR Amplitude Envelope
        const float attackSec  = 0.001f + std::pow(juce::jlimit(0.0f, 1.0f, params.envAttack),  2.0f) * 2.5f;
        const float holdSec    = std::pow(juce::jlimit(0.0f, 1.0f, params.envHold),    2.0f) * 1.5f;
        const float decaySec   = 0.010f + std::pow(juce::jlimit(0.0f, 1.0f, params.envDecay),   2.0f) * 3.5f;
        const float sustainLvl = juce::jlimit(0.0f, 1.0f, params.envSustain);
        const float blurTailS  = (blurActiveInChain && params.blurEnable) ? (params.blurTime * 1.8f) : 0.0f;
        const float releaseSec = 0.012f + std::pow(juce::jlimit(0.01f, 1.0f, params.envRelease), 1.8f) * 2.8f + blurTailS;

        switch (v.envStage)
        {
            case Voice::EnvStage::Attack:
                v.ahdsrLevel += dtSeconds / attackSec;
                if (v.ahdsrLevel >= 1.0f)
                {
                    v.ahdsrLevel = 1.0f;
                    v.envStageTime = 0.0f;
                    v.envStage = (holdSec > 0.0005f) ? Voice::EnvStage::Hold : Voice::EnvStage::Decay;
                }
                break;

            case Voice::EnvStage::Hold:
                v.ahdsrLevel = 1.0f;
                v.envStageTime += dtSeconds;
                if (v.envStageTime >= holdSec)
                    v.envStage = Voice::EnvStage::Decay;
                break;

            case Voice::EnvStage::Decay:
            {
                const float decCoeff = 1.0f - std::exp(-dtSeconds * 4.6f / decaySec);
                v.ahdsrLevel += decCoeff * (sustainLvl - v.ahdsrLevel);
                if (std::abs(v.ahdsrLevel - sustainLvl) < 0.001f)
                {
                    v.ahdsrLevel = sustainLvl;
                    v.envStage = Voice::EnvStage::Sustain;
                }
                break;
            }

            case Voice::EnvStage::Sustain:
                v.ahdsrLevel = sustainLvl;
                break;

            case Voice::EnvStage::Release:
            {
                const float relSpeedup = (releasingCount > 4) ? (1.0f + static_cast<float>(releasingCount - 4) * 0.55f) : 1.0f;
                const float relCoeff = 1.0f - std::exp((-dtSeconds * 4.6f / releaseSec) * relSpeedup);
                v.ahdsrLevel *= (1.0f - relCoeff);
                if (v.ahdsrLevel <= 0.0002f)
                {
                    v.reset();
                    return;
                }
                break;
            }

            case Voice::EnvStage::Off:
            default:
                v.reset();
                return;
        }

        const float volArtMod    = articulators[static_cast<size_t>(ArticulatorTarget::Volume)].evaluateLUT(
            v.artPhase[static_cast<size_t>(ArticulatorTarget::Volume)]);
        const float pitchArtMod  = (articulators[static_cast<size_t>(ArticulatorTarget::Pitch)].evaluateLUT(
            v.artPhase[static_cast<size_t>(ArticulatorTarget::Pitch)]) - 0.5f) * 2.0f;
        const float filt1ArtMod  = (articulators[static_cast<size_t>(ArticulatorTarget::Filter1Cutoff)].evaluateLUT(
            v.artPhase[static_cast<size_t>(ArticulatorTarget::Filter1Cutoff)]) - 0.5f) * 2.0f;
        const float timbreArtMod = (articulators[static_cast<size_t>(ArticulatorTarget::Timbre)].evaluateLUT(
            v.artPhase[static_cast<size_t>(ArticulatorTarget::Timbre)]) - 0.5f) * 0.7f;
        const float prismArtMod  = (articulators[static_cast<size_t>(ArticulatorTarget::Prism)].evaluateLUT(
            v.artPhase[static_cast<size_t>(ArticulatorTarget::Prism)]) - 0.5f) * 0.6f;
        const auto& customFilterCurve = articulators[static_cast<size_t>(ArticulatorTarget::CustomFilterShape)];

        v.curVolEnv = v.targetVolEnv;
        v.targetVolEnv = v.ahdsrLevel * volArtMod;
        if (v.noteAgeSeconds <= static_cast<double>(dtSeconds * 1.5f))
            v.curVolEnv = v.targetVolEnv;

        // Glide / Portamento
        if (v.glideProgress < 1.0f)
        {
            const float glideDuration = std::max(0.004f, params.portaTime * params.portaTime * 1.4f);
            v.glideProgress = std::min(1.0f, v.glideProgress + dtSeconds / glideDuration);
            const float shapedT = SplineCurve::applyTensionCurve(v.glideProgress, params.portaCurve);
            const float logStart = std::log2(std::max(20.0f, v.glideStartFreq));
            const float logEnd   = std::log2(std::max(20.0f, v.targetBaseFreq));
            v.currentBaseFreq = std::pow(2.0f, logStart + shapedT * (logEnd - logStart));
        }
        else
        {
            v.currentBaseFreq = v.targetBaseFreq;
        }

        // Independent Vibrato Phase Accumulator (Click-free automation!)
        v.vibPhase += dtSeconds * params.vibSpeed * juce::MathConstants<float>::twoPi;
        if (v.vibPhase >= juce::MathConstants<float>::twoPi)
            v.vibPhase -= std::floor(v.vibPhase / juce::MathConstants<float>::twoPi) * juce::MathConstants<float>::twoPi;
        const float vibLfo = fastSin(v.vibPhase);

        const float modSemis = params.pitchFreq + (vibLfo * params.vibDepth * 1.2f) + (pitchArtMod * 12.0f);
        const float field1PitchSt = static_cast<float>(params.oscAOct * 12 + params.oscASemi)
                                  + params.oscAFine * 0.01f + params.oscACoarse;
        const float field2PitchSt = static_cast<float>(params.oscBOct * 12 + params.oscBSemi)
                                  + params.oscBFine * 0.01f + params.oscBPitch;

        const float nyquist = static_cast<float>(sampleRate * 0.47);
        const float safeBaseFreq = juce::jlimit(10.0f, nyquist, v.currentBaseFreq);
        const float f0A = juce::jlimit(10.0f, nyquist, safeBaseFreq * std::pow(2.0f, juce::jlimit(-48.0f, 48.0f, modSemis + field1PitchSt) / 12.0f));
        const float f0B = juce::jlimit(10.0f, nyquist, safeBaseFreq * std::pow(2.0f, juce::jlimit(-48.0f, 48.0f, modSemis + field2PitchSt) / 12.0f));

        const float effTimbreA = juce::jlimit(0.0f, 1.0f, params.timbre  + timbreArtMod);
        const float effTimbreB = juce::jlimit(0.0f, 1.0f, params.timbreB + timbreArtMod);
        const float effPrism   = params.prismEnable ? juce::jlimit(0.0f, 1.0f, params.prismAmount + prismArtMod) : 0.0f;

        const float resynthScanPosA = std::fmod(static_cast<float>(v.noteAgeSeconds) * 0.35f + effTimbreA * 0.65f, 1.0f);
        const float resynthScanPosB = std::fmod(static_cast<float>(v.noteAgeSeconds) * 0.35f + effTimbreB * 0.65f + 0.25f, 1.0f);
        const float harmScaleA = juce::jlimit(0.25f, 4.0f, params.harmonicScale);
        const float harmScaleB = juce::jlimit(0.25f, 4.0f, params.harmonicScaleB);

        bool needAdditiveA = (effPrism > 0.001f) || (std::abs(harmScaleA - 1.0f) > 0.005f);
        bool needAdditiveB = (effPrism > 0.001f) || (std::abs(harmScaleB - 1.0f) > 0.005f);

        std::array<float, NUM_PARTIALS> rawAmpA {};
        std::array<float, NUM_PARTIALS> rawAmpB {};

        for (int k = 0; k < NUM_PARTIALS; ++k)
        {
            const size_t idx = static_cast<size_t>(k);
            const float h = static_cast<float>(k + 1);
            const float normK = static_cast<float>(k) / static_cast<float>(NUM_PARTIALS - 1);

            float warpedNormKA = normK, freqWarpMultA = 1.0f;
            float warpedNormKB = normK, freqWarpMultB = 1.0f;
            applyHarmonicWarp(params.warpMode,  params.warpAmount,  normK, warpedNormKA, freqWarpMultA);
            applyHarmonicWarp(params.warpModeB, params.warpAmountB, normK, warpedNormKB, freqWarpMultB);

            const int srcKA = juce::jlimit(0, NUM_PARTIALS - 1, static_cast<int>(std::round(warpedNormKA * static_cast<float>(NUM_PARTIALS - 1))));
            const int srcKB = juce::jlimit(0, NUM_PARTIALS - 1, static_cast<int>(std::round(warpedNormKB * static_cast<float>(NUM_PARTIALS - 1))));

            float resAmpA = 0.0f, resFreqOffA = 0.0f;
            float resAmpB = 0.0f, resFreqOffB = 0.0f;
            resynthEngine.samplePartial(resynthScanPosA, srcKA, resAmpA, resFreqOffA);
            resynthEngine.samplePartial(resynthScanPosB, srcKB, resAmpB, resFreqOffB);

            const float scaledRatioA = 1.0f + (h - 1.0f) * harmScaleA * freqWarpMultA;
            const float scaledRatioB = 1.0f + (h - 1.0f) * harmScaleB * freqWarpMultB;
            float ratioA = scaledRatioA + resFreqOffA * effTimbreA;
            float ratioB = scaledRatioB - resFreqOffB * effTimbreB;
            float hzOffsetA = 0.0f;
            float hzOffsetB = 0.0f;

            if (effPrism > 0.001f)
            {
                if (params.prismMode == 0)
                {
                    ratioA *= std::sqrt(1.0f + effPrism * 0.015f * h * h);
                    ratioB *= std::sqrt(1.0f + effPrism * 0.020f * h * h);
                }
                else if (params.prismMode == 1)
                {
                    hzOffsetA = effPrism * 260.0f * (k > 0 ? 1.0f : 0.0f);
                    hzOffsetB = -effPrism * 210.0f * (k > 0 ? 1.0f : 0.0f);
                }
                else if (params.prismMode == 2)
                {
                    const float sign = (k % 2 == 0) ? 1.0f : -1.0f;
                    ratioA += sign * effPrism * 0.50f * std::sqrt(h);
                    ratioB -= sign * effPrism * 0.50f * std::sqrt(h);
                }
                else
                {
                    ratioA += effPrism * 0.65f * std::sin(h * 1.732f);
                    ratioB += effPrism * 0.65f * std::cos(h * 1.732f);
                }
            }

            if (std::abs(freqWarpMultA - 1.0f) > 0.002f || std::abs(resFreqOffA * effTimbreA) > 0.002f)
                needAdditiveA = true;
            if (std::abs(freqWarpMultB - 1.0f) > 0.002f || std::abs(resFreqOffB * effTimbreB) > 0.002f)
                needAdditiveB = true;

            const float fkA = std::max(20.0f, f0A * ratioA + hzOffsetA);
            const float fkB = std::max(20.0f, f0B * ratioB + hzOffsetB);
            v.freqHzA[idx] = fkA;
            v.freqHzB[idx] = fkB;

            const bool overA = (fkA >= nyquist);
            const bool overB = (fkB >= nyquist);
            if (overA && overB)
            {
                rawAmpA[idx] = 0.0f;
                rawAmpB[idx] = 0.0f;
                continue;
            }

            const float tableGainA = (!overA) ? resynthEngine.getHarmonicGainRaw(0, srcKA) : 0.0f;
            const float tableGainB = (!overB) ? resynthEngine.getHarmonicGainRaw(1, srcKB) : 0.0f;

            float baseA = (!overA) ? (tableGainA * (1.0f - effTimbreA) + resAmpA * effTimbreA) : 0.0f;
            float baseB = (!overB) ? (tableGainB * (1.0f - effTimbreB) + resAmpB * effTimbreB) : 0.0f;

            if (std::abs(params.harmonicTilt) > 0.01f)
            {
                const float tiltDb = std::log2(h / 4.0f) * params.harmonicTilt * 6.5f;
                baseA *= juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, tiltDb));
            }
            if (std::abs(params.harmonicTiltB) > 0.01f)
            {
                const float tiltDbB = std::log2(h / 4.0f) * params.harmonicTiltB * 6.5f;
                baseB *= juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, tiltDbB));
            }

            if (params.strumTime > 0.01f)
            {
                const float curvedK = SplineCurve::applyTensionCurve(normK, params.strumTension);
                const float onsetTime = curvedK * params.strumTime * 0.55f;
                const float strumGate = juce::jlimit(0.0f, 1.0f, (static_cast<float>(v.noteAgeSeconds) - onsetTime) / 0.022f);
                baseA *= strumGate;
                baseB *= strumGate;
            }

            if (params.pluckEnable && params.pluckTime < 0.99f)
            {
                const float dampSlope = std::pow(h, 0.50f + (1.0f - params.pluckBlur) * 0.85f);
                const float decayTime = 0.025f + params.pluckTime * params.pluckTime * 2.6f;
                v.pluckState[idx] *= std::exp(-dtSeconds * (dampSlope / decayTime));
                baseA *= v.pluckState[idx];
                baseB *= v.pluckState[idx];
            }

            rawAmpA[idx] = baseA;
            rawAmpB[idx] = baseB;
        }

        // Harmonizer
        if (params.prismEnable && params.harmShift > 0.01f)
        {
            const int gapStep = std::max(1, static_cast<int>(1.0f + params.harmGap * 5.0f));
            const float widthDecay = 1.0f - params.harmWidth * 0.70f;
            const float shiftGain  = params.harmShift * 0.85f;
            for (int k = NUM_PARTIALS - 1; k >= gapStep; --k)
            {
                const int srcK = k / gapStep;
                if (srcK >= 0 && srcK < k)
                {
                    const float w = shiftGain * std::pow(widthDecay, static_cast<float>(k - srcK) * 0.08f);
                    rawAmpA[static_cast<size_t>(k)] += rawAmpA[static_cast<size_t>(srcK)] * w;
                    rawAmpB[static_cast<size_t>(k)] += rawAmpB[static_cast<size_t>(srcK)] * w;
                }
            }
        }

        // Harmonic Blur (if Blur active in chain)
        if (blurActiveInChain && params.blurEnable && params.blurHarmonic > 0.005f)
        {
            const int radius = std::max(1, static_cast<int>(1.0f + params.blurHarmonic * 10.0f));
            const float mix  = juce::jlimit(0.0f, 1.0f, params.blurHarmonic);
            const std::array<float, NUM_PARTIALS> tempA = rawAmpA;
            const std::array<float, NUM_PARTIALS> tempB = rawAmpB;

            for (int k = 0; k < NUM_PARTIALS; ++k)
            {
                float sumA = 0.0f, sumB = 0.0f, weightSum = 0.0f;
                const int jStart = std::max(0, k - radius);
                const int jEnd   = std::min(NUM_PARTIALS - 1, k + radius);
                for (int j = jStart; j <= jEnd; ++j)
                {
                    const float dist = static_cast<float>(std::abs(k - j)) / static_cast<float>(radius + 1);
                    const float w = 1.0f - dist * dist;
                    sumA += tempA[static_cast<size_t>(j)] * w;
                    sumB += tempB[static_cast<size_t>(j)] * w;
                    weightSum += w;
                }
                const float makeup = 1.0f + mix * 0.55f;
                rawAmpA[static_cast<size_t>(k)] = tempA[static_cast<size_t>(k)] * (1.0f - mix) + ((sumA / std::max(1.0f, weightSum)) * makeup) * mix;
                rawAmpB[static_cast<size_t>(k)] = tempB[static_cast<size_t>(k)] * (1.0f - mix) + ((sumB / std::max(1.0f, weightSum)) * makeup) * mix;
            }
        }

        // Dual Spectral Filters + Time Blur (NOTE: Tremolo, Phaser, and ParametricEQ run strictly inside FxRack!)
        const float keyOctaveOffset = std::log2(std::max(20.0f, f0A) / 261.63f);
        const float f1CutNorm = juce::jlimit(0.0f, 1.0f,
            params.filter1Cutoff + filt1ArtMod * 0.45f + keyOctaveOffset * params.filter1KeyTrack * 0.12f);
        const float f2CutNorm = juce::jlimit(0.0f, 1.0f,
            params.filter2Cutoff + filt1ArtMod * 0.30f + keyOctaveOffset * params.filter2KeyTrack * 0.12f);
        const float f1CutoffHz = 25.0f * std::pow(750.0f, f1CutNorm);
        const float f2CutoffHz = 25.0f * std::pow(750.0f, f2CutNorm);

        const float velScale = (1.0f - params.velLink) + params.velLink * v.velocity;
        const float baseVoiceScale = velScale * 0.34f;

        const float blurTau = (blurActiveInChain && params.blurEnable)
            ? (0.0005f + std::pow(params.blurTime, 1.6f) * 0.65f)
            : 0.0005f;
        const float blurCoeff = 1.0f - std::exp(-dtSeconds / blurTau);

        for (int k = 0; k < NUM_PARTIALS; ++k)
        {
            const size_t idx = static_cast<size_t>(k);
            const float fkA = v.freqHzA[idx];
            const float fkB = v.freqHzB[idx];
            const bool overA = (fkA >= nyquist);
            const bool overB = (fkB >= nyquist);
            if (overA && overB)
            {
                v.targetAmpA[idx] = 0.0f;
                v.targetAmpB[idx] = 0.0f;
                v.filterGainTable[idx] = 0.0f;
                continue;
            }

            auto evalMaskForFreq = [&](float fk) -> float
            {
                const float g1 = params.filter1Enable
                    ? evaluateFilterMask(params.filter1Type, fk, f1CutoffHz, params.filter1Res, params.filter1Width, customFilterCurve)
                    : 1.0f;
                const float g2 = params.filter2Enable
                    ? evaluateFilterMask(params.filter2Type, fk, f2CutoffHz, params.filter2Res, params.filter2Width, customFilterCurve)
                    : 1.0f;

                float mask = 1.0f;
                if (params.filter1Enable && params.filter2Enable)
                {
                    const float serialG   = std::min(2.8f, g1 * g2);
                    const float parallelG = 0.5f * (g1 + g2);
                    mask = serialG * (1.0f - params.filterRouting) + parallelG * params.filterRouting;
                }
                else if (params.filter1Enable)
                    mask = g1;
                else if (params.filter2Enable)
                    mask = g2;
                return mask;
            };

            const float maskA = (!overA) ? evalMaskForFreq(fkA) : 0.0f;
            const float maskB = (!overB) ? evalMaskForFreq(fkB) : 0.0f;
            v.filterGainTable[idx] = juce::jlimit(0.0f, 2.5f, maskA);

            const float desiredA = (!overA) ? (rawAmpA[idx] * maskA * baseVoiceScale) : 0.0f;
            const float desiredB = (!overB) ? (rawAmpB[idx] * maskB * baseVoiceScale) : 0.0f;

            v.blurStateA[idx] = (!overA) ? (v.blurStateA[idx] + blurCoeff * (desiredA - v.blurStateA[idx])) : 0.0f;
            v.blurStateB[idx] = (!overB) ? (v.blurStateB[idx] + blurCoeff * (desiredB - v.blurStateB[idx])) : 0.0f;

            v.targetAmpA[idx] = v.blurStateA[idx];
            v.targetAmpB[idx] = v.blurStateB[idx];
        }

        // Unison Sub-Oscillator Detuning, Panning & Stack Modes (All 8 Modes!)
        const int targetCount = juce::jlimit(1, MAX_UNISON, static_cast<int>(std::round(uVoicesCount)));
        const float weightSlewRate = dtSeconds / 0.015f;
        int maxActiveU = 1;
        for (int u = 0; u < MAX_UNISON; ++u)
        {
            const float targetW = (u < targetCount) ? 1.0f : 0.0f;
            const float diff = targetW - v.curUniVoiceWeight[static_cast<size_t>(u)];
            if (std::abs(diff) <= weightSlewRate)
                v.curUniVoiceWeight[static_cast<size_t>(u)] = targetW;
            else
                v.curUniVoiceWeight[static_cast<size_t>(u)] += (diff > 0.0f ? weightSlewRate : -weightSlewRate);

            if (v.curUniVoiceWeight[static_cast<size_t>(u)] > 0.0005f || u < targetCount)
                maxActiveU = std::max(maxActiveU, u + 1);
        }

        const float detuneCentsMax = uDetune * uDetune * 160.0f + uDetune * 40.0f;
        float sumSqWeights = 0.0f;

        for (int u = 0; u < maxActiveU; ++u)
        {
            const float normPos = (maxActiveU <= 1) ? 0.0f : (-1.0f + (2.0f * static_cast<float>(u)) / static_cast<float>(maxActiveU - 1));
            const float edgeDist = std::abs(normPos);
            const float superDetuneNorm = (normPos >= 0.0f ? 1.0f : -1.0f) * std::pow(edgeDist, 1.35f);
            float ratio = std::pow(2.0f, (superDetuneNorm * detuneCentsMax) / 1200.0f);

            if (params.unisonStackMode > 0 && maxActiveU > 1)
            {
                switch (params.unisonStackMode)
                {
                    case 1: if (u % 2 == 1) ratio *= 2.0f; break;                                                // +1 Oct
                    case 2: ratio *= (u % 3 == 1 ? 2.0f : (u % 3 == 2 ? 4.0f : 1.0f)); break;                    // +2 Oct
                    case 3: if (u == 0) ratio *= 0.5f; break;                                                    // Sub+Oct
                    case 4: if (u % 2 == 1) ratio *= 1.4983071f; break;                                          // Fifths
                    case 5: ratio *= (u % 3 == 1 ? 1.4983071f : (u % 3 == 2 ? 2.0f : 1.0f)); break;              // Oct+5th
                    case 6: ratio *= (u % 3 == 1 ? 1.1892071f : (u % 3 == 2 ? 1.4983071f : 1.0f)); break;        // Minor Triad (+3st, +7st)
                    case 7: ratio *= (u % 3 == 1 ? 1.2599210f : (u % 3 == 2 ? 1.4983071f : 1.0f)); break;        // Major Triad (+4st, +7st)
                    default: break;
                }
            }
            v.unisonPitchRatio[static_cast<size_t>(u)] = ratio;

            const float amp = ((1.0f - edgeDist) + edgeDist * uBlend) * v.curUniVoiceWeight[static_cast<size_t>(u)];
            const float panPos = juce::jlimit(-1.0f, 1.0f, normPos * uSpread);
            const float panAngle = (panPos + 1.0f) * juce::MathConstants<float>::pi * 0.25f;

            v.uWeightL[static_cast<size_t>(u)] = amp * std::cos(panAngle);
            v.uWeightR[static_cast<size_t>(u)] = amp * std::sin(panAngle);
            sumSqWeights += amp * amp;
        }

        const float gainNorm = 1.0f / std::sqrt(std::max(0.01f, sumSqWeights));
        for (int u = 0; u < maxActiveU; ++u)
        {
            v.uWeightL[static_cast<size_t>(u)] *= gainNorm;
            v.uWeightR[static_cast<size_t>(u)] *= gainNorm;

            const float subFreqA = juce::jlimit(10.0f, nyquist, f0A * v.unisonPitchRatio[static_cast<size_t>(u)]);
            const float subFreqB = juce::jlimit(10.0f, nyquist, f0B * v.unisonPitchRatio[static_cast<size_t>(u)]);
            v.subPhaseIncA[static_cast<size_t>(u)] = static_cast<uint32_t>((subFreqA * invSampleRate) * 4294967296.0);
            v.subPhaseIncB[static_cast<size_t>(u)] = static_cast<uint32_t>((subFreqB * invSampleRate) * 4294967296.0);
        }

        // Sync phase transition if entering additive mode dynamically
        v.wasAdditiveA = v.useAdditiveA;
        v.wasAdditiveB = v.useAdditiveB;
        v.useAdditiveA = needAdditiveA;
        v.useAdditiveB = needAdditiveB;
        if ((v.useAdditiveA && !v.wasAdditiveA) || (v.useAdditiveB && !v.wasAdditiveB))
        {
            syncAdditivePhasesFromWavetable(v, v.useAdditiveA && !v.wasAdditiveA, v.useAdditiveB && !v.wasAdditiveB);
            if (v.useAdditiveA && !v.wasAdditiveA)
            {
                for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
                    v.prevAddAmpA[static_cast<size_t>(k)] = v.targetAmpA[static_cast<size_t>(k)];
            }
            if (v.useAdditiveB && !v.wasAdditiveB)
            {
                for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
                    v.prevAddAmpB[static_cast<size_t>(k)] = v.targetAmpB[static_cast<size_t>(k)];
            }
        }
        if ((v.wasAdditiveA && !v.useAdditiveA) || (v.wasAdditiveB && !v.useAdditiveB))
            syncWavetablePhasesFromAdditive(v, v.wasAdditiveA && !v.useAdditiveA, v.wasAdditiveB && !v.useAdditiveB);

        // Build Single-Cycle Bandlimited Wavetable ONLY for whichever Field uses Path A (Harmonic)
        const bool doA = (mixA >= 0.001f);
        const bool doB = (mixB >= 0.001f);

        if (v.hasPrevTable)
        {
            v.prevWaveTableA = v.waveTableA;
            v.prevWaveTableB = v.waveTableB;
        }
        else
        {
            v.prevWaveTableA.fill(0.0f);
            v.prevWaveTableB.fill(0.0f);
            v.hasPrevTable = true;
        }

        if (doA && !v.useAdditiveA) v.waveTableA.fill(0.0f);
        if (doB && !v.useAdditiveB) v.waveTableB.fill(0.0f);

        float maxRatio = 1.0f;
        for (int u = 0; u < maxActiveU; ++u)
            maxRatio = std::max(maxRatio, v.unisonPitchRatio[static_cast<size_t>(u)]);

        const float maxF0A = f0A * maxRatio;
        const float maxF0B = f0B * maxRatio;
        const float nyquistGuard = nyquist * 0.95f;
        const float rollOffStart = nyquist * 0.80f;

        if ((doA && !v.useAdditiveA) || (doB && !v.useAdditiveB))
        {
            for (int k = 0; k < NUM_PARTIALS; ++k)
            {
                float aA = (doA && !v.useAdditiveA) ? v.targetAmpA[static_cast<size_t>(k)] : 0.0f;
                float aB = (doB && !v.useAdditiveB) ? v.targetAmpB[static_cast<size_t>(k)] : 0.0f;

                const float h = static_cast<float>(k + 1);
                const float fkA_max = h * maxF0A;
                const float fkB_max = h * maxF0B;

                if (fkA_max >= nyquistGuard) aA = 0.0f;
                else if (fkA_max > rollOffStart) aA *= 0.5f * (1.0f + std::cos(((fkA_max - rollOffStart) / (nyquistGuard - rollOffStart)) * juce::MathConstants<float>::pi));

                if (fkB_max >= nyquistGuard) aB = 0.0f;
                else if (fkB_max > rollOffStart) aB *= 0.5f * (1.0f + std::cos(((fkB_max - rollOffStart) / (nyquistGuard - rollOffStart)) * juce::MathConstants<float>::pi));

                if (aA < 1e-5f && aB < 1e-5f)
                    continue;

                const uint32_t step = static_cast<uint32_t>(k + 1) * 4194304u;
                if (aA >= 1e-5f)
                {
                    uint32_t phA = static_cast<uint32_t>(resynthEngine.getHarmonicPhaseRaw(0, k) * 4294967295.0f);
                    for (int i = 0; i < Voice::WAVE_TABLE_SIZE; ++i)
                    {
                        v.waveTableA[static_cast<size_t>(i)] += aA * sineLUT[phA >> (32 - 13)];
                        phA += step;
                    }
                }
                if (aB >= 1e-5f)
                {
                    uint32_t phB = static_cast<uint32_t>(resynthEngine.getHarmonicPhaseRaw(1, k) * 4294967295.0f + 0x40000000u);
                    for (int i = 0; i < Voice::WAVE_TABLE_SIZE; ++i)
                    {
                        v.waveTableB[static_cast<size_t>(i)] += aB * sineLUT[phB >> (32 - 13)];
                        phB += step;
                    }
                }
            }
        }

        if (v.wasAdditiveA && !v.useAdditiveA) v.prevWaveTableA = v.waveTableA;
        if (v.wasAdditiveB && !v.useAdditiveB) v.prevWaveTableB = v.waveTableB;

        // Master voice gain (true AHDSR * Volume Art * Velocity)
        v.targetGain = juce::jlimit(0.0f, 0.35f, v.targetVolEnv * velScale * 0.26f);
    }

    void renderChunk(float* outL, float* outR, int numSamples, const SynthParameters& params)
    {
        juce::ScopedNoDenormals noDenormals;
        const float dtSeconds = static_cast<float>(numSamples) * invSampleRate;
        const float invChunk  = 1.0f / static_cast<float>(numSamples);

        const float mixB = juce::jlimit(0.0f, 1.0f, params.abMix);
        const float mixA = 1.0f - mixB;
        const bool doA = (mixA >= 0.001f);
        const bool doB = (mixB >= 0.001f);

        const float pan = juce::jlimit(-1.0f, 1.0f, params.partPan);
        const float panGainL = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
        const float panGainR = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;

        const float uDet = smoothedDetune.getCurrentValue();
        const float uSpr = smoothedSpread.getCurrentValue();
        const float uBle = smoothedBlend.getCurrentValue();
        const float uVoi = smoothedUnisonVoices.getCurrentValue();
        smoothedDetune.skip(numSamples);
        smoothedSpread.skip(numSamples);
        smoothedBlend.skip(numSamples);
        smoothedUnisonVoices.skip(numSamples);

        const bool blurActive = isFxModuleActive(FxModuleId::Blur);
        const float nyquist = static_cast<float>(sampleRate * 0.47);
        const float nyqGuard = nyquist * 0.95f;

        Voice* newestActiveVoice = nullptr;
        int activeCount = 0;

        int releasingCount = 0;
        for (const auto& vCheck : *voices)
        {
            if (vCheck.active && vCheck.releasing)
                ++releasingCount;
        }

        for (auto& v : *voices)
        {
            if (!v.active)
                continue;

            updateVoiceSpectralTables(v, dtSeconds, params, uDet, uSpr, uBle, uVoi, mixA, mixB, blurActive, releasingCount);
            if (!v.active)
                continue;

            ++activeCount;
            if (newestActiveVoice == nullptr || v.noteAgeSeconds < newestActiveVoice->noteAgeSeconds)
                newestActiveVoice = &v;

            float curGain = v.curGain;
            const float targetGain = v.targetGain;
            const float stepGain = (targetGain - curGain) * invChunk;

            int maxU = 1;
            for (int u = 0; u < MAX_UNISON; ++u)
            {
                if (v.curUniVoiceWeight[static_cast<size_t>(u)] > 0.0005f)
                    maxU = u + 1;
            }

            // Precompute active additive partial lists if Path B enabled on A or B
            std::array<int, ADDITIVE_PARTIALS> activeAddK_A {};
            int activeAddCountA = 0;
            if (doA && v.useAdditiveA)
            {
                for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
                {
                    if (v.targetAmpA[static_cast<size_t>(k)] > 2e-4f || v.prevAddAmpA[static_cast<size_t>(k)] > 2e-4f)
                        activeAddK_A[static_cast<size_t>(activeAddCountA++)] = k;
                }
            }

            std::array<int, ADDITIVE_PARTIALS> activeAddK_B {};
            int activeAddCountB = 0;
            if (doB && v.useAdditiveB)
            {
                for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
                {
                    if (v.targetAmpB[static_cast<size_t>(k)] > 2e-4f || v.prevAddAmpB[static_cast<size_t>(k)] > 2e-4f)
                        activeAddK_B[static_cast<size_t>(activeAddCountB++)] = k;
                }
            }

            for (int s = 0; s < numSamples; ++s)
            {
                const float crossNew = static_cast<float>(s + 1) * invChunk;
                const float crossOld = 1.0f - crossNew;
                float sumVoiceL = 0.0f;
                float sumVoiceR = 0.0f;

                for (int u = 0; u < maxU; ++u)
                {
                    const size_t uIdx = static_cast<size_t>(u);
                    const float wL = v.uWeightL[uIdx];
                    const float wR = v.uWeightR[uIdx];
                    if (std::abs(wL) + std::abs(wR) < 1e-5f)
                    {
                        v.subPhaseA[uIdx] += v.subPhaseIncA[uIdx];
                        v.subPhaseB[uIdx] += v.subPhaseIncB[uIdx];
                        continue;
                    }

                    float sampleA = 0.0f;
                    if (doA)
                    {
                        if (!v.useAdditiveA)
                        {
                            const uint32_t phA = v.subPhaseA[uIdx];
                            const uint32_t idxA0 = phA >> (32 - 10);
                            const uint32_t idxA1 = (idxA0 + 1) & (Voice::WAVE_TABLE_SIZE - 1);
                            const float fracA = static_cast<float>(phA & 0x003FFFFF) * (1.0f / 4194304.0f);
                            const float sA_old = v.prevWaveTableA[idxA0] * (1.0f - fracA) + v.prevWaveTableA[idxA1] * fracA;
                            const float sA_new = v.waveTableA[idxA0] * (1.0f - fracA) + v.waveTableA[idxA1] * fracA;
                            sampleA = sA_old * crossOld + sA_new * crossNew;
                        }
                        else
                        {
                            const float uRatio = v.unisonPitchRatio[uIdx];
                            for (int a = 0; a < activeAddCountA; ++a)
                            {
                                const size_t k = static_cast<size_t>(activeAddK_A[static_cast<size_t>(a)]);
                                const float fk = v.freqHzA[k] * uRatio;
                                if (fk >= nyqGuard) continue;
                                const uint32_t inc = static_cast<uint32_t>((fk * invSampleRate) * 4294967296.0);
                                const float amp = v.prevAddAmpA[k] * crossOld + v.targetAmpA[k] * crossNew;
                                sampleA += amp * sineLUT[v.addPhaseA[uIdx][k] >> 19];
                                v.addPhaseA[uIdx][k] += inc;
                            }
                        }
                        v.subPhaseA[uIdx] += v.subPhaseIncA[uIdx];
                    }

                    float sampleB = 0.0f;
                    if (doB)
                    {
                        if (!v.useAdditiveB)
                        {
                            const uint32_t phB = v.subPhaseB[uIdx];
                            const uint32_t idxB0 = phB >> (32 - 10);
                            const uint32_t idxB1 = (idxB0 + 1) & (Voice::WAVE_TABLE_SIZE - 1);
                            const float fracB = static_cast<float>(phB & 0x003FFFFF) * (1.0f / 4194304.0f);
                            const float sB_old = v.prevWaveTableB[idxB0] * (1.0f - fracB) + v.prevWaveTableB[idxB1] * fracB;
                            const float sB_new = v.waveTableB[idxB0] * (1.0f - fracB) + v.waveTableB[idxB1] * fracB;
                            sampleB = sB_old * crossOld + sB_new * crossNew;
                        }
                        else
                        {
                            const float uRatio = v.unisonPitchRatio[uIdx];
                            for (int a = 0; a < activeAddCountB; ++a)
                            {
                                const size_t k = static_cast<size_t>(activeAddK_B[static_cast<size_t>(a)]);
                                const float fk = v.freqHzB[k] * uRatio;
                                if (fk >= nyqGuard) continue;
                                const uint32_t inc = static_cast<uint32_t>((fk * invSampleRate) * 4294967296.0);
                                const float amp = v.prevAddAmpB[k] * crossOld + v.targetAmpB[k] * crossNew;
                                sampleB += amp * sineLUT[v.addPhaseB[uIdx][k] >> 19];
                                v.addPhaseB[uIdx][k] += inc;
                            }
                        }
                        v.subPhaseB[uIdx] += v.subPhaseIncB[uIdx];
                    }

                    const float subSig = sampleA * mixA + sampleB * mixB;
                    sumVoiceL += subSig * wL;
                    sumVoiceR += subSig * wR;
                }

                if (!std::isfinite(sumVoiceL)) sumVoiceL = 0.0f;
                if (!std::isfinite(sumVoiceR)) sumVoiceR = 0.0f;

                const float voiceL = sumVoiceL * curGain * panGainL;
                const float voiceR = sumVoiceR * curGain * panGainR;

                outL[s] += std::tanh(voiceL);
                outR[s] += std::tanh(voiceR);
                curGain += stepGain;
            }

            v.curGain = targetGain;
            if (doA && !v.useAdditiveA) v.prevWaveTableA = v.waveTableA;
            if (doB && !v.useAdditiveB) v.prevWaveTableB = v.waveTableB;
            for (int k = 0; k < ADDITIVE_PARTIALS; ++k)
            {
                const size_t kIdx = static_cast<size_t>(k);
                v.prevAddAmpA[kIdx] = v.targetAmpA[kIdx];
                v.prevAddAmpB[kIdx] = v.targetAmpB[kIdx];
            }
            for (int k = 0; k < NUM_PARTIALS; ++k)
            {
                v.curAmpA[static_cast<size_t>(k)] = v.targetAmpA[static_cast<size_t>(k)];
                v.curAmpB[static_cast<size_t>(k)] = v.targetAmpB[static_cast<size_t>(k)];
            }
        }

        // Update Spectral Telemetry
        pendingSnapshot.activeArpStep = currentArpStepAtomic.load(std::memory_order_relaxed);
        if (newestActiveVoice != nullptr)
        {
            pendingSnapshot.voiceActive   = true;
            pendingSnapshot.envCursors    = newestActiveVoice->artPhase;
            pendingSnapshot.envCursor     = newestActiveVoice->artPhase[0];
            pendingSnapshot.activePitchHz = newestActiveVoice->currentBaseFreq;
            latestVoiceEnvGain = newestActiveVoice->targetVolEnv;

            for (int i = 0; i < VisualizerSnapshot::NUM_BINS; ++i)
            {
                const size_t k = static_cast<size_t>(i);
                const float fkA = std::max(20.0f, newestActiveVoice->freqHzA[k]);
                const float fkB = std::max(20.0f, newestActiveVoice->freqHzB[k]);
                const float logNormA = (std::log2(fkA / 30.0f)) / std::log2(18000.0f / 30.0f);
                const float logNormB = (std::log2(fkB / 30.0f)) / std::log2(18000.0f / 30.0f);
                pendingSnapshot.freqNormA[k] = juce::jlimit(0.0f, 1.0f, logNormA);
                pendingSnapshot.freqNormB[k] = juce::jlimit(0.0f, 1.0f, logNormB);
                pendingSnapshot.ampA[k] = juce::jlimit(0.0f, 1.0f, std::pow(newestActiveVoice->curAmpA[k] * 4.8f, 0.55f));
                pendingSnapshot.ampB[k] = juce::jlimit(0.0f, 1.0f, std::pow(newestActiveVoice->curAmpB[k] * 4.8f, 0.55f));
                const float fkDisplay = 30.0f * std::pow(600.0f, static_cast<float>(i) / static_cast<float>(VisualizerSnapshot::NUM_BINS - 1));
                pendingSnapshot.filterCurve[k] = juce::jlimit(0.0f, 1.0f, newestActiveVoice->filterGainTable[k] * evaluateParametricEQGain(fkDisplay, params) * 0.65f);
            }
        }
        else
        {
            pendingSnapshot.voiceActive = false;
            pendingSnapshot.envCursors.fill(0.0f);
            pendingSnapshot.envCursor = 0.0f;
            latestVoiceEnvGain = 0.0f;
            const auto& customFilterCurve = articulators[static_cast<size_t>(ArticulatorTarget::CustomFilterShape)];
            const float f1Cut = 25.0f * std::pow(750.0f, juce::jlimit(0.0f, 1.0f, params.filter1Cutoff));
            const float f2Cut = 25.0f * std::pow(750.0f, juce::jlimit(0.0f, 1.0f, params.filter2Cutoff));

            for (int i = 0; i < VisualizerSnapshot::NUM_BINS; ++i)
            {
                const size_t k = static_cast<size_t>(i);
                const float norm = static_cast<float>(i) / static_cast<float>(VisualizerSnapshot::NUM_BINS - 1);
                const float fk = 30.0f * std::pow(600.0f, norm);
                pendingSnapshot.freqNormA[k] = norm;
                pendingSnapshot.freqNormB[k] = norm;
                pendingSnapshot.ampA[k] = 0.0f;
                pendingSnapshot.ampB[k] = 0.0f;

                const float g1 = params.filter1Enable ? evaluateFilterMask(params.filter1Type, fk, f1Cut, params.filter1Res, params.filter1Width, customFilterCurve) : 1.0f;
                const float g2 = params.filter2Enable ? evaluateFilterMask(params.filter2Type, fk, f2Cut, params.filter2Res, params.filter2Width, customFilterCurve) : 1.0f;
                float combined = 1.0f;
                if (params.filter1Enable && params.filter2Enable)
                    combined = (g1 * g2) * (1.0f - params.filterRouting) + 0.5f * (g1 + g2) * params.filterRouting;
                else if (params.filter1Enable)
                    combined = g1;
                else if (params.filter2Enable)
                    combined = g2;

                combined *= evaluateParametricEQGain(fk, params);
                pendingSnapshot.filterCurve[k] = juce::jlimit(0.0f, 1.0f, combined * 0.60f);
            }
        }
    }

    double sampleRate    = 44100.0;
    float  invSampleRate = 1.0f / 44100.0f;
    float  lastPlayedFreq = 130.81f;
    float  latestVoiceEnvGain = 0.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDetune;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedSpread;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBlend;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedUnisonVoices;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedPolyHeadroom;

    mutable juce::CriticalSection arpLock;
    std::array<std::array<ArpStep, NUM_ARP_STEPS>, 2> arpStepsBuf {};
    std::atomic<int> activeArpBufIndex { 0 };
    FixedList<HeldNote, 64> arpHeldNotes;
    FixedList<int, 64> activeArpTriggeredNotes;
    int   arpStepIndex = 0;
    int   arpNoteIndex = 0;
    int   arpDirection = 1;
    float arpPhase = 0.0f;
    bool  arpWasEnabled = false;
    std::atomic<int> currentArpStepAtomic { 0 };

    mutable juce::CriticalSection fxOrderLock;
    std::vector<int> activeFxChain;
    std::array<int, MAX_FX_CHAIN_LEN> activeFxChainArray {};
    std::atomic<int> activeFxChainCount { 0 };
    std::atomic<uint32_t> activeFxMask { 0xFFFFFFFFu };

    SplineCurve timeModCurve;
    FxRack fxRack;
    FixedList<HeldNote, 64> heldNotes;
    juce::Random rng;

    std::array<float, SINE_LUT_SIZE> sineLUT {};
    std::unique_ptr<std::array<Voice, MAX_VOICES>> voices;
    std::array<SplineCurve, static_cast<size_t>(ArticulatorTarget::Count)> articulators {};
    ResynthesisEngine resynthEngine;

    VisualizerSnapshot pendingSnapshot {};
    std::array<VisualizerSnapshot, 2> visSnapshots {};
    std::atomic<int> activeVisIndex { 0 };
    std::atomic<int> activeVoicesAtomic { 0 };
};

} // namespace PrismorDSP
