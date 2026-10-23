#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <atomic>

namespace PrismorDSP
{

// Segment shape mode for ShaperBox-style spline interpolation
// 0 = Concave/Convex Exponential Curve, 1 = S-Curve (Sigmoid/Cosine), 2 = Linear
struct SplineNode
{
    float x = 0.0f;       // [0.0 .. 1.0]
    float y = 1.0f;       // [0.0 .. 1.0]
    float tension = 0.0f; // [-0.95 .. 0.95] segment curvature/tension to next node
    int shapeMode = 0;    // 0 = Exp/Log (Concave/Convex), 1 = S-Curve, 2 = Linear
};

enum class ArticulatorTarget : int
{
    Volume = 0,
    Pitch = 1,
    Filter1Cutoff = 2,
    CustomFilterShape = 3,
    Timbre = 4,
    Prism = 5,
    Count = 6
};

enum class ArticulatorMode : int
{
    Envelope = 0,
    Loop = 1,
    LFO = 2
};

class SplineCurve
{
public:
    static constexpr int LUT_SIZE = 512;

    SplineCurve()
    {
        resetToDefault(ArticulatorTarget::Volume);
    }

    static juce::StringArray getSyncDivisionNames()
    {
        return {
            "4 BAR", "2 BAR", "1 BAR", "1/2", "1/4",
            "1/8", "1/8T", "1/16", "1/16T", "1/32"
        };
    }

    // Converts a rhythmic division index + host BPM into cycles per second (Hz)
    static float divisionToHz(int divIndex, double bpm) noexcept
    {
        const float safeBpm = static_cast<float>(juce::jlimit(30.0, 300.0, bpm));
        const float beatsPerSec = safeBpm / 60.0f; // 1/4 note rate in Hz
        switch (juce::jlimit(0, 9, divIndex))
        {
            case 0: return beatsPerSec / 16.0f; // 4 Bars
            case 1: return beatsPerSec / 8.0f;  // 2 Bars
            case 2: return beatsPerSec / 4.0f;  // 1 Bar
            case 3: return beatsPerSec / 2.0f;  // 1/2
            case 4: return beatsPerSec;         // 1/4
            case 5: return beatsPerSec * 2.0f;  // 1/8
            case 6: return beatsPerSec * 3.0f;  // 1/8T
            case 7: return beatsPerSec * 4.0f;  // 1/16
            case 8: return beatsPerSec * 6.0f;  // 1/16T
            case 9: return beatsPerSec * 8.0f;  // 1/32
            default: return beatsPerSec / 4.0f;
        }
    }

    void clearToFlat(ArticulatorTarget target)
    {
        juce::ScopedLock sl(lock);
        nodes.clear();
        globalShapeMode.store(0, std::memory_order_relaxed);

        float flatY = 1.0f;
        if (target == ArticulatorTarget::Pitch ||
            target == ArticulatorTarget::Filter1Cutoff ||
            target == ArticulatorTarget::Prism)
        {
            flatY = 0.5f;
        }

        nodes.push_back({ 0.0f, flatY, 0.0f, 0 });
        nodes.push_back({ 0.5f, flatY, 0.0f, 0 });
        nodes.push_back({ 1.0f, flatY, 0.0f, 0 });
        rebuildLUT();
    }

