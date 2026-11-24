#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace PrismorDSP
{

// 13 Modular FX Rack Units (Dynamic Reorderable Chain)
enum class FxModuleId : int
{
    Blur = 0,
    Tremolo = 1,
    Distortion = 2,
    Phaser = 3,
    Chorus = 4,
    ParametricEQ = 5,
    Delay = 6,
    Reverb = 7,
    Compressor = 8,
    RingMod = 9,     // Zenology Modulation Pack: Ring Modulator / Hilbert Freq Shifter / Auto-Pan / Rotary
    Bitcrusher = 10, // Zenology Lo-Fi Pack: Bitcrusher & Sample-Rate Decimator
    Flanger = 11,    // Zenology Flanger Pack: Through-Zero / Jet / Barberpole / Step Flanger
    Bode = 12,       // Bode Frequency Shifter: Linear Hilbert SSB Frequency Shifter
    Count = 13
};

struct ArpStep
{
    bool  enabled = true;
    float velocity = 0.85f;      // [0.0 .. 1.0]
    int   octaveOffset = 0;      // [-1 .. +2]
    int   semitoneTranspose = 0; // [-12 .. +12]
};

struct SynthParameters
{
    double hostBpm = 120.0;

    // 1. Frequency Field 1 (Part A Oscillator / Pitch / Warp / Scale / Tilt)
    int   oscAOct        = 0;
    int   oscASemi       = 0;
    float oscAFine       = 0.0f;
    float oscACoarse     = 0.0f;

    float startPhase     = 0.0f;
    float phaseRandom    = 0.0f;
    float timbre         = 0.0f;
    int   warpMode       = 0;
    float warpAmount     = 0.0f;
    float harmonicScale  = 1.0f;
    float harmonicTilt   = 0.0f;

    // 2. Frequency Field 2 (Part B Oscillator / Pitch / Warp / Scale / Tilt)
    int   oscBOct        = 0;
    int   oscBSemi       = 0;
    float oscBFine       = 0.0f;
    float oscBPitch      = 0.0f;

    float oscBPhase      = 0.0f;
    float timbreB        = 0.0f;
    int   warpModeB      = 0;
    float warpAmountB    = 0.0f;
    float harmonicScaleB = 1.0f;
    float harmonicTiltB  = 0.0f;

    // 3. Prism, Harmonizer & Pluck
    bool  prismEnable    = true;
    float prismAmount    = 0.0f;
    int   prismMode      = 0;
    float harmShift      = 0.0f;
    float harmGap        = 0.25f;
    float harmWidth      = 0.45f;

    bool  pluckEnable    = true;
    float pluckTime      = 1.0f;
    float pluckBlur      = 0.30f;

    // 4. Voicing, True Legato, Glide & 16x Unison Engine
    int   voicingMode    = 0;
    int   maxVoices      = 8;
    float pitchFreq      = 0.0f;
    float portaTime      = 0.0f;
    float portaCurve     = 0.0f;
    bool  autoGlide      = true;

    int   unisonVoices    = 1;
    float unisonDetune    = 0.18f;
    float unisonSpread    = 0.65f;
    float unisonBlend     = 0.55f;
    float unisonPhase     = 0.35f;
    int   unisonStackMode = 0;

    float vibDepth       = 0.0f;
    float vibSpeed       = 5.5f;

    // 5. Dual Spectral Filter & Drive
    bool  filter1Enable   = true;
    int   filter1Type     = 1;
    float filter1Cutoff   = 1.0f;
    float filter1Res      = 0.15f;
    float filter1KeyTrack = 0.50f;
    float filter1Width    = 0.45f;

    bool  filter2Enable   = false;
    int   filter2Type     = 2;
    float filter2Cutoff   = 0.85f;
    float filter2Res      = 0.20f;
    float filter2KeyTrack = 0.50f;
    float filter2Width    = 0.50f;

    float filterRouting   = 0.0f;
    float filterDrive     = 0.0f;

    // 6. Noise Engine
    bool  noiseEnable     = false;
    int   noiseType       = 0;
    float noiseLevel      = 0.25f;
    float noisePitch      = 0.50f;
    float noiseKeyTrack   = 0.50f;
    float noisePhase      = 0.25f;
    float noisePan        = 0.0f;
    float noiseFilter     = 0.0f;

    // 7. Arpeggiator
    bool  arpEnable       = false;
    int   arpMode         = 0;
    int   arpOctaves      = 1;
    int   arpRate         = 3;
    float arpGate         = 0.75f;
    float arpSwing        = 0.0f;

    // 8. Multi-FX Rack (13 Modules)
    bool  blurEnable      = true;
    float blurTime        = 0.0f;
    float blurHarmonic    = 0.0f;

    bool  tremEnable      = true;
    float tremDepth       = 0.0f;
    float tremSpeed       = 3.5f;
    float tremGap         = 0.35f;

    bool  distEnable      = false;
    int   distType        = 0;
    float distDrive       = 0.35f;
    float distTone        = 0.65f;
    float distMix         = 0.50f;

    bool  phaserEnable    = true;
    float phaserMix       = 0.0f;
    float phaserWidth     = 0.50f;
    float phaserOffset    = 0.35f;
    float phaserSpeed     = 1.2f;

    bool  chorusEnable    = false;
    int   chorusVoices    = 4;
    float chorusRate      = 0.85f;
    float chorusDepth     = 0.45f;
    float chorusSpread    = 0.80f;
    float chorusMix       = 0.35f;

    bool  eqEnable        = true;
    float eqLowFreq       = 110.0f;
    float eqLowGain       = 0.0f;
    float eqMidFreq       = 1200.0f;
    float eqMidGain       = 0.0f;
    float eqMidQ          = 1.2f;
    float eqHighFreq      = 5500.0f;
    float eqHighGain      = 0.0f;

    bool  delayEnable     = false;
    float delayTime       = 0.35f;
    float delayFeedback   = 0.40f;
    float delayPingPong   = 0.65f;
    float delayFilter     = 0.60f;
    float delayMix        = 0.28f;

    bool  reverbEnable    = false;
    float reverbSize      = 0.68f;
    float reverbDecay     = 0.55f;
    float reverbPreDelay  = 0.18f;
    float reverbDamping   = 0.40f;
    float reverbShimmer   = 0.25f;
    float reverbMix       = 0.25f;

    bool  compEnable      = false;
    int   compMode        = 1;
    float compThreshold   = -18.0f;
    float compRatio       = 4.0f;
    float compAttack      = 10.0f;
    float compRelease     = 120.0f;
    float compMakeup      = 3.0f;
    float compMix         = 0.75f;

    bool  ringEnable      = false;
    int   ringMode        = 0;
    float ringFreq        = 220.0f;
    float ringDepth       = 0.70f;
    float ringLfoRate     = 1.20f;
    float ringLfoDepth    = 0.25f;
    float ringMix         = 0.50f;

    bool  crushEnable     = false;
    int   crushMode       = 0;
    float crushBits       = 8.0f;
    float crushDownsample = 4.0f;
    float crushDrive      = 0.25f;
    float crushTone       = 0.65f;
    float crushMix        = 0.55f;

    bool  flangerEnable   = false;
    int   flangerMode     = 0;
    float flangerRate     = 0.45f;
    float flangerDepth    = 0.65f;
    float flangerFeedback = 0.55f;
    float flangerManual   = 0.35f;
    float flangerMix      = 0.50f;

    bool  bodeEnable      = false;
    float bodeShift       = 0.0f;
    float bodeFeedback    = 0.0f;
    float bodeMix         = 0.50f;

    // 9. Time Modulation Engine
    bool  timeModEnable   = false;
    int   timeModMode     = 0;
    int   timeModRate     = 1;
    float timeModDepth    = 0.75f;
    float timeModStutter  = 0.35f;
    float timeModSpeed    = 0.0f;
    float timeModSmooth   = 0.35f;
    float timeModFilter   = 0.0f;
    float timeModMix      = 0.85f;

    // 10. AHDSR Envelope
    float envAttack       = 0.05f;
    float envHold         = 0.05f;
    float envDecay        = 0.35f;
    float envSustain      = 0.72f;
    float envRelease      = 0.25f;

    // 11. A/B Layering, Strum & Master Output / Limiter
    float abMix           = 0.0f;
    float velLink         = 0.65f;
    float partPan         = 0.0f;
    int   activeEditPart  = 0;

    float strumTime       = 0.0f;
    float strumTension    = 0.0f;

    float masterGainDb    = 0.0f;
    bool  limiterEnable   = true;
    float limiterThreshDb = -0.5f;
};

struct VisualizerSnapshot
{
    static constexpr int NUM_BINS = 96;
    std::array<float, NUM_BINS> freqNormA {};
    std::array<float, NUM_BINS> freqNormB {};
    std::array<float, NUM_BINS> ampA {};
    std::array<float, NUM_BINS> ampB {};
    std::array<float, NUM_BINS> filterCurve {};
    std::array<float, 6> envCursors {};
    float envCursor = 0.0f;
    float activePitchHz = 130.81f;
    int   activeVoiceCount = 0;
    int   activeArpStep = 0;
    bool  voiceActive = false;

    float timeModCursor     = 0.0f;
    float timeModOffsetNorm = 0.0f;
    std::array<float, 64> timeModWavePreview {};

    float peakLeft  = 0.0f;
    float peakRight = 0.0f;
    float rmsLeft   = 0.0f;
    float rmsRight  = 0.0f;
    float limiterGainReductionDb = 0.0f;
    float compGainReductionDb    = 0.0f;
};

// Fixed-capacity stack container for zero-allocation audio-thread note/arp lists
template <typename T, size_t Capacity>
class FixedList
{
public:
    void clear() noexcept { count = 0; }
    bool empty() const noexcept { return count == 0; }
    size_t size() const noexcept { return count; }
    static constexpr size_t capacity() noexcept { return Capacity; }

    bool push_back(const T& val) noexcept
    {
        if (count >= Capacity)
            return false;
        data[count++] = val;
        return true;
    }

    const T& back() const noexcept { return data[count > 0 ? count - 1 : 0]; }
    T& back() noexcept { return data[count > 0 ? count - 1 : 0]; }

    const T& operator[](size_t i) const noexcept { return data[i]; }
    T& operator[](size_t i) noexcept { return data[i]; }

    T* begin() noexcept { return data.data(); }
    T* end() noexcept { return data.data() + count; }
    const T* begin() const noexcept { return data.data(); }
    const T* end() const noexcept { return data.data() + count; }

    template <typename Pred>
    void eraseIf(Pred&& pred) noexcept
    {
        size_t w = 0;
        for (size_t r = 0; r < count; ++r)
        {
            if (!pred(data[r]))
                data[w++] = data[r];
        }
        count = w;
    }

private:
    std::array<T, Capacity> data {};
    size_t count = 0;
};

inline int nextPowerOfTwo(int n) noexcept
{
    int p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

inline float sampleRateScaledAlpha(float cutoffHz, double sampleRate) noexcept
{
    const float sr = static_cast<float>(std::max(8000.0, sampleRate));
    const float fc = juce::jlimit(5.0f, sr * 0.45f, cutoffHz);
    return 1.0f - std::exp(-juce::MathConstants<float>::twoPi * fc / sr);
}

inline float sanitizeSample(float x) noexcept
{
    return std::isfinite(x) ? x : 0.0f;
}

inline float softClipC1(float x, float thresh = 0.90f, float ceiling = 1.0f) noexcept
{
    if (!std::isfinite(x))
        return 0.0f;
    const float absX = std::abs(x);
    if (absX <= thresh)
        return x;
    const float margin = std::max(0.01f, ceiling - thresh);
    const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
    return sign * std::min(ceiling, thresh + margin * std::tanh((absX - thresh) / margin));
}

struct DcBlocker
{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float r  = 0.9983f;

    void prepare(double sampleRate, float cutoffHz = 12.0f) noexcept
    {
        const double sr = std::max(8000.0, sampleRate);
        r = static_cast<float>(std::exp(-2.0 * juce::MathConstants<double>::pi * static_cast<double>(cutoffHz) / sr));
        r = juce::jlimit(0.90f, 0.9999f, r);
    }

    void reset() noexcept { x1 = y1 = 0.0f; }

    inline float process(float x) noexcept
    {
        const float y = x - x1 + r * y1;
        x1 = x;
        y1 = std::isfinite(y) ? y : 0.0f;
        return y1;
    }
};

// Wideband 90-degree Hilbert All-Pass Network (6+6 equiripple sections, >60 dB SSB rejection)
struct BodeAllpassSection
{
    double c = 0.0;
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;

    void reset() noexcept { x1 = x2 = y1 = y2 = 0.0; }

    inline float process(float x) noexcept
    {
        const double xd = static_cast<double>(x);
        const double y = c * (xd + y2) - x2;
        x2 = x1;
        x1 = xd;
        y2 = y1;
        y1 = std::isfinite(y) ? y : 0.0;
        return static_cast<float>(y1);
    }
};

struct BodeHilbertPair
{
    BodeAllpassSection path1[6];
    BodeAllpassSection path2[6];
    float delay1 = 0.0f;

    BodeHilbertPair() noexcept { reset(); }

    void reset() noexcept
    {
        static constexpr double c1[6] = {
            0.273441384034312,
            0.676080350839039,
            0.886086052400462,
            0.963440195924904,
            0.989125024674536,
            0.998424750221192
        };
        static constexpr double c2[6] = {
            0.079559134975939,
            0.493062390673500,
            0.804523768425927,
            0.935002510822597,
            0.979745259460972,
            0.994712018350454
        };

        for (int i = 0; i < 6; ++i)
        {
            path1[i].c = c1[i];
            path1[i].reset();
            path2[i].c = c2[i];
            path2[i].reset();
        }
        delay1 = 0.0f;
    }

    inline void process(float x, float& outI, float& outQ) noexcept
    {
        float y1 = x;
        for (int i = 0; i < 6; ++i)
            y1 = path1[i].process(y1);
        outI = delay1;
        delay1 = y1;

        float y2 = x;
        for (int i = 0; i < 6; ++i)
            y2 = path2[i].process(y2);
        outQ = y2;
    }
};

// Direct Form II Transposed Stereo Biquad Filter (Audio EQ Cookbook)
struct StereoBiquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1L = 0.0f, z2L = 0.0f;
    float z1R = 0.0f, z2R = 0.0f;

    void reset() noexcept { z1L = z2L = z1R = z2R = 0.0f; }

    void setBypass() noexcept
    {
        b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f;
    }

    void setLowShelf(double sampleRate, float freqHz, float gainDb) noexcept
    {
        if (std::abs(gainDb) < 0.02f) { setBypass(); return; }
        const double sr = std::max(8000.0, sampleRate);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * juce::jlimit(20.0, sr * 0.42, static_cast<double>(freqHz)) / sr;
        const double A  = std::pow(10.0, juce::jlimit(-24.0, 24.0, static_cast<double>(gainDb)) / 40.0);
        const double sinW = std::sin(w0);
        const double cosW = std::cos(w0);
        const double alpha = sinW * 0.7071067811865475; // S = 1 -> 1/sqrt(2)
        const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;

        const double a0 = (A + 1.0) + (A - 1.0) * cosW + twoSqrtAAlpha;
        const double invA0 = 1.0 / std::max(1e-12, a0);
        b0 = static_cast<float>(A * ((A + 1.0) - (A - 1.0) * cosW + twoSqrtAAlpha) * invA0);
        b1 = static_cast<float>(2.0 * A * ((A - 1.0) - (A + 1.0) * cosW) * invA0);
        b2 = static_cast<float>(A * ((A + 1.0) - (A - 1.0) * cosW - twoSqrtAAlpha) * invA0);
        a1 = static_cast<float>((-2.0 * ((A - 1.0) + (A + 1.0) * cosW)) * invA0);
        a2 = static_cast<float>(((A + 1.0) + (A - 1.0) * cosW - twoSqrtAAlpha) * invA0);
    }

    void setPeak(double sampleRate, float freqHz, float q, float gainDb) noexcept
    {
        if (std::abs(gainDb) < 0.02f) { setBypass(); return; }
        const double sr = std::max(8000.0, sampleRate);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * juce::jlimit(30.0, sr * 0.44, static_cast<double>(freqHz)) / sr;
        const double A  = std::pow(10.0, juce::jlimit(-24.0, 24.0, static_cast<double>(gainDb)) / 40.0);
        const double sinW = std::sin(w0);
        const double cosW = std::cos(w0);
        const double safeQ = juce::jlimit(0.25, 12.0, static_cast<double>(q));
        const double alpha = sinW / (2.0 * safeQ);

        const double a0 = 1.0 + alpha / A;
        const double invA0 = 1.0 / std::max(1e-12, a0);
        b0 = static_cast<float>((1.0 + alpha * A) * invA0);
        b1 = static_cast<float>((-2.0 * cosW) * invA0);
        b2 = static_cast<float>((1.0 - alpha * A) * invA0);
        a1 = static_cast<float>((-2.0 * cosW) * invA0);
        a2 = static_cast<float>((1.0 - alpha / A) * invA0);
    }

    void setHighShelf(double sampleRate, float freqHz, float gainDb) noexcept
    {
        if (std::abs(gainDb) < 0.02f) { setBypass(); return; }
        const double sr = std::max(8000.0, sampleRate);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * juce::jlimit(100.0, sr * 0.44, static_cast<double>(freqHz)) / sr;
        const double A  = std::pow(10.0, juce::jlimit(-24.0, 24.0, static_cast<double>(gainDb)) / 40.0);
        const double sinW = std::sin(w0);
        const double cosW = std::cos(w0);
        const double alpha = sinW * 0.7071067811865475;
        const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;

        const double a0 = (A + 1.0) - (A - 1.0) * cosW + twoSqrtAAlpha;
        const double invA0 = 1.0 / std::max(1e-12, a0);
        b0 = static_cast<float>(A * ((A + 1.0) + (A - 1.0) * cosW + twoSqrtAAlpha) * invA0);
        b1 = static_cast<float>(-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW) * invA0);
        b2 = static_cast<float>(A * ((A + 1.0) + (A - 1.0) * cosW - twoSqrtAAlpha) * invA0);
        a1 = static_cast<float>((2.0 * ((A - 1.0) - (A + 1.0) * cosW)) * invA0);
        a2 = static_cast<float>(((A + 1.0) - (A - 1.0) * cosW - twoSqrtAAlpha) * invA0);
    }

    inline void processSample(float& sL, float& sR) noexcept
    {
        const float outL = b0 * sL + z1L;
        z1L = b1 * sL - a1 * outL + z2L;
        z2L = b2 * sL - a2 * outL;

        const float outR = b0 * sR + z1R;
        z1R = b1 * sR - a1 * outR + z2R;
        z2R = b2 * sR - a2 * outR;

        sL = std::isfinite(outL) ? outL : 0.0f;
        sR = std::isfinite(outR) ? outR : 0.0f;
        if (!std::isfinite(z1L) || !std::isfinite(z2L)) z1L = z2L = 0.0f;
        if (!std::isfinite(z1R) || !std::isfinite(z2R)) z1R = z2R = 0.0f;
    }
};

} // namespace PrismorDSP