    void resetToDefault(ArticulatorTarget target)
    {
        juce::ScopedLock sl(lock);
        nodes.clear();
        globalShapeMode.store(0, std::memory_order_relaxed);

        switch (target)
        {
            case ArticulatorTarget::Volume:
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(false, std::memory_order_relaxed);
                syncDivision.store(4, std::memory_order_relaxed); // 1/4
                rate.store(1.5f, std::memory_order_relaxed);
                nodes.push_back({ 0.0f,  0.05f, -0.45f, 0 });
                nodes.push_back({ 0.06f, 1.00f,  0.30f, 0 });
                nodes.push_back({ 0.42f, 0.72f,  0.20f, 0 });
                nodes.push_back({ 0.82f, 0.58f,  0.35f, 0 });
                nodes.push_back({ 1.0f,  0.15f,  0.0f,  0 });
                break;

            case ArticulatorTarget::Pitch:
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(false, std::memory_order_relaxed);
                syncDivision.store(4, std::memory_order_relaxed); // 1/4
                rate.store(2.0f, std::memory_order_relaxed);
                nodes.push_back({ 0.0f,  0.5f,  0.0f, 0 });
                nodes.push_back({ 0.25f, 0.5f,  0.0f, 0 });
                nodes.push_back({ 0.65f, 0.5f,  0.0f, 0 });
                nodes.push_back({ 1.0f,  0.5f,  0.0f, 0 });
                break;

            case ArticulatorTarget::Filter1Cutoff:
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(false, std::memory_order_relaxed);
                syncDivision.store(3, std::memory_order_relaxed); // 1/2
                rate.store(1.5f, std::memory_order_relaxed);
                nodes.push_back({ 0.0f,  0.85f,  0.45f, 0 });
                nodes.push_back({ 0.28f, 0.38f, -0.25f, 0 });
                nodes.push_back({ 0.65f, 0.62f,  0.30f, 0 });
                nodes.push_back({ 1.0f,  0.85f,  0.0f,  0 });
                break;

            case ArticulatorTarget::CustomFilterShape:
                mode.store(static_cast<int>(ArticulatorMode::Envelope), std::memory_order_relaxed);
                tempoSync.store(false, std::memory_order_relaxed);
                syncDivision.store(2, std::memory_order_relaxed);
                rate.store(1.0f, std::memory_order_relaxed);
                nodes.push_back({ 0.0f,  1.0f,  0.0f,  0 });
                nodes.push_back({ 0.28f, 0.92f, 0.55f, 0 });
                nodes.push_back({ 0.48f, 0.22f,-0.55f, 0 });
                nodes.push_back({ 0.64f, 0.98f, 0.65f, 0 });
                nodes.push_back({ 0.82f, 0.08f, 0.30f, 0 });
                nodes.push_back({ 1.0f,  0.0f,  0.0f,  0 });
                break;

            case ArticulatorTarget::Timbre:
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(2, std::memory_order_relaxed); // 1 Bar
                rate.store(1.0f, std::memory_order_relaxed);
                nodes.push_back({ 0.0f,  0.25f, -0.35f, 1 });
                nodes.push_back({ 0.45f, 0.85f,  0.35f, 1 });
                nodes.push_back({ 1.0f,  0.25f,  0.0f,  1 });
                break;

            case ArticulatorTarget::Prism:
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(3, std::memory_order_relaxed); // 1/2
                rate.store(1.2f, std::memory_order_relaxed);
                nodes.push_back({ 0.0f,  0.5f, -0.3f, 0 });
                nodes.push_back({ 0.5f,  0.5f,  0.3f, 0 });
                nodes.push_back({ 1.0f,  0.5f,  0.0f, 0 });
                break;

            case ArticulatorTarget::Count:
            default:
                nodes.push_back({ 0.0f, 1.0f, 0.0f, 0 });
                nodes.push_back({ 1.0f, 1.0f, 0.0f, 0 });
                break;
        }

        rebuildLUT();
    }

    // ShaperBox 3 Inspired Gate & Curve Presets
    // 0: SIDECHAIN PUMP, 1: TRANCE GATE, 2: STUTTER, 3: SAW RAMP, 4: TRIPLET GROOVE, 5: SINE WAVE, 6: PLUCK, 7: SWELL
    void applyPresetShape(int shapeIndex)
    {
        juce::ScopedLock sl(lock);
        nodes.clear();
        switch (shapeIndex)
        {
            case 0: // ShaperBox Sidechain Pump (Ducking Curve)
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(4, std::memory_order_relaxed); // 1/4 Note Pump
                rate.store(2.0f, std::memory_order_relaxed);
                nodes = {
                    { 0.0f,  0.02f,  0.0f,  0 },
                    { 0.12f, 0.08f, -0.58f, 0 },
                    { 0.58f, 0.96f, -0.35f, 0 },
                    { 0.94f, 1.00f,  0.65f, 0 },
                    { 1.0f,  0.02f,  0.0f,  0 }
                };
                break;

            case 1: // ShaperBox Rhythmic Trance Gate (8-Step Gate)
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(2, std::memory_order_relaxed); // 1 Bar
                rate.store(1.0f, std::memory_order_relaxed);
                nodes = {
                    { 0.00f, 1.00f,  0.75f, 0 },
                    { 0.12f, 0.05f, -0.80f, 0 },
                    { 0.25f, 0.95f,  0.75f, 0 },
                    { 0.37f, 0.08f, -0.80f, 0 },
                    { 0.50f, 1.00f,  0.40f, 0 },
                    { 0.68f, 0.85f,  0.80f, 0 },
                    { 0.75f, 0.05f, -0.80f, 0 },
                    { 0.88f, 0.95f,  0.75f, 0 },
                    { 1.00f, 0.05f,  0.0f,  0 }
                };
                break;

            case 2: // ShaperBox Stutter Chop (Accelerating Gate)
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(3, std::memory_order_relaxed); // 1/2
                rate.store(2.5f, std::memory_order_relaxed);
                nodes = {
                    { 0.00f, 1.00f,  0.55f, 0 },
                    { 0.24f, 0.05f, -0.85f, 0 },
                    { 0.26f, 1.00f,  0.55f, 0 },
                    { 0.48f, 0.05f, -0.85f, 0 },
                    { 0.50f, 0.95f,  0.65f, 0 },
                    { 0.62f, 0.05f, -0.85f, 0 },
                    { 0.64f, 0.95f,  0.65f, 0 },
                    { 0.76f, 0.05f, -0.85f, 0 },
                    { 0.78f, 0.95f,  0.70f, 0 },
                    { 0.88f, 0.05f, -0.85f, 0 },
                    { 0.90f, 0.95f,  0.70f, 0 },
                    { 1.00f, 0.05f,  0.0f,  0 }
                };
                break;

            case 3: // Sawtooth Ramp
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(5, std::memory_order_relaxed); // 1/8
                rate.store(4.0f, std::memory_order_relaxed);
                nodes = {
                    { 0.00f, 1.00f,  0.22f, 0 },
                    { 0.96f, 0.02f, -0.85f, 2 },
                    { 1.00f, 1.00f,  0.0f,  2 }
                };
                break;

            case 4: // Triplet Groove (3 Equal Swells)
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(3, std::memory_order_relaxed); // 1/2
                rate.store(1.8f, std::memory_order_relaxed);
                nodes = {
                    { 0.000f, 0.10f, -0.50f, 0 },
                    { 0.166f, 1.00f,  0.50f, 0 },
                    { 0.333f, 0.08f, -0.50f, 0 },
                    { 0.500f, 0.95f,  0.50f, 0 },
                    { 0.666f, 0.08f, -0.50f, 0 },
                    { 0.833f, 0.95f,  0.50f, 0 },
                    { 1.000f, 0.10f,  0.0f,  0 }
                };
                break;

            case 5: // Sine Wave (S-Curve LFO)
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(3, std::memory_order_relaxed); // 1/2
                rate.store(2.0f, std::memory_order_relaxed);
                nodes = {
                    { 0.00f, 0.50f, -0.35f, 0 },
                    { 0.25f, 0.98f,  0.35f, 0 },
                    { 0.50f, 0.50f,  0.35f, 0 },
                    { 0.75f, 0.02f, -0.35f, 0 },
                    { 1.00f, 0.50f,  0.0f,  0 }
                };
                break;

            case 6: // Snappy Pluck (ADSR)
                mode.store(static_cast<int>(ArticulatorMode::Envelope), std::memory_order_relaxed);
                tempoSync.store(false, std::memory_order_relaxed);
                rate.store(1.2f, std::memory_order_relaxed);
                nodes = {
                    { 0.0f,  0.0f, -0.60f, 0 },
                    { 0.02f, 1.0f,  0.58f, 0 },
                    { 0.28f, 0.20f, 0.45f, 0 },
                    { 1.0f,  0.0f,  0.0f,  0 }
                };
                break;

            case 7: // Cyber Swell / Riser
            default:
                mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
                tempoSync.store(true, std::memory_order_relaxed);
                syncDivision.store(1, std::memory_order_relaxed); // 2 Bars
                rate.store(0.5f, std::memory_order_relaxed);
                nodes = {
                    { 0.0f,  0.04f,  0.48f, 0 },
                    { 0.62f, 1.00f, -0.35f, 1 },
                    { 0.84f, 0.68f,  0.40f, 0 },
                    { 1.0f,  0.04f,  0.0f,  0 }
                };
                break;
        }
        rebuildLUT();
    }

    // Serum-2 Style Physical AHDSR Envelope Shaper
    // Maps Attack, Hold, Decay, Sustain, and Release [0..1] into a 5-node AHDSR curve
    void applyAHDSRParameters(float attack, float hold, float decay, float sustain, float release)
    {
        juce::ScopedLock sl(lock);
        nodes.clear();

        const float a = juce::jlimit(0.005f, 0.32f, attack * 0.30f + 0.005f);
        const float h = juce::jlimit(0.0f,   0.22f, hold * 0.22f);
        const float d = juce::jlimit(0.02f,  0.34f, decay * 0.32f + 0.02f);
        const float s = juce::jlimit(0.0f,   1.0f,  sustain);
        const float r = juce::jlimit(0.02f,  0.45f, release * 0.42f + 0.02f);

        const float xAtt  = a;
        const float xHold = std::min(0.60f, xAtt + h);
        const float xDec  = std::min(0.86f, xHold + d);
        const float xRel  = std::min(0.98f, std::max(xDec + 0.04f, 1.0f - r * 0.55f));

        nodes.push_back({ 0.0f,  0.0f, -0.42f, 0 });
        nodes.push_back({ xAtt,  1.0f,  0.0f,  2 });
        if (h > 0.008f)
            nodes.push_back({ xHold, 1.0f, 0.48f, 0 });
        else
            nodes.back().tension = 0.48f;
        nodes.push_back({ xDec,  s,     0.0f,  2 });
        if (xRel > xDec + 0.02f)
            nodes.push_back({ xRel, s,  0.52f, 0 });
        else
            nodes.back().tension = 0.52f;
        nodes.push_back({ 1.0f,  0.0f,  0.0f,  0 });

        rebuildLUT();
    }

    // Gross Beat / ShaperBox 3 Time-Modulation Curve Presets
    // Y-axis represents time delay offset behind live write-head (0.0 = real-time, 1.0 = max beat delay)
    void applyTimeModPreset(int presetIndex)
    {
        juce::ScopedLock sl(lock);
        nodes.clear();
        mode.store(static_cast<int>(ArticulatorMode::Loop), std::memory_order_relaxed);
        tempoSync.store(true, std::memory_order_relaxed);
        syncDivision.store(2, std::memory_order_relaxed); // 1 Bar

        switch (presetIndex)
        {
            case 0: // 1. HALF-TIME (1/2x Gross Beat Diagonal Ramp)
                nodes = {
                    { 0.00f, 0.00f, 0.0f, 2 },
                    { 0.49f, 0.49f, 0.0f, 2 },
                    { 0.50f, 0.00f, 0.0f, 2 },
                    { 1.00f, 0.50f, 0.0f, 2 }
                };
                break;

            case 1: // 2. 1/8 STUTTER REPEAT (Rhythmic Stepped Buffer Re-triggers)
                nodes = {
                    { 0.00f, 0.00f,  0.0f, 2 },
                    { 0.24f, 0.24f, -0.8f, 2 },
                    { 0.25f, 0.00f,  0.0f, 2 },
                    { 0.49f, 0.24f, -0.8f, 2 },
                    { 0.50f, 0.25f,  0.0f, 2 },
                    { 0.74f, 0.49f, -0.8f, 2 },
                    { 0.75f, 0.25f,  0.0f, 2 },
                    { 1.00f, 0.50f,  0.0f, 2 }
                };
                break;

            case 2: // 3. TAPE STOP (Exponential Deceleration Curve)
                nodes = {
                    { 0.00f, 0.00f,  0.00f, 2 },
                    { 0.50f, 0.00f, -0.62f, 0 },
                    { 0.96f, 0.88f,  0.75f, 0 },
                    { 1.00f, 0.00f,  0.00f, 2 }
                };
                break;

            case 3: // 4. REVERSE BAR (Steep Upward Time Offset = Reverse Playback)
                nodes = {
                    { 0.00f, 0.00f, 0.0f, 2 },
                    { 0.49f, 0.95f, 0.0f, 2 },
                    { 0.50f, 0.00f, 0.0f, 2 },
                    { 1.00f, 0.95f, 0.0f, 2 }
                };
                break;

            case 4: // 5. DJ SCRATCH (Turntable Back-and-Forth Scrubbing)
                nodes = {
                    { 0.00f, 0.00f, -0.35f, 1 },
                    { 0.18f, 0.55f,  0.35f, 1 },
                    { 0.34f, 0.10f, -0.40f, 1 },
                    { 0.52f, 0.75f,  0.40f, 1 },
                    { 0.72f, 0.15f, -0.35f, 1 },
                    { 0.88f, 0.60f,  0.35f, 1 },
                    { 1.00f, 0.00f,  0.00f, 1 }
                };
                break;

            case 5: // 6. TRIPLET GLITCH (3x Fast Time Warps + Pitch Jump)
                nodes = {
                    { 0.000f, 0.00f,  0.35f, 0 },
                    { 0.330f, 0.45f,  0.00f, 2 },
                    { 0.334f, 0.00f,  0.35f, 0 },
                    { 0.664f, 0.45f,  0.00f, 2 },
                    { 0.667f, 0.00f, -0.45f, 0 },
                    { 1.000f, 0.72f,  0.00f, 2 }
                };
                break;

            case 6: // 7. VINYL DROP & SPIN-UP
                nodes = {
                    { 0.00f, 0.00f, -0.65f, 0 },
                    { 0.42f, 0.82f,  0.65f, 0 },
                    { 0.78f, 0.04f,  0.00f, 2 },
                    { 1.00f, 0.00f,  0.00f, 2 }
                };
                break;

            case 7: // 8. CHAOS REPEAT (Rapid Micro-Stutter + Reverse Tail)
            default:
                nodes = {
                    { 0.00f, 0.00f, 0.0f,  2 },
                    { 0.12f, 0.12f, 0.0f,  2 },
                    { 0.13f, 0.00f, 0.0f,  2 },
                    { 0.25f, 0.12f, 0.0f,  2 },
                    { 0.26f, 0.00f, 0.0f,  2 },
                    { 0.38f, 0.12f, 0.0f,  2 },
                    { 0.40f, 0.00f, -0.5f, 0 },
                    { 0.75f, 0.85f,  0.5f, 0 },
                    { 1.00f, 0.00f, 0.0f,  2 }
                };
                break;
        }
        rebuildLUT();
    }

    static float applyTensionCurve(float t, float tension, int shapeMode = 0) noexcept
    {
        t = juce::jlimit(0.0f, 1.0f, t);

        // Mode 2: Linear
        if (shapeMode == 2)
            return t;

        // Mode 1: S-Curve (Sigmoid / Cosine with adjustable steepness via tension)
        if (shapeMode == 1)
        {
            const float cosineS = 0.5f - 0.5f * std::cos(t * juce::MathConstants<float>::pi);
            if (std::abs(tension) < 0.01f)
                return cosineS;

            // Steep logistic S-curve for positive tension, cubic inflection for negative tension
            if (tension > 0.0f)
            {
                const float k = 1.0f + tension * 10.0f;
                const float centered = (t - 0.5f) * 2.0f;
                const float sig = std::tanh(centered * k) / std::tanh(k);
                return 0.5f + 0.5f * sig;
            }
            else
            {
                const float amt = -tension;
                const float centered = (t - 0.5f) * 2.0f;
                const float invS = centered * centered * centered;
                const float raw = 0.5f + 0.5f * invS;
                return cosineS * (1.0f - amt) + raw * amt;
            }
        }

        // Mode 0: Concave / Convex Exponential Curve
        if (std::abs(tension) < 0.001f)
            return t;

        const float k = tension * 4.8f;
        const float denom = std::exp(k) - 1.0f;
        if (std::abs(denom) < 1e-5f)
            return t;
        return (std::exp(k * t) - 1.0f) / denom;
    }

    float evaluateNodesDirect(float x) const noexcept
    {
        if (nodes.empty())
            return 0.5f;
        if (x <= nodes.front().x)
            return nodes.front().y;
        if (x >= nodes.back().x)
            return nodes.back().y;

        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const auto& n0 = nodes[i];
            const auto& n1 = nodes[i + 1];
            if (x >= n0.x && x <= n1.x)
            {
                const float dx = n1.x - n0.x;
                if (dx <= 1e-6f)
                    return n1.y;
                const float localT = (x - n0.x) / dx;
                const float shapedT = applyTensionCurve(localT, n0.tension, n0.shapeMode);
                return n0.y + shapedT * (n1.y - n0.y);
            }
        }
        return nodes.back().y;
    }

    void rebuildLUT()
    {
        const int writeIdx = 1 - activeLutIndex.load(std::memory_order_relaxed);
        auto& table = lutBuffers[static_cast<size_t>(writeIdx)];
        for (int i = 0; i < LUT_SIZE; ++i)
        {
            const float x = static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1);
            table[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, evaluateNodesDirect(x));
        }
        activeLutIndex.store(writeIdx, std::memory_order_release);
    }

    // Lock-free evaluation on audio thread
    float evaluateLUT(float normalizedX) const noexcept
    {
        const float x = juce::jlimit(0.0f, 1.0f, normalizedX);
        const int readIdx = activeLutIndex.load(std::memory_order_acquire);
        const auto& table = lutBuffers[static_cast<size_t>(readIdx)];

        const float scaled = x * static_cast<float>(LUT_SIZE - 1);
        const int i0 = static_cast<int>(scaled);
        const int i1 = std::min(i0 + 1, LUT_SIZE - 1);
        const float frac = scaled - static_cast<float>(i0);

        return table[static_cast<size_t>(i0)] + frac * (table[static_cast<size_t>(i1)] - table[static_cast<size_t>(i0)]);
    }

    std::vector<SplineNode> getNodesCopy() const
    {
        juce::ScopedLock sl(lock);
        return nodes;
    }

    void setNodes(const std::vector<SplineNode>& newNodes)
    {
        juce::ScopedLock sl(lock);
        nodes = newNodes;
        std::sort(nodes.begin(), nodes.end(), [](const SplineNode& a, const SplineNode& b) {
            return a.x < b.x;
        });
        if (!nodes.empty())
        {
            nodes.front().x = 0.0f;
            nodes.back().x = 1.0f;
        }
        rebuildLUT();
    }

    void updateNode(int index, float x, float y)
    {
        juce::ScopedLock sl(lock);
        if (index < 0 || index >= static_cast<int>(nodes.size()))
            return;

        if (index == 0)
            nodes[0].x = 0.0f;
        else if (index == static_cast<int>(nodes.size()) - 1)
            nodes.back().x = 1.0f;
        else
        {
            const float minX = nodes[static_cast<size_t>(index - 1)].x + 0.01f;
            const float maxX = nodes[static_cast<size_t>(index + 1)].x - 0.01f;
            nodes[static_cast<size_t>(index)].x = juce::jlimit(minX, maxX, x);
        }

        nodes[static_cast<size_t>(index)].y = juce::jlimit(0.0f, 1.0f, y);
        rebuildLUT();
    }

    void updateTension(int index, float tension)
    {
        juce::ScopedLock sl(lock);
        if (index < 0 || index + 1 >= static_cast<int>(nodes.size()))
            return;
        nodes[static_cast<size_t>(index)].tension = juce::jlimit(-0.95f, 0.95f, tension);
        rebuildLUT();
    }

    void cycleSegmentShapeMode(int index)
    {
        juce::ScopedLock sl(lock);
        if (index < 0 || index + 1 >= static_cast<int>(nodes.size()))
            return;
        nodes[static_cast<size_t>(index)].shapeMode = (nodes[static_cast<size_t>(index)].shapeMode + 1) % 3;
        rebuildLUT();
    }

    void setAllSegmentsShapeMode(int shapeMode)
    {
        juce::ScopedLock sl(lock);
        const int clamped = juce::jlimit(0, 2, shapeMode);
        globalShapeMode.store(clamped, std::memory_order_relaxed);
        for (auto& n : nodes)
        {
            n.shapeMode = clamped;
            if (clamped == 2)
                n.tension = 0.0f;
        }
        rebuildLUT();
    }

    void nudgeAllSegmentsTension(float delta)
    {
        juce::ScopedLock sl(lock);
        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            if (nodes[i].shapeMode == 2)
                nodes[i].shapeMode = 0;
            const float dir = (nodes[i + 1].y >= nodes[i].y) ? 1.0f : -1.0f;
            nodes[i].tension = juce::jlimit(-0.95f, 0.95f, nodes[i].tension + delta * dir);
        }
        rebuildLUT();
    }

    int getGlobalShapeMode() const noexcept { return globalShapeMode.load(std::memory_order_relaxed); }

    int addNode(float x, float y)
    {
        juce::ScopedLock sl(lock);
        if (nodes.size() >= 18)
            return -1;

        SplineNode n;
        n.x = juce::jlimit(0.015f, 0.985f, x);
        n.y = juce::jlimit(0.0f, 1.0f, y);
        n.tension = 0.0f;
        n.shapeMode = globalShapeMode.load(std::memory_order_relaxed);

        nodes.push_back(n);
        std::sort(nodes.begin(), nodes.end(), [](const SplineNode& a, const SplineNode& b) {
            return a.x < b.x;
        });
        rebuildLUT();

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (std::abs(nodes[i].x - n.x) < 1e-4f && std::abs(nodes[i].y - n.y) < 1e-4f)
                return static_cast<int>(i);
        }
        return -1;
    }

    void removeNode(int index)
    {
        juce::ScopedLock sl(lock);
        if (index <= 0 || index >= static_cast<int>(nodes.size()) - 1)
            return; // Keep endpoints
        nodes.erase(nodes.begin() + index);
        rebuildLUT();
    }

    void setMode(ArticulatorMode m) noexcept { mode.store(static_cast<int>(m), std::memory_order_release); }
    ArticulatorMode getMode() const noexcept { return static_cast<ArticulatorMode>(mode.load(std::memory_order_acquire)); }

    void setRate(float r) noexcept { rate.store(juce::jlimit(0.05f, 16.0f, r), std::memory_order_release); }
    float getRate() const noexcept { return rate.load(std::memory_order_acquire); }

    void setTempoSync(bool sync) noexcept { tempoSync.store(sync, std::memory_order_release); }
    bool isTempoSync() const noexcept { return tempoSync.load(std::memory_order_acquire); }

    void setSyncDivision(int div) noexcept { syncDivision.store(juce::jlimit(0, 9, div), std::memory_order_release); }
    int getSyncDivision() const noexcept { return syncDivision.load(std::memory_order_acquire); }

    float getEffectiveRateHz(double hostBpm) const noexcept
    {
        if (tempoSync.load(std::memory_order_acquire))
            return divisionToHz(syncDivision.load(std::memory_order_acquire), hostBpm);
        return rate.load(std::memory_order_acquire);
    }

    juce::ValueTree toValueTree(const juce::Identifier& treeId) const
    {
        juce::ScopedLock sl(lock);
        juce::ValueTree vt(treeId);
        vt.setProperty("mode", mode.load(std::memory_order_acquire), nullptr);
        vt.setProperty("rate", rate.load(std::memory_order_acquire), nullptr);
        vt.setProperty("tempoSync", tempoSync.load(std::memory_order_acquire), nullptr);
        vt.setProperty("syncDivision", syncDivision.load(std::memory_order_acquire), nullptr);
        vt.setProperty("globalShapeMode", globalShapeMode.load(std::memory_order_acquire), nullptr);
        for (const auto& n : nodes)
        {
            juce::ValueTree nodeVt("Node");
            nodeVt.setProperty("x", n.x, nullptr);
            nodeVt.setProperty("y", n.y, nullptr);
            nodeVt.setProperty("tension", n.tension, nullptr);
            nodeVt.setProperty("shapeMode", n.shapeMode, nullptr);
            vt.appendChild(nodeVt, nullptr);
        }
        return vt;
    }

    void fromValueTree(const juce::ValueTree& vt)
    {
        if (!vt.isValid())
            return;
        juce::ScopedLock sl(lock);
        mode.store(static_cast<int>(vt.getProperty("mode", 0)), std::memory_order_release);
        rate.store(static_cast<float>(vt.getProperty("rate", 1.0f)), std::memory_order_release);
        tempoSync.store(static_cast<bool>(vt.getProperty("tempoSync", false)), std::memory_order_release);
        syncDivision.store(static_cast<int>(vt.getProperty("syncDivision", 2)), std::memory_order_release);
        globalShapeMode.store(static_cast<int>(vt.getProperty("globalShapeMode", 0)), std::memory_order_release);

        std::vector<SplineNode> loaded;
        for (int i = 0; i < vt.getNumChildren(); ++i)
        {
            auto child = vt.getChild(i);
            if (child.hasType("Node"))
            {
                SplineNode n;
                n.x = static_cast<float>(child.getProperty("x", 0.0f));
                n.y = static_cast<float>(child.getProperty("y", 0.5f));
                n.tension = static_cast<float>(child.getProperty("tension", 0.0f));
                n.shapeMode = static_cast<int>(child.getProperty("shapeMode", 0));
                loaded.push_back(n);
            }
        }
        if (loaded.size() >= 2)
        {
            nodes = loaded;
            rebuildLUT();
        }
    }

private:
    mutable juce::CriticalSection lock;
    std::vector<SplineNode> nodes;
    std::array<std::array<float, LUT_SIZE>, 2> lutBuffers {};
    std::atomic<int> activeLutIndex { 0 };
    std::atomic<int> mode { static_cast<int>(ArticulatorMode::Loop) };
    std::atomic<float> rate { 1.5f };
    std::atomic<bool> tempoSync { false };
    std::atomic<int> syncDivision { 4 }; // Default: 1/4
    std::atomic<int> globalShapeMode { 0 };
};

} // namespace PrismorDSP
