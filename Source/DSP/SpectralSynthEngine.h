#pragma once
#include <JuceHeader.h>
#include "SplineCurve.h"
#include "ResynthesisEngine.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

namespace PrismorDSP
{

// 12 Modular FX Rack Units (Serum-2 + Zenology Style Dynamic FX Rack)
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
    RingMod = 9,     // Zenology Modulation Pack: Ring Modulator / Freq Shifter / Auto-Pan
    Bitcrusher = 10, // Zenology Lo-Fi Pack: Bitcrusher & Sample-Rate Decimator
    Flanger = 11,    // Zenology Flanger Pack: Through-Zero / Jet Stereo Flanger
    Count = 12
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

    // -------------------------------------------------------------------------
    // 1. Frequency Field 1 (Part A Oscillator / Pitch / Warp / Scale / Tilt)
    // -------------------------------------------------------------------------
    int   oscAOct        = 0;      // -4..+4 Octaves
    int   oscASemi       = 0;      // -12..+12 Semitones
    float oscAFine       = 0.0f;   // -100..+100 Cents
    float oscACoarse     = 0.0f;   // -24..+24 Continuous Semitones

    float startPhase     = 0.0f;   // 0..1
    float phaseRandom    = 0.0f;   // 0..1
    float timbre         = 0.0f;   // 0..1 (Morphs Part A Harmonic Table -> Resynth Plane)
    int   warpMode       = 0;      // 0: Off, 1: Bend, 2: Mirror, 3: Quantize, 4: Asymmetric, 5: Sync Fold
    float warpAmount     = 0.0f;   // -1..+1
    float harmonicScale  = 1.0f;   // 0.25..4.0
    float harmonicTilt   = 0.0f;   // -1..+1 (-12dB/oct to +12dB/oct)

    // -------------------------------------------------------------------------
    // 2. Frequency Field 2 (Part B Independent Oscillator / Pitch / Warp / Scale)
    // -------------------------------------------------------------------------
    int   oscBOct        = 0;      // -4..+4 Octaves
    int   oscBSemi       = 0;      // -12..+12 Semitones
    float oscBFine       = 0.0f;   // -100..+100 Cents
    float oscBPitch      = 0.0f;   // -24..+24 Continuous Coarse Semitones

    float oscBPhase      = 0.0f;   // 0..1 start phase for Field 2
    float timbreB        = 0.0f;   // 0..1 (Morphs Part B Harmonic Table -> Resynth Plane)
    int   warpModeB      = 0;      // 0: Off, 1: Bend, 2: Mirror, 3: Quantize, 4: Asymmetric, 5: Sync Fold
    float warpAmountB    = 0.0f;   // -1..+1
    float harmonicScaleB = 1.0f;   // 0.25..4.0
    float harmonicTiltB  = 0.0f;   // -1..+1

    // -------------------------------------------------------------------------
    // 3. Prism, Harmonizer & Pluck (Spectral Synthesis Modifiers)
    // -------------------------------------------------------------------------
    bool  prismEnable    = true;
    float prismAmount    = 0.0f;   // 0..1
    int   prismMode      = 0;      // 0: Inharmonic, 1: Hz Shift, 2: Odd/Even, 3: Quantum
    float harmShift      = 0.0f;   // 0..1
    float harmGap        = 0.25f;  // 0..1
    float harmWidth      = 0.45f;  // 0..1

    bool  pluckEnable    = true;
    float pluckTime      = 1.0f;   // 1.0 = full sustain, lower = shorter spectral pluck
    float pluckBlur      = 0.30f;  // 0..1

    // -------------------------------------------------------------------------
    // 4. Voicing, True Legato, Glide & Serum-2 Style 16x Unison Engine
    // -------------------------------------------------------------------------
    int   voicingMode    = 0;      // 0: Polyphonic, 1: Monophonic, 2: True Legato
    int   maxVoices      = 8;      // 1..16
    float pitchFreq      = 0.0f;   // -24..+24 semitones master pitch
    float portaTime      = 0.0f;   // 0..1
    float portaCurve     = 0.0f;   // -1..+1
    bool  autoGlide      = true;   // Fingered portamento

    int   unisonVoices    = 1;     // 1..16
    float unisonDetune    = 0.18f; // 0..1
    float unisonSpread    = 0.65f; // 0..1 (Stereo Pan Spread)
    float unisonBlend     = 0.55f; // 0..1 (Outer vs Center Voice Balance)
    float unisonPhase     = 0.35f; // 0..1 (Initial Phase Spread)
    int   unisonStackMode = 0;     // 0: Normal, 1: +1 Oct, 2: +2 Oct, 3: Sub+Oct, 4: Fifths, 5: Oct+5th, 6: Min Triad, 7: Maj Triad

    float vibDepth       = 0.0f;   // 0..1
    float vibSpeed       = 5.5f;   // 0.5..15 Hz

    // -------------------------------------------------------------------------
    // 5. Dual Spectral Filter & Drive (10 Topologies incl. Vox, Vocoder, Talkbox)
    // -------------------------------------------------------------------------
    bool  filter1Enable   = true;
    int   filter1Type     = 1;      // 0..9
    float filter1Cutoff   = 1.0f;   // 0..1
    float filter1Res      = 0.15f;  // 0..1
    float filter1KeyTrack = 0.50f;  // 0..1
    float filter1Width    = 0.45f;  // 0..1

    bool  filter2Enable   = false;
    int   filter2Type     = 2;      // Crude Low Pass
    float filter2Cutoff   = 0.85f;  // 0..1
    float filter2Res      = 0.20f;  // 0..1
    float filter2KeyTrack = 0.50f;  // 0..1
    float filter2Width    = 0.50f;  // 0..1

    float filterRouting   = 0.0f;   // 0 = Serial, 1 = Parallel
    float filterDrive     = 0.0f;   // 0..1 (Drive / Saturation)

    // -------------------------------------------------------------------------
    // 6. Dedicated Serum-2 Style Noise Engine
    // -------------------------------------------------------------------------
    bool  noiseEnable     = false;
    int   noiseType       = 0;      // 0: White, 1: Pink, 2: Brown, 3: Vinyl Crackle, 4: Digital Bitnoise, 5: Breath/Air
    float noiseLevel      = 0.25f;  // 0..1
    float noisePitch      = 0.50f;  // 0..1
    float noiseKeyTrack   = 0.50f;  // 0..1
    float noisePhase      = 0.25f;  // 0..1
    float noisePan        = 0.0f;   // -1..+1
    float noiseFilter     = 0.0f;   // -1..+1 (<0 = Lowpass, >0 = Highpass)

    // -------------------------------------------------------------------------
    // 7. Built-in Arpeggiator & 16-Step Sequencer (Serum 2 / u-he Hive 2 Style)
    // -------------------------------------------------------------------------
    bool  arpEnable       = false;
    int   arpMode         = 0;      // 0: Up, 1: Down, 2: Up/Down, 3: Random, 4: Chord, 5: Step Seq
    int   arpOctaves      = 1;      // 1..4 Octaves
    int   arpRate         = 3;      // 0: 1/4, 1: 1/8, 2: 1/8T, 3: 1/16, 4: 1/16T, 5: 1/32
    float arpGate         = 0.75f;  // 0.05..1.0
    float arpSwing        = 0.0f;   // 0.0..0.75

    // -------------------------------------------------------------------------
    // 8. Dedicated FX Tab: 9 Dynamic Modular Multi-FX Units
    // -------------------------------------------------------------------------
    // FX 0: Spectral Blur
    bool  blurEnable      = true;
    float blurTime        = 0.0f;   // 0..1
    float blurHarmonic    = 0.0f;   // 0..1

    // FX 1: Harmonic Tremolo
    bool  tremEnable      = true;
    float tremDepth       = 0.0f;   // 0..1
    float tremSpeed       = 3.5f;   // 0.1..15 Hz
    float tremGap         = 0.35f;  // 0..1

    // FX 2: Distortion / Saturation (Tube, Tape, Hard Clip, Wavefold, Bitcrush, Asym Scream)
    bool  distEnable      = false;
    int   distType        = 0;      // 0: Tube, 1: Tape, 2: Hard Clip, 3: Wavefold, 4: Bitcrush Fuzz, 5: Asym Scream
    float distDrive       = 0.35f;  // 0..1
    float distTone        = 0.65f;  // 0..1 (Post-filter tone)
    float distMix         = 0.50f;  // 0..1

    // FX 3: Spectral & All-Pass Phaser
    bool  phaserEnable    = true;
    float phaserMix       = 0.0f;   // 0..1
    float phaserWidth     = 0.50f;  // 0..1
    float phaserOffset    = 0.35f;  // 0..1
    float phaserSpeed     = 1.2f;   // 0.05..10 Hz

    // FX 4: Multi-Voice Stereo Chorus
    bool  chorusEnable    = false;
    int   chorusVoices    = 4;      // 2..6
    float chorusRate      = 0.85f;  // 0.05..5.0 Hz
    float chorusDepth     = 0.45f;  // 0..1
    float chorusSpread    = 0.80f;  // 0..1
    float chorusMix       = 0.35f;  // 0..1

    // FX 5: 3-Band Parametric EQ
    bool  eqEnable        = true;
    float eqLowFreq       = 110.0f; // 30..400 Hz
    float eqLowGain       = 0.0f;   // -15..+15 dB
    float eqMidFreq       = 1200.0f;// 200..6000 Hz
    float eqMidGain       = 0.0f;   // -15..+15 dB
    float eqMidQ          = 1.2f;   // 0.3..8.0
    float eqHighFreq      = 5500.0f;// 1500..16000 Hz
    float eqHighGain      = 0.0f;   // -15..+15 dB

    // FX 6: Tempo-Sync Stereo / Ping-Pong Delay
    bool  delayEnable     = false;
    float delayTime       = 0.35f;  // 0..1
    float delayFeedback   = 0.40f;  // 0..0.92
    float delayPingPong   = 0.65f;  // 0..1
    float delayFilter     = 0.60f;  // 0..1
    float delayMix        = 0.28f;  // 0..1

    // FX 7: Hall / Shimmer Reverb
    bool  reverbEnable    = false;
    float reverbSize      = 0.68f;  // 0..1
    float reverbDecay     = 0.55f;  // 0..1
    float reverbPreDelay  = 0.18f;  // 0..1
    float reverbDamping   = 0.40f;  // 0..1
    float reverbShimmer   = 0.25f;  // 0..1
    float reverbMix       = 0.25f;  // 0..1

    // FX 8: Single-Band & Multiband OTT Compressor
    bool  compEnable      = false;
    int   compMode        = 1;      // 0: Single-Band VCA, 1: Multiband OTT, 2: Opto Smooth, 3: Punch Limiter
    float compThreshold   = -18.0f; // -48..0 dB
    float compRatio       = 4.0f;   // 1.0..20.0
    float compAttack      = 10.0f;  // 0.1..100 ms
    float compRelease     = 120.0f; // 10..1000 ms
    float compMakeup      = 3.0f;   // 0..24 dB
    float compMix         = 0.75f;  // 0..1

    // FX 9: Zenology Modulation Pack (Ring Modulator / Freq Shifter / Auto-Pan / Rotary)
    bool  ringEnable      = false;
    int   ringMode        = 0;      // 0: Ring Mod, 1: Freq Shifter, 2: Stereo Auto-Pan, 3: Rotary Speaker
    float ringFreq        = 220.0f; // 20..2000 Hz Carrier Frequency
    float ringDepth       = 0.70f;  // 0..1 Carrier Depth
    float ringLfoRate     = 1.20f;  // 0.05..15.0 Hz LFO Rate
    float ringLfoDepth    = 0.25f;  // 0..1 LFO Pitch/Mod Depth
    float ringMix         = 0.50f;  // 0..1

    // FX 10: Zenology Lo-Fi Pack (Bitcrusher / Sample-Rate Decimator / Vintage SP)
    bool  crushEnable     = false;
    int   crushMode       = 0;      // 0: Digital Bitcrush, 1: Sample Decimate, 2: SP-1200 Vintage, 3: Radio Lo-Fi
    float crushBits       = 8.0f;   // 1.0..16.0 bits
    float crushDownsample = 4.0f;   // 1.0..64.0x Downsample Factor
    float crushDrive      = 0.25f;  // 0..1 Pre-saturation / Jitter
    float crushTone       = 0.65f;  // 0..1 Post Anti-Aliasing Tone Filter
    float crushMix        = 0.55f;  // 0..1

    // FX 11: Zenology Flanger Pack (Jet Flanger / Through-Zero / Barberpole / Step Flanger)
    bool  flangerEnable   = false;
    int   flangerMode     = 0;      // 0: Jet Flanger, 1: Through-Zero, 2: Barberpole, 3: Step Flanger
    float flangerRate     = 0.45f;  // 0.05..8.0 Hz
    float flangerDepth    = 0.65f;  // 0..1
    float flangerFeedback = 0.55f;  // -0.95..+0.95 Bipolar Feedback
    float flangerManual   = 0.35f;  // 0..1 Center Delay Offset
    float flangerMix      = 0.50f;  // 0..1

    // -------------------------------------------------------------------------
    // 9. Time Modulation Engine (Gross Beat / ShaperBox 3 Buffer Time-Shaping)
    // -------------------------------------------------------------------------
    bool  timeModEnable   = false;
    int   timeModMode     = 0;      // 0: Time Warp Curve, 1: Stutter Repeat, 2: Tape Stop, 3: Reverse Grain, 4: Half-Time 1/2x, 5: DJ Scratch
    int   timeModRate     = 1;      // 0: 2 Bars, 1: 1 Bar, 2: 1/2, 3: 1/4, 4: 1/8, 5: 1/16
    float timeModDepth    = 0.75f;  // 0..1 Time-warp span
    float timeModStutter  = 0.35f;  // 0..1 Stutter repeat density & gate
    float timeModSpeed    = 0.0f;   // -1..+1 Playback speed / Tape slope
    float timeModSmooth   = 0.35f;  // 0..1 De-click crossfade smoothing
    float timeModFilter   = 0.0f;   // -1..+1 Post-Time-Warp LP/HP Filter
    float timeModMix      = 0.85f;  // 0..1 Dry/Wet Mix

    // -------------------------------------------------------------------------
    // 10. Serum-2 Style Physical AHDSR Envelope Knobs (Graphical Articulator)
    // -------------------------------------------------------------------------
    float envAttack       = 0.05f;  // 0.001..1.0
    float envHold         = 0.05f;  // 0.0..1.0
    float envDecay        = 0.35f;  // 0.01..1.0
    float envSustain      = 0.72f;  // 0.0..1.0
    float envRelease      = 0.25f;  // 0.01..1.0

    // -------------------------------------------------------------------------
    // 11. A/B Layering, Strum & Master Output / Brickwall Limiter
    // -------------------------------------------------------------------------
    float abMix           = 0.0f;   // 0 = 100% Part A, 1 = 100% Part B
    float velLink         = 0.65f;  // 0..1
    float partPan         = 0.0f;   // -1..+1
    int   activeEditPart  = 0;      // 0 = Part A, 1 = Part B

    float strumTime       = 0.0f;   // 0..1
    float strumTension    = 0.0f;   // -1..+1

    float masterGainDb    = 0.0f;   // -24..+12 dB
    bool  limiterEnable   = true;
    float limiterThreshDb = -0.5f;  // -12..0 dB
};

struct VisualizerSnapshot
{
    static constexpr int NUM_BINS = 96;
    std::array<float, NUM_BINS> freqNormA {};   // Normalized log-freq Field 1 [0..1]
    std::array<float, NUM_BINS> freqNormB {};   // Normalized log-freq Field 2 [0..1]
    std::array<float, NUM_BINS> ampA {};        // Partial amplitude Field 1 (Part A) [0..1]
    std::array<float, NUM_BINS> ampB {};        // Partial amplitude Field 2 (Part B) [0..1]
    std::array<float, NUM_BINS> filterCurve {}; // Combined spectral filter + EQ transfer curve [0..1]
    std::array<float, 6> envCursors {};         // Live audio-thread cursor per ArticulatorTarget
    float envCursor = 0.0f;
    float activePitchHz = 130.81f;
    int   activeVoiceCount = 0;
    int   activeArpStep = 0;
    bool  voiceActive = false;

    // Time Modulation Live Buffer Telemetry
    float timeModCursor     = 0.0f;
    float timeModOffsetNorm = 0.0f;
    std::array<float, 64> timeModWavePreview {};

    // Master Output, Compressor & Limiter Telemetry
    float peakLeft  = 0.0f;
    float peakRight = 0.0f;
    float rmsLeft   = 0.0f;
    float rmsRight  = 0.0f;
    float limiterGainReductionDb = 0.0f;
    float compGainReductionDb    = 0.0f;
};

class SpectralSynthEngine
{
public:
    static constexpr int NUM_PARTIALS = 128;
    static constexpr int MAX_VOICES = 16;
    static constexpr int MAX_UNISON = 16;
    static constexpr int CONTROL_RATE_SAMPLES = 32;
    static constexpr int SINE_LUT_SIZE = 8192;
    static constexpr int MAX_DELAY_SAMPLES = 96000; // 2.0s at 48kHz
    static constexpr int CHORUS_BUFFER_SIZE = 4096;
    static constexpr int FLANGER_BUFFER_SIZE = 2048;
    static constexpr int TIME_MOD_BUFFER_SIZE = 262144; // ~5.5s stereo circular buffer

    SpectralSynthEngine()
    {
        initSineLUT();
        timeModBufL.assign(TIME_MOD_BUFFER_SIZE, 0.0f);
        timeModBufR.assign(TIME_MOD_BUFFER_SIZE, 0.0f);
        timeModCurve.applyTimeModPreset(0);
        resetFxChainOrder();
        resetArpStepsToDefault(0);
        for (int i = 0; i < static_cast<int>(ArticulatorTarget::Count); ++i)
            articulators[static_cast<size_t>(i)].resetToDefault(static_cast<ArticulatorTarget>(i));
    }

    void prepare(double newSampleRate, int /*samplesPerBlock*/)
    {
        sampleRate = std::max(22050.0, newSampleRate);
        invSampleRate = 1.0f / static_cast<float>(sampleRate);

        heldNotes.reserve(64);
        arpHeldNotes.reserve(64);
        activeArpTriggeredNotes.reserve(64);

        // Parameter smoothing ramp time: 15ms (smooth glide, zero clicks or zipper noise)
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

        if (timeModBufL.size() != TIME_MOD_BUFFER_SIZE)
        {
            timeModBufL.assign(TIME_MOD_BUFFER_SIZE, 0.0f);
            timeModBufR.assign(TIME_MOD_BUFFER_SIZE, 0.0f);
        }
        resetAllVoices();
    }

    void resetAllVoices()
    {
        for (auto& v : voices)
            v.reset();
        heldNotes.clear();
        arpHeldNotes.clear();
        activeArpTriggeredNotes.clear();
        arpWasEnabled = false;
        pendingSnapshot.activeVoiceCount = 0;
        pendingSnapshot.voiceActive = false;
        activeVoicesAtomic.store(0, std::memory_order_relaxed);
        smoothedPolyHeadroom.setCurrentAndTargetValue(1.0f);
        phaserLfoPhase = 0.0f;
        tremMasterPhase = 0.0f;
        chorusPhase = 0.0f;
        ringCarrierPhase = 0.0f;
        ringLfoPhase = 0.0f;
        crushHoldPhase = 0.0f;
        crushHeldL = crushHeldR = 0.0f;
        crushLpL = crushLpR = 0.0f;
        flangerPhase = 0.0f;
        flangerWritePos = 0;
        flangerFbL = flangerFbR = 0.0f;
        flangerBufL.fill(0.0f);
        flangerBufR.fill(0.0f);
        timeModWritePos = 0;
        timeModBarPhase = 0.0f;
        timeModTapePhase = 0.0f;
        timeModPrevOffsetSamples = 0.0f;
        timeModLpL = timeModLpR = timeModHpL = timeModHpR = 0.0f;
        std::fill(timeModBufL.begin(), timeModBufL.end(), 0.0f);
        std::fill(timeModBufR.begin(), timeModBufR.end(), 0.0f);
        phaserApStateL.fill(0.0f);
        phaserApStateR.fill(0.0f);
        chorusBufL.fill(0.0f);
        chorusBufR.fill(0.0f);
        delayBufL.fill(0.0f);
        delayBufR.fill(0.0f);
        revCombBufL0.fill(0.0f); revCombBufR0.fill(0.0f);
        revCombBufL1.fill(0.0f); revCombBufR1.fill(0.0f);
        revCombBufL2.fill(0.0f); revCombBufR2.fill(0.0f);
        revCombBufL3.fill(0.0f); revCombBufR3.fill(0.0f);
        revApBufL0.fill(0.0f);   revApBufR0.fill(0.0f);
        revApBufL1.fill(0.0f);   revApBufR1.fill(0.0f);
        revPreBufL.fill(0.0f);   revPreBufR.fill(0.0f);
        compEnvSingle = 0.0f;
        compEnvBand.fill(0.01f);
        compLpLowL = compLpLowR = compLpMidL = compLpMidR = 0.0f;
        limiterEnv = 1.0f;
    }

    // -------------------------------------------------------------------------
    // Dynamic Add / Remove / Drag-and-Drop Reorderable FX Rack API (12 Modules)
    // Lock-free fixed-size cache for zero audio-thread contention and zero allocations!
    // -------------------------------------------------------------------------
    static constexpr int MAX_FX_CHAIN_LEN = 16;

    void syncActiveFxChainArray() noexcept
    {
        const int count = std::min(static_cast<int>(activeFxChain.size()), MAX_FX_CHAIN_LEN);
        for (int i = 0; i < count; ++i)
            activeFxChainArray[static_cast<size_t>(i)] = activeFxChain[static_cast<size_t>(i)];
        activeFxChainCount.store(count, std::memory_order_release);
    }

    void resetFxChainOrder() noexcept
    {
        juce::ScopedLock sl(fxOrderLock);
        // Default 12-slot chain including Zenology Ring Mod, Bitcrusher & Flanger Packs
        activeFxChain = {
            static_cast<int>(FxModuleId::Blur),
            static_cast<int>(FxModuleId::Tremolo),
            static_cast<int>(FxModuleId::Distortion),
            static_cast<int>(FxModuleId::RingMod),
            static_cast<int>(FxModuleId::Bitcrusher),
            static_cast<int>(FxModuleId::Flanger),
            static_cast<int>(FxModuleId::Compressor),
            static_cast<int>(FxModuleId::Phaser),
            static_cast<int>(FxModuleId::Chorus),
            static_cast<int>(FxModuleId::ParametricEQ),
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
            return false; // Already in chain
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
    // Arpeggiator & 16-Step Sequencer API
    // -------------------------------------------------------------------------
    static constexpr int NUM_ARP_STEPS = 16;

    ArpStep getArpStep(int stepIdx) const noexcept
    {
        juce::ScopedLock sl(arpLock);
        return arpSteps[static_cast<size_t>(juce::jlimit(0, NUM_ARP_STEPS - 1, stepIdx))];
    }

    void setArpStep(int stepIdx, const ArpStep& step) noexcept
    {
        juce::ScopedLock sl(arpLock);
        auto& s = arpSteps[static_cast<size_t>(juce::jlimit(0, NUM_ARP_STEPS - 1, stepIdx))];
        s.enabled = step.enabled;
        s.velocity = juce::jlimit(0.05f, 1.0f, step.velocity);
        s.octaveOffset = juce::jlimit(-1, 2, step.octaveOffset);
        s.semitoneTranspose = juce::jlimit(-12, 12, step.semitoneTranspose);
    }

    void resetArpStepsToDefault(int patternPreset = 0) noexcept
    {
        juce::ScopedLock sl(arpLock);
        for (int i = 0; i < NUM_ARP_STEPS; ++i)
        {
            auto& s = arpSteps[static_cast<size_t>(i)];
            s.enabled = true;
            s.velocity = (i % 4 == 0) ? 0.95f : ((i % 2 == 0) ? 0.78f : 0.65f);
            s.octaveOffset = 0;
            s.semitoneTranspose = 0;

            if (patternPreset == 1) // Cyber Arp (Octave Jumps + Fifths)
            {
                s.enabled = (i != 3 && i != 11);
                s.octaveOffset = (i % 4 == 2) ? 1 : ((i == 7 || i == 15) ? 2 : 0);
                s.semitoneTranspose = (i == 5 || i == 13) ? 7 : 0;
                s.velocity = (i % 2 == 0) ? 0.92f : 0.68f;
            }
            else if (patternPreset == 2) // Driving Bassline (Root / Oct / Sub)
            {
                s.enabled = (i != 6 && i != 14);
                s.octaveOffset = (i % 2 == 1) ? 1 : ((i == 4 || i == 12) ? -1 : 0);
                s.semitoneTranspose = (i == 15) ? 3 : 0;
                s.velocity = (i % 4 == 0) ? 1.0f : 0.74f;
            }
            else if (patternPreset == 3) // Acid 303 Stepper
            {
                static const int acidSemi[16] = { 0, 0, 12, 0, 3, 7, 0, -5, 0, 10, 7, 3, 0, 12, 7, 3 };
                static const int acidOct[16]  = { 0, 0, 1,  0, 0, 1, -1, 0, 0, 1,  0, 0, -1, 1, 0, 0 };
                s.enabled = (i != 1 && i != 8);
                s.octaveOffset = acidOct[i];
                s.semitoneTranspose = acidSemi[i];
                s.velocity = (i % 3 == 0) ? 0.98f : 0.62f;
            }
            else if (patternPreset == 4) // Trance Pluck Gate
            {
                static const bool gateOn[16] = { true, true, false, true, true, false, true, true, true, false, true, true, false, true, true, true };
                s.enabled = gateOn[i];
                s.octaveOffset = (i >= 12) ? 1 : 0;
                s.semitoneTranspose = (i == 7) ? 7 : ((i == 15) ? 12 : 0);
                s.velocity = s.enabled ? (0.72f + 0.25f * ((i % 4 == 0) ? 1.0f : 0.0f)) : 0.5f;
            }
        }
    }

    int getCurrentArpStep() const noexcept
    {
        return currentArpStepAtomic.load(std::memory_order_relaxed);
    }

    SplineCurve& getArticulator(ArticulatorTarget target) noexcept
    {
        return articulators[static_cast<size_t>(target)];
    }

    const SplineCurve& getArticulator(ArticulatorTarget target) const noexcept
    {
        return articulators[static_cast<size_t>(target)];
    }

    SplineCurve& getTimeModCurve() noexcept { return timeModCurve; }
    const SplineCurve& getTimeModCurve() const noexcept { return timeModCurve; }

    // Re-triggers / wraps active voice articulator phase when user edits Rate in UI
    void notifyArticulatorRateEdited(ArticulatorTarget target) noexcept
    {
        const size_t tIdx = static_cast<size_t>(target);
        for (auto& v : voices)
        {
            if (v.active && v.artPhase[tIdx] >= 0.98f)
                v.artPhase[tIdx] = 0.0f;
        }
    }

    ResynthesisEngine& getResynthesisEngine() noexcept { return resynthEngine; }
    const ResynthesisEngine& getResynthesisEngine() const noexcept { return resynthEngine; }

    int getActiveVoiceCount() const noexcept
    {
        return activeVoicesAtomic.load(std::memory_order_relaxed);
    }

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

        // 1. Low Shelf
        if (std::abs(params.eqLowGain) > 0.05f)
        {
            const float ratio = freqHz / std::max(20.0f, params.eqLowFreq);
            const float shelf = 1.0f / (1.0f + std::pow(ratio, 2.4f));
            totalDb += params.eqLowGain * shelf;
        }

        // 2. Parametric Mid Peaking Band
        if (std::abs(params.eqMidGain) > 0.05f)
        {
            const float octDiff = std::log2(freqHz / std::max(40.0f, params.eqMidFreq));
            const float bwOct = 1.1f / std::max(0.25f, params.eqMidQ);
            const float bell = std::exp(-0.5f * std::pow(octDiff / bwOct, 2.0f));
            totalDb += params.eqMidGain * bell;
        }

        // 3. High Shelf
        if (std::abs(params.eqHighGain) > 0.05f)
        {
            const float ratio = freqHz / std::max(400.0f, params.eqHighFreq);
            const float shelf = 1.0f - (1.0f / (1.0f + std::pow(ratio, 2.4f)));
            totalDb += params.eqHighGain * shelf;
        }

        return juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, totalDb));
    }

    // -------------------------------------------------------------------------
    // 10 Spectral Filter Topologies (including Vox A-E-I-O-U, Vocoder Band-Bank, Talkbox)
    // -------------------------------------------------------------------------
    static float evaluateFilterMask(
        int filterType,
        float freqHz,
        float cutoffHz,
        float resonance,
        float width,
        const SplineCurve& customShape) noexcept
    {
        if (freqHz <= 10.0f)
            return 1.0f;

        // Architectural Safeguard: Clamp resonance to mathematically stable limits
        const float safeRes = juce::jlimit(0.0f, 0.94f, resonance);
        const float octDiff = std::log2(freqHz / std::max(20.0f, cutoffHz));
        const float bandwidth = std::max(0.15f, 0.2f + width * 2.0f);
        float gain = 1.0f;

        switch (filterType)
        {
            case 0: // SVF Lowpass 12dB/oct
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
            case 1: // SVF Lowpass 24dB/oct
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
            case 2: // Crude Low Pass (Brickwall Spectral Slope)
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
            case 3: // SVF Highpass 24dB/oct
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
            case 5: // Spectral Comb Filter (Protected against NaN)
            {
                const float combFreq = std::max(40.0f, cutoffHz * 0.25f);
                const float phase = (freqHz / combFreq) * juce::MathConstants<float>::pi;
                const float comb = std::max(0.0f, 0.5f + 0.5f * std::cos(phase));
                const float depth = 0.45f + 0.55f * (1.0f - width * 0.5f);
                gain = (1.0f - depth) + depth * std::pow(comb, 1.0f + safeRes * 2.6f) * (1.0f + safeRes * 1.2f);
                break;
            }
            case 6: // Vox / Formant Filter (5-Vowel A-E-I-O-U Morph + Formant Shift)
            {
                // 5-Vowel Formant Table: A, E, I, O, U
                static constexpr float vF1[5] = { 730.0f, 530.0f, 270.0f, 570.0f, 300.0f };
                static constexpr float vF2[5] = { 1090.0f, 1840.0f, 2290.0f, 840.0f, 870.0f };
                static constexpr float vF3[5] = { 2440.0f, 2480.0f, 3010.0f, 2410.0f, 2240.0f };

                const float vowelPos = juce::jlimit(0.0f, 1.0f, std::log2(std::max(25.0f, cutoffHz) / 25.0f) / 9.5f) * 4.0f;
                const int idx0 = juce::jlimit(0, 3, static_cast<int>(vowelPos));
                const int idx1 = idx0 + 1;
                const float frac = vowelPos - static_cast<float>(idx0);

                // Width acts as Formant Frequency Shift (0.55x baritone to 1.60x soprano)
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
            case 7: // Custom Shape (Linked to Graphical Articulator's CUSTOM Curve)
            {
                const float normFreq = juce::jlimit(0.0f, 1.0f,
                    (std::log2(freqHz / std::max(25.0f, cutoffHz * 0.12f))) / (4.5f + width * 5.0f));
                gain = customShape.evaluateLUT(normFreq);
                const float resBell = std::exp(-0.5f * std::pow(octDiff / 0.25f, 2.0f));
                gain += safeRes * 1.5f * resBell * gain;
                break;
            }
            case 8: // Vocoder Band-Bank Filter (16-Channel Robotic Vocoder Spectral Bank)
            {
                // Map frequency into log-spaced vocoder band coordinates
                const float centerShift = std::pow(2.0f, (std::log2(std::max(25.0f, cutoffHz) / 600.0f)) * 0.45f);
                const float shiftedFreq = freqHz / std::max(0.25f, centerShift);
                const float bandDensity = 2.0f + (1.0f - width * 0.75f) * 3.5f; // Bands per octave
                const float logBandPos = std::log2(std::max(40.0f, shiftedFreq) / 65.0f) * bandDensity;

                // Comb-like narrow bandpass channels + robotic formant envelope (Protected against NaN)
                const float bandPhase = logBandPos * juce::MathConstants<float>::twoPi;
                const float chBase = std::max(0.0f, 0.5f + 0.5f * std::cos(bandPhase));
                const float channelTransmission = std::pow(chBase, 1.8f + safeRes * 7.5f);

                // Robotic carrier formant contour
                const float voxEnv = std::exp(-0.5f * std::pow(std::log2(shiftedFreq / 520.0f) / 1.4f, 2.0f))
                                   + 0.85f * std::exp(-0.5f * std::pow(std::log2(shiftedFreq / 1950.0f) / 1.1f, 2.0f));
                const float bandFloor = 0.04f * (1.0f - safeRes * 0.85f);
                gain = (bandFloor + channelTransmission * (1.20f + safeRes * 1.50f)) * juce::jlimit(0.15f, 1.35f, voxEnv);
                break;
            }
            case 9: // Talkbox Mode (Expressive Vowel Articulation + Throat/Tube Driver Emphasis)
            default:
            {
                // Talkbox vocal tract trajectory: OOO (0.0) -> WAH (0.33) -> YAH (0.66) -> EEE (1.0)
                const float morph = juce::jlimit(0.0f, 1.0f, std::log2(std::max(25.0f, cutoffHz) / 25.0f) / 9.2f);
                const float throatScale = 0.70f + width * 0.65f;

                // Continuous expressive talkbox trajectory
                const float f1 = (290.0f + 510.0f * std::sin(morph * juce::MathConstants<float>::pi)) * throatScale;
                const float f2 = (720.0f + 1680.0f * std::pow(morph, 1.25f)) * throatScale;
                const float f3 = (2150.0f + 780.0f * morph) * throatScale;
                const float throatChest = 195.0f * throatScale;

                const float q = 0.13f + (1.0f - safeRes * 0.70f) * 0.18f;
                const float pChest = 0.55f * std::exp(-0.5f * std::pow(std::log2(freqHz / throatChest) / 0.35f, 2.0f));
                const float p1 = 1.15f * std::exp(-0.5f * std::pow(std::log2(freqHz / f1) / q, 2.0f));
                const float p2 = 1.10f * std::exp(-0.5f * std::pow(std::log2(freqHz / f2) / (q * 0.9f), 2.0f));
                const float p3 = 0.70f * std::exp(-0.5f * std::pow(std::log2(freqHz / f3) / (q * 1.1f), 2.0f));

                // Talkbox plastic tube driver 1.8kHz presence + steep acoustic tube rolloff above 5.5kHz
                const float driverPresence = 1.0f + 0.45f * std::exp(-0.5f * std::pow(std::log2(freqHz / 1850.0f) / 0.85f, 2.0f));
                const float tubeRolloff = 1.0f / (1.0f + std::pow(freqHz / 5400.0f, 3.2f));

                gain = (0.06f + (1.15f + safeRes * 1.65f) * (pChest + p1 + p2 + p3)) * driverPresence * tubeRolloff;
                break;
            }
        }

        if (std::isnan(gain) || std::isinf(gain))
            gain = 1.0f;

        return juce::jlimit(0.0f, 2.4f, gain);
    }

    void noteOn(int midiNote, float velocity, const SynthParameters& params)
    {
        if (params.arpEnable)
        {
            arpHeldNotes.erase(std::remove_if(arpHeldNotes.begin(), arpHeldNotes.end(),
                [midiNote](const HeldNote& n) { return n.note == midiNote; }), arpHeldNotes.end());
            arpHeldNotes.push_back({ midiNote, velocity });
            std::sort(arpHeldNotes.begin(), arpHeldNotes.end(),
                [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

            if (arpHeldNotes.size() == 1)
            {
                arpStepIndex = 0;
                arpNoteIndex = 0;
                arpDirection = 1;
                arpPhase = 1.0f; // Trigger step immediately on first key press
            }
            return;
        }

        triggerInternalNoteOn(midiNote, velocity, params);
    }

    void noteOff(int midiNote, const SynthParameters& params)
    {
        if (params.arpEnable)
        {
            arpHeldNotes.erase(std::remove_if(arpHeldNotes.begin(), arpHeldNotes.end(),
                [midiNote](const HeldNote& n) { return n.note == midiNote; }), arpHeldNotes.end());

            if (arpHeldNotes.empty())
            {
                stopActiveArpNotes(params);
            }
            return;
        }

        triggerInternalNoteOff(midiNote, params);
    }

    void allNotesOff()
    {
        heldNotes.clear();
        arpHeldNotes.clear();
        activeArpTriggeredNotes.clear();
        for (auto& v : voices)
        {
            if (v.active)
                v.releasing = true;
        }
    }

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const SynthParameters& params)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        if (numSamples <= 0 || numChannels <= 0)
            return;

        // 1. Mandatory Buffer Initialization (All Channels)
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);

        // Handle clean transition when Arp is toggled on/off while notes are held
        if (params.arpEnable != arpWasEnabled)
        {
            if (!params.arpEnable)
            {
                stopActiveArpNotes(params);
                arpHeldNotes.clear();
            }
            else
            {
                // Transfer any currently held notes into Arp
                arpHeldNotes = heldNotes;
                std::sort(arpHeldNotes.begin(), arpHeldNotes.end(),
                    [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
                for (const auto& hn : heldNotes)
                    triggerInternalNoteOff(hn.note, params);
                arpStepIndex = 0;
                arpNoteIndex = 0;
                arpDirection = 1;
                arpPhase = 1.0f;
            }
            arpWasEnabled = params.arpEnable;
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = (numChannels > 1) ? buffer.getWritePointer(1) : outL;

        // Smooth Unison parameter targets (15ms linear ramp, click-free)
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

        // ---------------------------------------------------------------------
        // 2. Pre-FX Headroom & Gain Scaling (Crucial):
        // Scale the summed polyphonic signal DOWN based on active voices (1.0f / sqrt(N))
        // STRICTLY AFTER all voices are rendered, but BEFORE the global Filter Drive and FX rack!
        // This prevents multi-voice polyphony from overloading non-linear filters or FX.
        // ---------------------------------------------------------------------
        int activeVoicesCount = 0;
        for (const auto& v : voices)
        {
            if (v.active)
                ++activeVoicesCount;
        }
        activeVoicesCount = std::max(1, activeVoicesCount);

        const float targetPreFxScale = 1.0f / std::sqrt(static_cast<float>(activeVoicesCount));
        smoothedPolyHeadroom.setTargetValue(targetPreFxScale);

        for (int i = 0; i < numSamples; ++i)
        {
            const float preFxGain = smoothedPolyHeadroom.getNextValue();
            outL[i] *= preFxGain;
            if (numChannels > 1)
                outR[i] *= preFxGain;
        }

        // Synthesize Dedicated Serum-2 Style Noise Oscillator & Run Dynamic Multi-FX Chain + Master Limiter
        processNoiseAndMasterFXChain(outL, outR, numSamples, numChannels, params);
    }

private:
    struct HeldNote
    {
        int note = 60;
        float velocity = 0.8f;
    };

    void triggerInternalNoteOn(int midiNote, float velocity, const SynthParameters& params)
    {
        midiNote = juce::jlimit(0, 127, midiNote);
        const float targetFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        const int effectiveMaxVoices = juce::jlimit(1, MAX_VOICES,
            (params.voicingMode == 0) ? params.maxVoices : 1);

        const bool hadHeldNotes = !heldNotes.empty();

        heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
            [midiNote](const HeldNote& n) { return n.note == midiNote; }), heldNotes.end());
        heldNotes.push_back({ midiNote, velocity });

        const bool shouldGlide = (params.portaTime > 0.005f) && (!params.autoGlide || hadHeldNotes);
        const float startFreq = (shouldGlide && lastPlayedFreq > 20.0f) ? lastPlayedFreq : targetFreq;
        lastPlayedFreq = targetFreq;

        if (params.voicingMode == 1 || params.voicingMode == 2)
        {
            for (int i = 1; i < MAX_VOICES; ++i)
                voices[static_cast<size_t>(i)].reset();

            auto& monoVoice = voices[0];
            const bool isLegatoTransition = (params.voicingMode == 2) && hadHeldNotes && monoVoice.active && !monoVoice.releasing;

            if (isLegatoTransition)
                monoVoice.triggerLegatoNote(midiNote, velocity, targetFreq, params);
            else
                monoVoice.startNote(midiNote, velocity, startFreq, targetFreq, params, rng);
            return;
        }

        for (int i = 0; i < effectiveMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            if (v.active && v.midiNote == midiNote)
            {
                v.startNote(midiNote, velocity, startFreq, targetFreq, params, rng);
                return;
            }
        }

        Voice* chosen = nullptr;

        // Tier 1: Prefer any completely inactive voice
        for (int i = 0; i < effectiveMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            if (!v.active)
            {
                chosen = &v;
                break;
            }
        }

        // Tier 2: Steal the releasing voice with the lowest current gain (inaudible steal!)
        if (chosen == nullptr)
        {
            float lowestRelGain = 999.0f;
            for (int i = 0; i < effectiveMaxVoices; ++i)
            {
                auto& v = voices[static_cast<size_t>(i)];
                if (v.releasing && v.curGain < lowestRelGain)
                {
                    lowestRelGain = v.curGain;
                    chosen = &v;
                }
            }
        }

        // Tier 3: If all voices are currently held, steal the oldest held note
        if (chosen == nullptr)
        {
            double oldestHeldAge = -1.0;
            for (int i = 0; i < effectiveMaxVoices; ++i)
            {
                auto& v = voices[static_cast<size_t>(i)];
                if (v.noteAgeSeconds > oldestHeldAge)
                {
                    oldestHeldAge = v.noteAgeSeconds;
                    chosen = &v;
                }
            }
        }

        for (int i = effectiveMaxVoices; i < MAX_VOICES; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            if (v.curGain < 0.001f)
                v.reset();
        }

        if (chosen == nullptr)
            chosen = &voices[0];

        chosen->startNote(midiNote, velocity, startFreq, targetFreq, params, rng);
    }

    void triggerInternalNoteOff(int midiNote, const SynthParameters& params)
    {
        heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
            [midiNote](const HeldNote& n) { return n.note == midiNote; }), heldNotes.end());

        if ((params.voicingMode == 1 || params.voicingMode == 2) && !heldNotes.empty())
        {
            const auto& prevHeld = heldNotes.back();
            auto& monoVoice = voices[0];
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

        for (auto& v : voices)
        {
            if (v.active && v.midiNote == midiNote)
                v.releasing = true;
        }
    }

    void stopActiveArpNotes(const SynthParameters& params)
    {
        for (int n : activeArpTriggeredNotes)
            triggerInternalNoteOff(n, params);
        activeArpTriggeredNotes.clear();
    }

    void advanceArpeggiator(int numSamples, const SynthParameters& params)
    {
        if (arpHeldNotes.empty())
            return;

        // Calculate step rate in Hz from host BPM and arpRate division
        // 0: 1/4, 1: 1/8, 2: 1/8T, 3: 1/16, 4: 1/16T, 5: 1/32
        const float safeBpm = static_cast<float>(juce::jlimit(30.0, 300.0, params.hostBpm));
        const float beatsPerSec = safeBpm / 60.0f;
        float stepsPerSec = beatsPerSec * 4.0f;
        switch (juce::jlimit(0, 5, params.arpRate))
        {
            case 0: stepsPerSec = beatsPerSec * 1.0f; break; // 1/4
            case 1: stepsPerSec = beatsPerSec * 2.0f; break; // 1/8
            case 2: stepsPerSec = beatsPerSec * 3.0f; break; // 1/8T
            case 3: stepsPerSec = beatsPerSec * 4.0f; break; // 1/16
            case 4: stepsPerSec = beatsPerSec * 6.0f; break; // 1/16T
            case 5: stepsPerSec = beatsPerSec * 8.0f; break; // 1/32
            default: break;
        }

        // Apply Swing to even/odd steps
        const float swing = juce::jlimit(0.0f, 0.75f, params.arpSwing);
        const float swingMult = ((arpStepIndex & 1) == 0) ? (1.0f + swing * 0.45f) : (1.0f - swing * 0.45f);
        const float effectiveStepHz = stepsPerSec / std::max(0.25f, swingMult);

        const float dt = static_cast<float>(numSamples) * invSampleRate;
        arpPhase += dt * effectiveStepHz;

        const float gateLen = juce::jlimit(0.08f, 0.99f, params.arpGate);
        if (arpPhase >= gateLen && !activeArpTriggeredNotes.empty() && arpPhase < 1.0f)
        {
            stopActiveArpNotes(params);
        }

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

            if (params.arpMode == 4) // Chord Mode: trigger all held notes shifted by step octave & transpose
            {
                for (const auto& hn : arpHeldNotes)
                {
                    const int noteToPlay = juce::jlimit(12, 115,
                        hn.note + step.octaveOffset * 12 + step.semitoneTranspose);
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
                    case 0: // Up
                        poolIdx = arpNoteIndex % totalPool;
                        arpNoteIndex = (arpNoteIndex + 1) % totalPool;
                        break;

                    case 1: // Down
                        poolIdx = (totalPool - 1) - (arpNoteIndex % totalPool);
                        arpNoteIndex = (arpNoteIndex + 1) % totalPool;
                        break;

                    case 2: // Up / Down
                        if (totalPool <= 1)
                        {
                            poolIdx = 0;
                        }
                        else
                        {
                            arpNoteIndex = juce::jlimit(0, totalPool - 1, arpNoteIndex);
                            poolIdx = arpNoteIndex;
                            arpNoteIndex += arpDirection;
                            if (arpNoteIndex >= totalPool)
                            {
                                arpNoteIndex = std::max(0, totalPool - 2);
                                arpDirection = -1;
                            }
                            else if (arpNoteIndex < 0)
                            {
                                arpNoteIndex = std::min(totalPool - 1, 1);
                                arpDirection = 1;
                            }
                        }
                        break;

                    case 3: // Random
                        poolIdx = rng.nextInt(totalPool);
                        break;

                    case 5: // Step Sequence (locks to lowest held root note + step sequence intervals)
                    default:
                        poolIdx = curStep % totalPool;
                        break;
                }

                const int baseIdx = poolIdx % numHeld;
                const int octIdx  = (params.arpMode == 5) ? 0 : (poolIdx / numHeld);
                const auto& baseNote = arpHeldNotes[static_cast<size_t>(baseIdx)];

                const int finalNote = juce::jlimit(12, 115,
                    baseNote.note + (octIdx + step.octaveOffset) * 12 + step.semitoneTranspose);
                const float finalVel = juce::jlimit(0.05f, 1.0f, baseNote.velocity * step.velocity);

                triggerInternalNoteOn(finalNote, finalVel, params);
                activeArpTriggeredNotes.push_back(finalNote);
            }
        }
    }

    struct Voice
    {
        bool  active = false;
        bool  releasing = false;
        int   midiNote = 60;
        float velocity = 0.8f;
        float glideStartFreq = 130.81f;
        float currentBaseFreq = 130.81f;
        float targetBaseFreq = 130.81f;
        float glideProgress = 1.0f;
        double noteAgeSeconds = 0.0;
        float releaseEnv = 1.0f;

        // Independent phase accumulators per ArticulatorTarget (Audio-Thread Rate Fix!)
        std::array<float, static_cast<size_t>(ArticulatorTarget::Count)> artPhase {};
        float curVolEnv = 1.0f;
        float targetVolEnv = 1.0f;

        // 1024-Sample Single-Cycle Periodic Waveform Buffers (Optimized Serum 2 / RAPID Architecture)
        static constexpr int WAVE_TABLE_SIZE = 1024;
        std::array<float, WAVE_TABLE_SIZE> waveTableA {};
        std::array<float, WAVE_TABLE_SIZE> waveTableB {};
        std::array<float, WAVE_TABLE_SIZE> prevWaveTableA {};
        std::array<float, WAVE_TABLE_SIZE> prevWaveTableB {};
        bool hasPrevTable = false;

        // 16 Unison Sub-Oscillators per Voice (True Single-Note Stacking!)
        std::array<uint32_t, MAX_UNISON> subPhaseA {};
        std::array<uint32_t, MAX_UNISON> subPhaseB {};
        std::array<uint32_t, MAX_UNISON> subPhaseIncA {};
        std::array<uint32_t, MAX_UNISON> subPhaseIncB {};

        std::array<float, MAX_UNISON> uWeightL {};
        std::array<float, MAX_UNISON> uWeightR {};
        std::array<float, MAX_UNISON> curUniVoiceWeight {};
        std::array<float, MAX_UNISON> targetUniVoiceWeight {};

        // Per-voice Unison initial decorrelation phases [MAX_UNISON]
        std::array<float, MAX_UNISON> unisonInitPhaseA {};
        std::array<float, MAX_UNISON> unisonInitPhaseB {};

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

        void reset() noexcept
        {
            active = false;
            releasing = false;
            releaseEnv = 0.0f;
            noteAgeSeconds = 0.0;
            artPhase.fill(0.0f);
            curAmpA.fill(0.0f);
            curAmpB.fill(0.0f);
            targetAmpA.fill(0.0f);
            targetAmpB.fill(0.0f);
            blurStateA.fill(0.0f);
            blurStateB.fill(0.0f);
            pluckState.fill(1.0f);

            subPhaseA.fill(0u);
            subPhaseB.fill(0u);
            subPhaseIncA.fill(0u);
            subPhaseIncB.fill(0u);
            uWeightL.fill(0.0f);
            uWeightR.fill(0.0f);
            curUniVoiceWeight.fill(0.0f);
            targetUniVoiceWeight.fill(0.0f);
            waveTableA.fill(0.0f);
            waveTableB.fill(0.0f);
            prevWaveTableA.fill(0.0f);
            prevWaveTableB.fill(0.0f);
            hasPrevTable = false;
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
            releaseEnv = 1.0f;
            artPhase.fill(0.0f);
            curVolEnv = 0.0f;
            targetVolEnv = 1.0f;

            // Initialize 16 Unison Sub-Oscillators inside this single voice
            const int initVoices = juce::jlimit(1, MAX_UNISON, params.unisonVoices);
            const float randPhaseSpread = juce::jlimit(0.0f, 1.0f, params.unisonPhase);

            for (int u = 0; u < MAX_UNISON; ++u)
            {
                const float w = (u < initVoices) ? 1.0f : 0.0f;
                curUniVoiceWeight[static_cast<size_t>(u)] = w;
                targetUniVoiceWeight[static_cast<size_t>(u)] = w;

                const float spreadPh = (static_cast<float>(u) / static_cast<float>(MAX_UNISON)) * juce::MathConstants<float>::twoPi;
                const float randPhA  = randomGen.nextFloat() * juce::MathConstants<float>::twoPi;
                const float randPhB  = randomGen.nextFloat() * juce::MathConstants<float>::twoPi;
                const float initA = (u == 0) ? 0.0f : (spreadPh * 0.5f + randPhA * 0.5f) * randPhaseSpread;
                const float initB = (u == 0) ? 0.0f : (spreadPh * 0.5f + randPhB * 0.5f) * randPhaseSpread;

                unisonInitPhaseA[static_cast<size_t>(u)] = initA;
                unisonInitPhaseB[static_cast<size_t>(u)] = initB;

                subPhaseA[static_cast<size_t>(u)] = static_cast<uint32_t>((initA / juce::MathConstants<float>::twoPi) * 4294967295.0f);
                subPhaseB[static_cast<size_t>(u)] = static_cast<uint32_t>((initB / juce::MathConstants<float>::twoPi) * 4294967295.0f);
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

            waveTableA.fill(0.0f);
            waveTableB.fill(0.0f);
            prevWaveTableA.fill(0.0f);
            prevWaveTableB.fill(0.0f);
            hasPrevTable = false;
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
            releaseEnv = 1.0f;
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

    // High-performance vectorized trig lookups (2-3 clock cycles, zero branches)
    inline float fastSin(float rad) const noexcept
    {
        const uint32_t ph = static_cast<uint32_t>(static_cast<int64_t>(rad * 683565275.5764316));
        return sineLUT[ph >> 19];
    }

    inline float fastCos(float rad) const noexcept
    {
        const uint32_t ph = static_cast<uint32_t>(static_cast<int64_t>(rad * 683565275.5764316)) + 0x40000000u;
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
                if (normK <= pivot)
                    outSourceNormK = normK / pivot;
                else
                    outSourceNormK = 1.0f - (normK - pivot) / (1.0f - pivot);
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
                const float folded = std::fmod(normK * syncRatio, 1.0f);
                outSourceNormK = folded;
                break;
            }

            default:
                break;
        }
    }

    void updateVoiceSpectralTables(Voice& v, float dtSeconds, const SynthParameters& params,
                                   float uDetune, float uSpread, float uBlend, float uVoicesCount,
                                   float mixA, float mixB, int releasingCount = 0)
    {
        v.noteAgeSeconds += static_cast<double>(dtSeconds);

        // ---------------------------------------------------------------------
        // 1. Advance ALL 6 Articulators at their exact Free Hz or Tempo-Sync Rate!
        // ---------------------------------------------------------------------
        v.curVolEnv = v.targetVolEnv;
        for (size_t t = 0; t < static_cast<size_t>(ArticulatorTarget::Count); ++t)
        {
            const auto& art = articulators[t];
            const float effectiveHz = art.getEffectiveRateHz(params.hostBpm);
            v.artPhase[t] += dtSeconds * effectiveHz;

            if (art.getMode() == ArticulatorMode::Envelope)
            {
                // In One-Shot Envelope mode, hold at sustain if note is still held and end node is 0
                v.artPhase[t] = std::min(1.0f, v.artPhase[t]);
            }
            else
            {
                v.artPhase[t] = v.artPhase[t] - std::floor(v.artPhase[t]);
            }
        }

        // Release envelope (controlled by physical AHDSR Release knob + Time Blur tail)
        const float blurTailSeconds = params.blurEnable ? (params.blurTime * 1.8f) : 0.0f;
        const float ahdsrRelSeconds = 0.015f + std::pow(juce::jlimit(0.01f, 1.0f, params.envRelease), 1.8f) * 2.4f;
        if (v.releasing)
        {
            // Aggressive release decay acceleration when overlapping tails stack up
            const float relSpeedup = (releasingCount > 3) ? (1.0f + static_cast<float>(releasingCount - 3) * 0.75f) : 1.0f;
            const float relRate = (1.0f / (ahdsrRelSeconds + blurTailSeconds)) * relSpeedup;
            v.releaseEnv -= dtSeconds * relRate;
            if (v.releaseEnv <= 0.0008f)
            {
                v.reset();
                return;
            }
        }

        // Evaluate Articulators using their own dedicated audio-thread phases!
        const float volEnvMod    = articulators[static_cast<size_t>(ArticulatorTarget::Volume)].evaluateLUT(
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

        v.targetVolEnv = volEnvMod;
        if (v.noteAgeSeconds <= static_cast<double>(dtSeconds * 1.5f))
            v.curVolEnv = volEnvMod;

        // Portamento / Glide with configurable slope curve (portaCurve)
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

        // ---------------------------------------------------------------------
        // 2. Serum-2 Style Pitch Calculation (OCT, SEMI, FIN, CRS) per Field!
        // ---------------------------------------------------------------------
        const float vibLfo = fastSin(static_cast<float>(v.noteAgeSeconds) * params.vibSpeed * juce::MathConstants<float>::twoPi);
        const float modSemis = params.pitchFreq + (vibLfo * params.vibDepth * 1.2f) + (pitchArtMod * 12.0f);

        const float field1PitchSt = static_cast<float>(params.oscAOct * 12 + params.oscASemi)
                                  + params.oscAFine * 0.01f + params.oscACoarse;
        const float field2PitchSt = static_cast<float>(params.oscBOct * 12 + params.oscBSemi)
                                  + params.oscBFine * 0.01f + params.oscBPitch;

        const float nyquist = static_cast<float>(sampleRate * 0.47);
        const float safeBaseFreq = juce::jlimit(10.0f, nyquist, v.currentBaseFreq);
        const float clampedPitchSemisA = juce::jlimit(-48.0f, 48.0f, modSemis + field1PitchSt);
        const float f0A = juce::jlimit(10.0f, nyquist, safeBaseFreq * std::pow(2.0f, clampedPitchSemisA / 12.0f));
        const float clampedPitchSemisB = juce::jlimit(-48.0f, 48.0f, modSemis + field2PitchSt);
        const float f0B = juce::jlimit(10.0f, nyquist, safeBaseFreq * std::pow(2.0f, clampedPitchSemisB / 12.0f));
        const float effTimbreA = juce::jlimit(0.0f, 1.0f, params.timbre  + timbreArtMod);
        const float effTimbreB = juce::jlimit(0.0f, 1.0f, params.timbreB + timbreArtMod);
        const float effPrism   = params.prismEnable ? juce::jlimit(0.0f, 1.0f, params.prismAmount + prismArtMod) : 0.0f;

        const float resynthScanPosA = std::fmod(static_cast<float>(v.noteAgeSeconds) * 0.35f + effTimbreA * 0.65f, 1.0f);
        const float resynthScanPosB = std::fmod(static_cast<float>(v.noteAgeSeconds) * 0.35f + effTimbreB * 0.65f + 0.25f, 1.0f);
        const float harmScaleA = juce::jlimit(0.25f, 4.0f, params.harmonicScale);
        const float harmScaleB = juce::jlimit(0.25f, 4.0f, params.harmonicScaleB);

        std::array<float, NUM_PARTIALS> rawAmpA {};
        std::array<float, NUM_PARTIALS> rawAmpB {};

        // STEP 1 & 2: Independent Dual Frequency Tables (Field 1 & Field 2)
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

            // Prism Frequency Table Dispersion
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

            // Independent Spectral Tilt for Field 1 (A) and Field 2 (B)
            if (std::abs(params.harmonicTilt) > 0.01f)
            {
                const float octFromRef = std::log2(h / 4.0f);
                const float tiltDb = octFromRef * params.harmonicTilt * 6.5f;
                baseA *= juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, tiltDb));
            }
            if (std::abs(params.harmonicTiltB) > 0.01f)
            {
                const float octFromRef = std::log2(h / 4.0f);
                const float tiltDbB = octFromRef * params.harmonicTiltB * 6.5f;
                baseB *= juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, tiltDbB));
            }

            // Harmonic Strum onset delay
            if (params.strumTime > 0.01f)
            {
                const float curvedK = SplineCurve::applyTensionCurve(normK, params.strumTension);
                const float onsetTime = curvedK * params.strumTime * 0.55f;
                const float strumGate = juce::jlimit(0.0f, 1.0f, (static_cast<float>(v.noteAgeSeconds) - onsetTime) / 0.022f);
                baseA *= strumGate;
                baseB *= strumGate;
            }

            // Spectral Pluck decay
            if (params.pluckEnable && params.pluckTime < 0.99f)
            {
                const float dampSlope = std::pow(h, 0.50f + (1.0f - params.pluckBlur) * 0.85f);
                const float decayTime = 0.025f + params.pluckTime * params.pluckTime * 2.6f;
                const float decayRate = dampSlope / decayTime;
                v.pluckState[idx] *= std::exp(-dtSeconds * decayRate);
                baseA *= v.pluckState[idx];
                baseB *= v.pluckState[idx];
            }

            rawAmpA[idx] = baseA;
            rawAmpB[idx] = baseB;
        }

        // STEP 3: Harmonizer
        if (params.prismEnable && params.harmShift > 0.01f)
        {
            const int gapStep = std::max(1, static_cast<int>(1.0f + params.harmGap * 5.0f));
            const float widthDecay = 1.0f - params.harmWidth * 0.70f;
            const float shiftGain = params.harmShift * 0.85f;

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

        // STEP 4: Harmonic Blur
        if (params.blurEnable && params.blurHarmonic > 0.005f)
        {
            const int radius = std::max(1, static_cast<int>(1.0f + params.blurHarmonic * 10.0f));
            const float mix = juce::jlimit(0.0f, 1.0f, params.blurHarmonic);
            std::array<float, NUM_PARTIALS> tempA = rawAmpA;
            std::array<float, NUM_PARTIALS> tempB = rawAmpB;

            for (int k = 0; k < NUM_PARTIALS; ++k)
            {
                float sumA = 0.0f;
                float sumB = 0.0f;
                float weightSum = 0.0f;

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
                const float blurredA = (sumA / std::max(1.0f, weightSum)) * makeup;
                const float blurredB = (sumB / std::max(1.0f, weightSum)) * makeup;

                rawAmpA[static_cast<size_t>(k)] = tempA[static_cast<size_t>(k)] * (1.0f - mix) + blurredA * mix;
                rawAmpB[static_cast<size_t>(k)] = tempB[static_cast<size_t>(k)] * (1.0f - mix) + blurredB * mix;
            }
        }

        // STEP 5: Dual Spectral Filters + Drive + Spectral Phaser + Parametric EQ + Tremolo
        const float keyOctaveOffset = std::log2(std::max(20.0f, f0A) / 261.63f);

        const float f1CutNorm = juce::jlimit(0.0f, 1.0f,
            params.filter1Cutoff + filt1ArtMod * 0.45f + keyOctaveOffset * params.filter1KeyTrack * 0.12f);
        const float f2CutNorm = juce::jlimit(0.0f, 1.0f,
            params.filter2Cutoff + filt1ArtMod * 0.30f + keyOctaveOffset * params.filter2KeyTrack * 0.12f);

        const float f1CutoffHz = 25.0f * std::pow(750.0f, f1CutNorm);
        const float f2CutoffHz = 25.0f * std::pow(750.0f, f2CutNorm);

        const float phaserSweepPhase = static_cast<float>(v.noteAgeSeconds) * params.phaserSpeed * juce::MathConstants<float>::twoPi
                                     + params.phaserOffset * juce::MathConstants<float>::twoPi;
        const float tremBasePhase = static_cast<float>(v.noteAgeSeconds) * params.tremSpeed * juce::MathConstants<float>::twoPi;

        const float velScale = (1.0f - params.velLink) + params.velLink * v.velocity;
        // Note: volEnvMod is applied AFTER blurStateA/B so fast ShaperBox articulator rates are never smeared by Blur!
        const float baseVoiceScale = v.releaseEnv * velScale * 0.34f;

        const float blurTau = params.blurEnable
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

                if (params.filterDrive > 0.01f)
                {
                    const float harmonicBoost = 1.0f + params.filterDrive * 1.4f * ((k % 2 == 0) ? 1.0f : 0.65f);
                    mask = std::tanh(mask * harmonicBoost) * (1.0f + params.filterDrive * 0.45f);
                }

                if (params.phaserEnable && params.phaserMix > 0.005f)
                {
                    const float logF = std::log2(std::max(25.0f, fk));
                    const float density = 1.2f + (1.0f - params.phaserWidth) * 4.2f;
                    const float rawCos = std::cos(logF * density - phaserSweepPhase);
                    const float deepNotch = std::pow(0.5f + 0.5f * rawCos, 3.2f) * 1.45f;
                    mask *= (1.0f - params.phaserMix) + params.phaserMix * deepNotch;
                }

                mask *= evaluateParametricEQGain(fk, params);
                return mask;
            };

            const float maskA = (!overA) ? evalMaskForFreq(fkA) : 0.0f;
            const float maskB = (!overB) ? evalMaskForFreq(fkB) : 0.0f;
            v.filterGainTable[idx] = juce::jlimit(0.0f, 2.5f, maskA);

            float tremModA = 1.0f;
            float tremModB = 1.0f;
            if (params.tremEnable && params.tremDepth > 0.005f)
            {
                const float gapOffset = static_cast<float>(k) * params.tremGap * 0.85f;
                const float lfoA = 0.5f + 0.5f * std::sin(tremBasePhase + gapOffset);
                const float lfoB = 0.5f + 0.5f * std::cos(tremBasePhase - gapOffset);
                tremModA = 1.0f - params.tremDepth * (1.0f - lfoA * lfoA);
                tremModB = 1.0f - params.tremDepth * (1.0f - lfoB * lfoB);
            }

            const float desiredA = (!overA) ? (rawAmpA[idx] * maskA * tremModA * baseVoiceScale) : 0.0f;
            const float desiredB = (!overB) ? (rawAmpB[idx] * maskB * tremModB * baseVoiceScale) : 0.0f;

            v.blurStateA[idx] = (!overA) ? (v.blurStateA[idx] + blurCoeff * (desiredA - v.blurStateA[idx])) : 0.0f;
            v.blurStateB[idx] = (!overB) ? (v.blurStateB[idx] + blurCoeff * (desiredB - v.blurStateB[idx])) : 0.0f;

            // Apply Volume Articulator AFTER Blur so ShaperBox gates/rates are crisp and sample-responsive!
            v.targetAmpA[idx] = v.blurStateA[idx] * volEnvMod;
            v.targetAmpB[idx] = v.blurStateB[idx] * volEnvMod;
        }

        // ---------------------------------------------------------------------
        // 1. Configure 16 Unison Sub-Oscillators inside this Single Voice
        // (Calculates pitch detune ratios before wavetable bandlimiting)
        // ---------------------------------------------------------------------
        const int targetCount = juce::jlimit(1, MAX_UNISON, static_cast<int>(std::round(uVoicesCount)));
        const float weightSlewRate = dtSeconds / 0.015f; // 15ms crossfade

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

        // Serum 2 Detune: 0 to 200 cents range with exponential response
        const float detuneCentsMax = uDetune * uDetune * 160.0f + uDetune * 40.0f;

        float sumSqWeights = 0.0f;
        std::array<float, MAX_UNISON> pitchRatio {};

        for (int u = 0; u < maxActiveU; ++u)
        {
            // Normalized position across unison sub-oscillators [-1.0, +1.0]
            const float normPos = (maxActiveU <= 1) ? 0.0f : (-1.0f + (2.0f * static_cast<float>(u)) / static_cast<float>(maxActiveU - 1));
            const float edgeDist = std::abs(normPos);

            // Serum 2 "Super" exponential detune curve
            const float superDetuneNorm = (normPos >= 0.0f ? 1.0f : -1.0f) * std::pow(edgeDist, 1.35f);

            // True sub-oscillator pitch ratio: f_u / f_0
            float ratio = std::pow(2.0f, (superDetuneNorm * detuneCentsMax) / 1200.0f);

            // Unison Stack Modes (when enabled)
            if (params.unisonStackMode > 0 && maxActiveU > 1)
            {
                if (params.unisonStackMode == 1 && (u % 2 == 1))      ratio *= 2.0f; // +1 Oct
                else if (params.unisonStackMode == 2)                  ratio *= (u % 3 == 1 ? 2.0f : (u % 3 == 2 ? 4.0f : 1.0f)); // +2 Oct
                else if (params.unisonStackMode == 3 && (u == 0))      ratio *= 0.5f; // Sub-octave
                else if (params.unisonStackMode == 4 && (u % 2 == 1))  ratio *= 1.498307f; // Fifths
                else if (params.unisonStackMode == 5)                  ratio *= (u % 3 == 1 ? 1.498307f : (u % 3 == 2 ? 2.0f : 1.0f));
            }
            pitchRatio[static_cast<size_t>(u)] = ratio;

            // Serum 2 Blend: at Blend=0, outer voices fade; at Blend=1.0, all active voices share equal volume
            const float amp = ((1.0f - edgeDist) + edgeDist * uBlend) * v.curUniVoiceWeight[static_cast<size_t>(u)];

            // Equal-power stereo panning spread
            const float panPos = juce::jlimit(-1.0f, 1.0f, normPos * uSpread);
            const float panAngle = (panPos + 1.0f) * juce::MathConstants<float>::pi * 0.25f;

            v.uWeightL[static_cast<size_t>(u)] = amp * std::cos(panAngle);
            v.uWeightR[static_cast<size_t>(u)] = amp * std::sin(panAngle);
            sumSqWeights += amp * amp;
        }

        // Strict Equal-Power Gain Normalization: 1 / sqrt(sumSq) (Eliminates Crackling!)
        const float gainNorm = 1.0f / std::sqrt(std::max(0.01f, sumSqWeights));
        for (int u = 0; u < maxActiveU; ++u)
        {
            v.uWeightL[static_cast<size_t>(u)] *= gainNorm;
            v.uWeightR[static_cast<size_t>(u)] *= gainNorm;

            const float subFreqA = juce::jlimit(10.0f, nyquist, f0A * pitchRatio[static_cast<size_t>(u)]);
            const float subFreqB = juce::jlimit(10.0f, nyquist, f0B * pitchRatio[static_cast<size_t>(u)]);

            v.subPhaseIncA[static_cast<size_t>(u)] = static_cast<uint32_t>((subFreqA * invSampleRate) * 4294967296.0);
            v.subPhaseIncB[static_cast<size_t>(u)] = static_cast<uint32_t>((subFreqB * invSampleRate) * 4294967296.0);
        }

        // ---------------------------------------------------------------------
        // 2. Build 1024-Sample Single-Cycle Periodic Waveform for Part A & Part B
        // (Strict Unison-Aware Per-Voice Bandlimiting & Ping-Pong Chunk Smoothing)
        // ---------------------------------------------------------------------
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

        if (doA)
            v.waveTableA.fill(0.0f);
        if (doB)
            v.waveTableB.fill(0.0f);

        float maxRatio = 1.0f;
        for (int u = 0; u < maxActiveU; ++u)
            maxRatio = std::max(maxRatio, pitchRatio[static_cast<size_t>(u)]);

        const float maxF0A = f0A * maxRatio;
        const float maxF0B = f0B * maxRatio;
        const float nyquistGuard = nyquist * 0.95f;
        const float rollOffStart = nyquist * 0.80f;

        for (int k = 0; k < NUM_PARTIALS; ++k)
        {
            float aA = v.targetAmpA[static_cast<size_t>(k)];
            float aB = v.targetAmpB[static_cast<size_t>(k)];

            const float h = static_cast<float>(k + 1);
            const float fkA_max = h * maxF0A;
            const float fkB_max = h * maxF0B;

            if (fkA_max >= nyquistGuard)
            {
                aA = 0.0f;
            }
            else if (fkA_max > rollOffStart)
            {
                const float normF = (fkA_max - rollOffStart) / (nyquistGuard - rollOffStart);
                aA *= 0.5f * (1.0f + std::cos(normF * juce::MathConstants<float>::pi));
            }

            if (fkB_max >= nyquistGuard)
            {
                aB = 0.0f;
            }
            else if (fkB_max > rollOffStart)
            {
                const float normF = (fkB_max - rollOffStart) / (nyquistGuard - rollOffStart);
                aB *= 0.5f * (1.0f + std::cos(normF * juce::MathConstants<float>::pi));
            }

            if ((!doA || aA < 1e-5f) && (!doB || aB < 1e-5f))
                continue;

            const uint32_t step = static_cast<uint32_t>(k + 1) * 4194304u; // (k + 1) * (2^32 / 1024)

            if (doA && aA >= 1e-5f)
            {
                uint32_t phA = static_cast<uint32_t>(resynthEngine.getHarmonicPhaseRaw(0, k) * 4294967295.0f);
                for (int i = 0; i < Voice::WAVE_TABLE_SIZE; ++i)
                {
                    v.waveTableA[static_cast<size_t>(i)] += aA * sineLUT[phA >> (32 - 13)];
                    phA += step;
                }
            }

            if (doB && aB >= 1e-5f)
            {
                uint32_t phB = static_cast<uint32_t>(resynthEngine.getHarmonicPhaseRaw(1, k) * 4294967295.0f + 0x40000000u);
                for (int i = 0; i < Voice::WAVE_TABLE_SIZE; ++i)
                {
                    v.waveTableB[static_cast<size_t>(i)] += aB * sineLUT[phB >> (32 - 13)];
                    phB += step;
                }
            }
        }

        // Master voice gain (panning & volume envelope & calibrated polyphonic headroom)
        v.targetGain = juce::jlimit(0.0f, 0.35f, v.releaseEnv * volEnvMod * velScale * 0.26f);
    }

    void renderChunk(float* outL, float* outR, int numSamples, const SynthParameters& params)
    {
        juce::ScopedNoDenormals noDenormals;
        const float dtSeconds = static_cast<float>(numSamples) * invSampleRate;
        const float invChunk = 1.0f / static_cast<float>(numSamples);

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

        Voice* newestActiveVoice = nullptr;
        int activeCount = 0;

        // Dynamic Release Acceleration: Count active decaying tails
        int releasingCount = 0;
        for (const auto& vCheck : voices)
        {
            if (vCheck.active && vCheck.releasing)
                ++releasingCount;
        }

        for (auto& v : voices)
        {
            if (!v.active)
                continue;

            // Smart Voice Sleeping: If releasing voice fell below -80 dBFS, sleep it immediately
            if (v.releasing && v.curGain < 0.0001f && v.targetGain < 0.0001f)
            {
                v.reset();
                continue;
            }

            updateVoiceSpectralTables(v, dtSeconds, params, uDet, uSpr, uBle, uVoi, mixA, mixB, releasingCount);
            if (!v.active)
                continue;

            if (v.releasing && v.curGain < 0.0001f && v.targetGain < 0.0001f)
            {
                v.reset();
                continue;
            }

            ++activeCount;
            if (newestActiveVoice == nullptr || v.noteAgeSeconds < newestActiveVoice->noteAgeSeconds)
                newestActiveVoice = &v;

            float curGain = v.curGain;
            const float targetGain = v.targetGain;
            const float stepGain = (targetGain - curGain) * invChunk;

            // Determine active sub-oscillator count for this voice
            int maxU = 1;
            for (int u = 0; u < MAX_UNISON; ++u)
            {
                if (v.curUniVoiceWeight[static_cast<size_t>(u)] > 0.0005f)
                    maxU = u + 1;
            }

            for (int s = 0; s < numSamples; ++s)
            {
                const float crossNew = static_cast<float>(s + 1) * invChunk;
                const float crossOld = 1.0f - crossNew;
                float sumVoiceL = 0.0f;
                float sumVoiceR = 0.0f;

                if (doA && !doB)
                {
                    for (int u = 0; u < maxU; ++u)
                    {
                        const size_t uIdx = static_cast<size_t>(u);
                        const float wL = v.uWeightL[uIdx];
                        const float wR = v.uWeightR[uIdx];
                        if (std::abs(wL) + std::abs(wR) < 1e-5f)
                        {
                            v.subPhaseA[uIdx] += v.subPhaseIncA[uIdx];
                            continue;
                        }

                        // Fast 1024-sample wavetable lookup with chunk crossfade (Part A only)
                        const uint32_t phA = v.subPhaseA[uIdx];
                        const uint32_t idxA0 = phA >> (32 - 10);
                        const uint32_t idxA1 = (idxA0 + 1) & (Voice::WAVE_TABLE_SIZE - 1);
                        const float fracA = static_cast<float>(phA & 0x003FFFFF) * (1.0f / 4194304.0f);
                        const float sA_old = v.prevWaveTableA[idxA0] * (1.0f - fracA) + v.prevWaveTableA[idxA1] * fracA;
                        const float sA_new = v.waveTableA[idxA0] * (1.0f - fracA) + v.waveTableA[idxA1] * fracA;
                        const float sampleA = sA_old * crossOld + sA_new * crossNew;

                        const float subSig = sampleA * mixA;
                        sumVoiceL += subSig * wL;
                        sumVoiceR += subSig * wR;

                        v.subPhaseA[uIdx] += v.subPhaseIncA[uIdx];
                    }
                }
                else if (!doA && doB)
                {
                    for (int u = 0; u < maxU; ++u)
                    {
                        const size_t uIdx = static_cast<size_t>(u);
                        const float wL = v.uWeightL[uIdx];
                        const float wR = v.uWeightR[uIdx];
                        if (std::abs(wL) + std::abs(wR) < 1e-5f)
                        {
                            v.subPhaseB[uIdx] += v.subPhaseIncB[uIdx];
                            continue;
                        }

                        // Fast 1024-sample wavetable lookup with chunk crossfade (Part B only)
                        const uint32_t phB = v.subPhaseB[uIdx];
                        const uint32_t idxB0 = phB >> (32 - 10);
                        const uint32_t idxB1 = (idxB0 + 1) & (Voice::WAVE_TABLE_SIZE - 1);
                        const float fracB = static_cast<float>(phB & 0x003FFFFF) * (1.0f / 4194304.0f);
                        const float sB_old = v.prevWaveTableB[idxB0] * (1.0f - fracB) + v.prevWaveTableB[idxB1] * fracB;
                        const float sB_new = v.waveTableB[idxB0] * (1.0f - fracB) + v.waveTableB[idxB1] * fracB;
                        const float sampleB = sB_old * crossOld + sB_new * crossNew;

                        const float subSig = sampleB * mixB;
                        sumVoiceL += subSig * wL;
                        sumVoiceR += subSig * wR;

                        v.subPhaseB[uIdx] += v.subPhaseIncB[uIdx];
                    }
                }
                else
                {
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

                        // Linear interpolation lookup with chunk crossfade
                        const uint32_t phA = v.subPhaseA[uIdx];
                        const uint32_t idxA0 = phA >> (32 - 10);
                        const uint32_t idxA1 = (idxA0 + 1) & (Voice::WAVE_TABLE_SIZE - 1);
                        const float fracA = static_cast<float>(phA & 0x003FFFFF) * (1.0f / 4194304.0f);
                        const float sA_old = v.prevWaveTableA[idxA0] * (1.0f - fracA) + v.prevWaveTableA[idxA1] * fracA;
                        const float sA_new = v.waveTableA[idxA0] * (1.0f - fracA) + v.waveTableA[idxA1] * fracA;
                        const float sampleA = sA_old * crossOld + sA_new * crossNew;

                        const uint32_t phB = v.subPhaseB[uIdx];
                        const uint32_t idxB0 = phB >> (32 - 10);
                        const uint32_t idxB1 = (idxB0 + 1) & (Voice::WAVE_TABLE_SIZE - 1);
                        const float fracB = static_cast<float>(phB & 0x003FFFFF) * (1.0f / 4194304.0f);
                        const float sB_old = v.prevWaveTableB[idxB0] * (1.0f - fracB) + v.prevWaveTableB[idxB1] * fracB;
                        const float sB_new = v.waveTableB[idxB0] * (1.0f - fracB) + v.waveTableB[idxB1] * fracB;
                        const float sampleB = sB_old * crossOld + sB_new * crossNew;

                        const float subSig = sampleA * mixA + sampleB * mixB;
                        sumVoiceL += subSig * wL;
                        sumVoiceR += subSig * wR;

                        v.subPhaseA[uIdx] += v.subPhaseIncA[uIdx];
                        v.subPhaseB[uIdx] += v.subPhaseIncB[uIdx];
                    }
                }

                if (std::isnan(sumVoiceL) || std::isinf(sumVoiceL)) sumVoiceL = 0.0f;
                if (std::isnan(sumVoiceR) || std::isinf(sumVoiceR)) sumVoiceR = 0.0f;

                const float voiceL = sumVoiceL * curGain * panGainL;
                const float voiceR = sumVoiceR * curGain * panGainR;

                // Per-Voice Pre-Summation Soft Clipper: guarantees each individual voice stays smoothly bounded <= 1.0
                outL[s] += std::tanh(voiceL);
                outR[s] += std::tanh(voiceR);
                curGain += stepGain;
            }

            v.curGain = targetGain;
            v.prevWaveTableA = v.waveTableA;
            v.prevWaveTableB = v.waveTableB;

            // Keep partial amplitude telemetry updated for visualizer
            for (int k = 0; k < NUM_PARTIALS; ++k)
            {
                v.curAmpA[static_cast<size_t>(k)] = v.targetAmpA[static_cast<size_t>(k)];
                v.curAmpB[static_cast<size_t>(k)] = v.targetAmpB[static_cast<size_t>(k)];
            }
        }

        // Update spectral telemetry in pending snapshot
        pendingSnapshot.activeVoiceCount = activeCount;
        pendingSnapshot.activeArpStep = currentArpStepAtomic.load(std::memory_order_relaxed);
        if (newestActiveVoice != nullptr)
        {
            pendingSnapshot.voiceActive = true;
            pendingSnapshot.envCursors = newestActiveVoice->artPhase;
            pendingSnapshot.envCursor = newestActiveVoice->artPhase[0];
            pendingSnapshot.activePitchHz = newestActiveVoice->currentBaseFreq;
            latestVoiceEnvGain = newestActiveVoice->targetVolEnv * newestActiveVoice->releaseEnv;

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
                pendingSnapshot.filterCurve[k] = juce::jlimit(0.0f, 1.0f, newestActiveVoice->filterGainTable[k] * 0.65f);
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

    // -------------------------------------------------------------------------
    // Dedicated Serum-2 Style Noise Generator + Dynamic Multi-FX Chain
    // -------------------------------------------------------------------------
    void processNoiseAndMasterFXChain(float* outL, float* outR, int numSamples, int numChannels, const SynthParameters& params)
    {
        juce::ScopedNoDenormals noDenormals;
        // 1. Serum-2 Style Noise Oscillator
        if (params.noiseEnable && params.noiseLevel > 0.002f && pendingSnapshot.activeVoiceCount > 0)
        {
            const float nGain = params.noiseLevel * latestVoiceEnvGain * 0.45f;
            const float nPan = juce::jlimit(-1.0f, 1.0f, params.noisePan);
            const float panL = std::cos((nPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            const float panR = std::sin((nPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            const float stereoDecorrel = params.noisePhase;

            const float basePitchHz = pendingSnapshot.activePitchHz;
            const float bitHoldRateHz = (250.0f + params.noisePitch * 14000.0f)
                                      * std::pow(std::max(40.0f, basePitchHz) / 261.63f, params.noiseKeyTrack);
            const float bitHoldStep = std::min(1.0f, bitHoldRateHz * invSampleRate);

            const float filtAmt = juce::jlimit(-1.0f, 1.0f, params.noiseFilter);
            const float lpCut = (filtAmt < 0.0f) ? std::pow(1.0f + filtAmt, 2.2f) * 0.85f + 0.02f : 0.98f;
            const float hpCut = (filtAmt > 0.0f) ? (filtAmt * 0.45f) : 0.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float wL = rng.nextFloat() * 2.0f - 1.0f;
                const float wR = (1.0f - stereoDecorrel) * wL + stereoDecorrel * (rng.nextFloat() * 2.0f - 1.0f);
                float sigL = wL;
                float sigR = wR;

                switch (params.noiseType)
                {
                    case 0: // White Noise
                        break;

                    case 1: // Pink Noise
                        pinkB0 = 0.99765f * pinkB0 + wL * 0.0990460f;
                        pinkB1 = 0.96300f * pinkB1 + wL * 0.2965164f;
                        pinkB2 = 0.57000f * pinkB2 + wL * 1.0526913f;
                        sigL = (pinkB0 + pinkB1 + pinkB2 + wL * 0.1848f) * 0.25f;
                        sigR = sigL * (1.0f - 0.35f * stereoDecorrel) + wR * 0.35f * stereoDecorrel;
                        break;

                    case 2: // Brown Noise
                        brownStateL = juce::jlimit(-1.0f, 1.0f, (brownStateL + 0.045f * wL) * 0.985f);
                        brownStateR = juce::jlimit(-1.0f, 1.0f, (brownStateR + 0.045f * wR) * 0.985f);
                        sigL = brownStateL * 1.8f;
                        sigR = brownStateR * 1.8f;
                        break;

                    case 3: // Vinyl Crackle
                    {
                        brownStateL = (brownStateL + 0.025f * wL) * 0.98f;
                        const float popL = (rng.nextFloat() > (0.9965f - params.noisePitch * 0.008f)) ? (wL * 2.4f) : 0.0f;
                        const float popR = (rng.nextFloat() > (0.9965f - params.noisePitch * 0.008f)) ? (wR * 2.4f) : 0.0f;
                        sigL = brownStateL * 0.65f + popL;
                        sigR = brownStateL * 0.65f + popR;
                        break;
                    }

                    case 4: // Digital / Bitnoise
                        bitHoldPhase += bitHoldStep;
                        if (bitHoldPhase >= 1.0f)
                        {
                            bitHoldPhase -= 1.0f;
                            bitHoldSampleL = (wL >= 0.0f ? 0.8f : -0.8f) + 0.2f * wL;
                            bitHoldSampleR = (wR >= 0.0f ? 0.8f : -0.8f) + 0.2f * wR;
                        }
                        sigL = bitHoldSampleL;
                        sigR = bitHoldSampleR;
                        break;

                    case 5: // Breath / Air
                    default:
                    {
                        const float airCut = 0.08f + params.noisePitch * 0.35f;
                        airBandL += airCut * (wL - airBandL - 0.35f * airLowL);
                        airLowL  += airCut * airBandL;
                        airBandR += airCut * (wR - airBandR - 0.35f * airLowR);
                        airLowR  += airCut * airBandR;
                        sigL = airBandL * 1.8f;
                        sigR = airBandR * 1.8f;
                        break;
                    }
                }

                noiseLpStateL += lpCut * (sigL - noiseLpStateL);
                noiseLpStateR += lpCut * (sigR - noiseLpStateR);
                noiseHpStateL += hpCut * (noiseLpStateL - noiseHpStateL);
                noiseHpStateR += hpCut * (noiseLpStateR - noiseHpStateR);

                const float fOutL = (filtAmt > 0.0f) ? (noiseLpStateL - noiseHpStateL) : noiseLpStateL;
                const float fOutR = (filtAmt > 0.0f) ? (noiseLpStateR - noiseHpStateR) : noiseLpStateR;

                outL[i] += fOutL * nGain * panL;
                if (numChannels > 1)
                    outR[i] += fOutR * nGain * panR;
            }
        }

        // 2. Filter Drive / Saturation (Warm strictly monotonic analog saturation, no parabolic foldover)
        if (params.filterDrive > 0.005f)
        {
            const float driveAmt = juce::jlimit(0.0f, 1.0f, params.filterDrive);
            const float drivePreGain = 1.0f + driveAmt * 3.5f;
            const float driveComp = 1.0f / (1.0f + driveAmt * 1.15f);
            const float asymFactor = 1.0f + 0.20f * driveAmt;
            for (int i = 0; i < numSamples; ++i)
            {
                const float xL = juce::jlimit(-12.0f, 12.0f, outL[i] * drivePreGain);
                const float xR = juce::jlimit(-12.0f, 12.0f, outR[i] * drivePreGain);
                const float satL = (xL >= 0.0f ? std::tanh(xL * asymFactor) : std::tanh(xL / asymFactor)) * driveComp;
                const float satR = (xR >= 0.0f ? std::tanh(xR * asymFactor) : std::tanh(xR / asymFactor)) * driveComp;
                outL[i] = outL[i] * (1.0f - driveAmt) + satL * driveAmt;
                if (numChannels > 1)
                    outR[i] = outR[i] * (1.0f - driveAmt) + satR * driveAmt;
            }
        }

        // 2.5. Gross Beat / ShaperBox 3 Circular Buffer Time Modulation Engine
        processTimeModulation(outL, outR, numSamples, numChannels, params);

        // 3. Execute Dynamic Modular FX Rack in User-Configured Order (Lock-Free & Zero-Allocation!)
        pendingSnapshot.compGainReductionDb = 0.0f;
        const int chainCount = activeFxChainCount.load(std::memory_order_acquire);
        for (int i = 0; i < chainCount; ++i)
        {
            const int moduleId = activeFxChainArray[static_cast<size_t>(i)];
            processSingleFxModule(static_cast<FxModuleId>(moduleId), outL, outR, numSamples, numChannels, params);
        }

        // 4. Master Gain + Smooth Studio Brickwall Limiter + Peak/RMS/GR Metering
        const float masterLinearGain = juce::Decibels::decibelsToGain(params.masterGainDb);
        const float ceilingLinear = juce::Decibels::decibelsToGain(juce::jlimit(-18.0f, 0.0f, params.limiterThreshDb));

        float maxPeakL = 0.0f;
        float maxPeakR = 0.0f;
        float sumSqL = 0.0f;
        float sumSqR = 0.0f;
        float maxGainReductionDb = 0.0f;

        const float attackCoeff = 1.0f - std::exp(-invSampleRate / 0.002f); // 2ms smooth attack
        const float releaseCoeff = 1.0f - std::exp(-invSampleRate / 0.080f); // 80ms smooth release

        for (int i = 0; i < numSamples; ++i)
        {
            float sL = outL[i] * masterLinearGain;
            float sR = ((numChannels > 1) ? outR[i] : outL[i]) * masterLinearGain;

            if (params.limiterEnable)
            {
                const float absMax = std::max(std::abs(sL), std::abs(sR));
                const float targetLimiterGain = (absMax > ceilingLinear && absMax > 1e-5f)
                    ? (ceilingLinear / absMax)
                    : 1.0f;

                if (targetLimiterGain < limiterEnv)
                    limiterEnv += attackCoeff * (targetLimiterGain - limiterEnv);
                else
                    limiterEnv += releaseCoeff * (targetLimiterGain - limiterEnv);

                sL *= limiterEnv;
                sR *= limiterEnv;

                // C1-continuous soft saturation catching fast peaks with zero slope or step discontinuity
                const float thresh = ceilingLinear * 0.90f;
                const float margin = ceilingLinear - thresh;
                if (std::abs(sL) > thresh)
                {
                    const float sign = (sL >= 0.0f) ? 1.0f : -1.0f;
                    sL = sign * (thresh + margin * std::tanh((std::abs(sL) - thresh) / margin));
                }
                if (std::abs(sR) > thresh)
                {
                    const float sign = (sR >= 0.0f) ? 1.0f : -1.0f;
                    sR = sign * (thresh + margin * std::tanh((std::abs(sR) - thresh) / margin));
                }

                const float grDb = (limiterEnv < 0.999f)
                    ? -juce::Decibels::gainToDecibels(std::max(0.01f, limiterEnv))
                    : 0.0f;
                if (grDb > maxGainReductionDb)
                    maxGainReductionDb = grDb;
            }
            else
            {
                // Smooth zero-latency soft clipper when limiter is bypassed
                if (std::abs(sL) > 0.90f)
                {
                    const float sign = (sL >= 0.0f) ? 1.0f : -1.0f;
                    sL = sign * (0.90f + 0.10f * std::tanh((std::abs(sL) - 0.90f) * 10.0f));
                }
                if (std::abs(sR) > 0.90f)
                {
                    const float sign = (sR >= 0.0f) ? 1.0f : -1.0f;
                    sR = sign * (0.90f + 0.10f * std::tanh((std::abs(sR) - 0.90f) * 10.0f));
                }
            }

            // Hard output ceiling protection: guaranteed strictly <= 0 dBFS
            sL = juce::jlimit(-1.0f, 1.0f, sL);
            sR = juce::jlimit(-1.0f, 1.0f, sR);

            outL[i] = sL;
            if (numChannels > 1)
                outR[i] = sR;

            maxPeakL = std::max(maxPeakL, std::abs(sL));
            maxPeakR = std::max(maxPeakR, std::abs(sR));
            sumSqL += sL * sL;
            sumSqR += sR * sR;
        }

        const float invN = 1.0f / static_cast<float>(std::max(1, numSamples));
        pendingSnapshot.peakLeft  = juce::jlimit(0.0f, 1.2f, maxPeakL);
        pendingSnapshot.peakRight = juce::jlimit(0.0f, 1.2f, maxPeakR);
        pendingSnapshot.rmsLeft   = juce::jlimit(0.0f, 1.2f, std::sqrt(sumSqL * invN));
        pendingSnapshot.rmsRight  = juce::jlimit(0.0f, 1.2f, std::sqrt(sumSqR * invN));
        pendingSnapshot.limiterGainReductionDb = juce::jlimit(0.0f, 24.0f, maxGainReductionDb);

        int finalActiveVoiceCount = 0;
        for (const auto& v : voices)
        {
            if (v.active)
                ++finalActiveVoiceCount;
        }
        // Polyphonic Active Voice Allocation Telemetry: reports true active MIDI note count (1 when 1 note played!)
        pendingSnapshot.activeVoiceCount = finalActiveVoiceCount;
        activeVoicesAtomic.store(finalActiveVoiceCount, std::memory_order_relaxed);

        const int writeVis = 1 - activeVisIndex.load(std::memory_order_relaxed);
        visSnapshots[static_cast<size_t>(writeVis)] = pendingSnapshot;
        activeVisIndex.store(writeVis, std::memory_order_release);
    }

    void processSingleFxModule(FxModuleId modId, float* outL, float* outR, int numSamples, int numChannels, const SynthParameters& params)
    {
        switch (modId)
        {
            case FxModuleId::Blur:
                break;

            case FxModuleId::Tremolo:
            {
                if (!params.tremEnable || params.tremDepth <= 0.005f)
                    break;
                const float tremInc = (params.tremSpeed * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float tremStereoOffset = params.tremGap * juce::MathConstants<float>::pi;
                for (int i = 0; i < numSamples; ++i)
                {
                    tremMasterPhase += tremInc;
                    if (tremMasterPhase > juce::MathConstants<float>::twoPi)
                        tremMasterPhase -= juce::MathConstants<float>::twoPi;

                    const float gL = 1.0f - params.tremDepth * 0.85f * (0.5f + 0.5f * fastSin(tremMasterPhase));
                    const float gR = 1.0f - params.tremDepth * 0.85f * (0.5f + 0.5f * fastSin(tremMasterPhase + tremStereoOffset));
                    outL[i] *= gL;
                    if (numChannels > 1)
                        outR[i] *= gR;
                }
                break;
            }

            case FxModuleId::Distortion:
            {
                if (!params.distEnable || params.distMix <= 0.005f)
                    break;
                const float drive = 1.0f + params.distDrive * 9.5f;
                const float mix = juce::jlimit(0.0f, 1.0f, params.distMix);
                const float toneCoeff = 0.08f + params.distTone * 0.88f;
                const float comp = 1.0f / (1.0f + params.distDrive * 1.4f);

                for (int i = 0; i < numSamples; ++i)
                {
                    float inL = outL[i];
                    float inR = (numChannels > 1) ? outR[i] : inL;
                    if (std::isnan(inL) || std::isinf(inL)) inL = 0.0f;
                    if (std::isnan(inR) || std::isinf(inR)) inR = 0.0f;
                    const float xL = inL * drive;
                    const float xR = inR * drive;
                    float wL = xL;
                    float wR = xR;

                    if (params.distType == 0) // Tube Saturation (Strictly monotonic, rich even harmonics)
                    {
                        const float asym = 1.0f + 0.25f * params.distDrive;
                        wL = (xL >= 0.0f ? std::tanh(xL * asym) : std::tanh(xL / asym)) * comp;
                        wR = (xR >= 0.0f ? std::tanh(xR * asym) : std::tanh(xR / asym)) * comp;
                    }
                    else if (params.distType == 1) // Tape Warmth
                    {
                        wL = (2.0f / juce::MathConstants<float>::pi) * std::atan(xL * 1.35f) * comp;
                        wR = (2.0f / juce::MathConstants<float>::pi) * std::atan(xR * 1.35f) * comp;
                    }
                    else if (params.distType == 2) // Hard Clip
                    {
                        wL = juce::jlimit(-0.85f, 0.85f, xL) * comp;
                        wR = juce::jlimit(-0.85f, 0.85f, xR) * comp;
                    }
                    else if (params.distType == 3) // Wavefold
                    {
                        wL = fastSin(xL * 1.6f) * comp;
                        wR = fastSin(xR * 1.6f) * comp;
                    }
                    else if (params.distType == 4) // Bitcrush Fuzz
                    {
                        const float levels = std::max(3.0f, 24.0f * (1.0f - params.distDrive * 0.82f));
                        wL = (std::round(std::tanh(xL) * levels) / levels) * comp * 1.15f;
                        wR = (std::round(std::tanh(xR) * levels) / levels) * comp * 1.15f;
                    }
                    else // Asymmetric Acid Scream
                    {
                        wL = ((xL >= 0.0f) ? std::tanh(xL * 1.6f) : (std::atan(xL * 0.8f) * 0.7f)) * comp * 1.2f;
                        wR = ((xR >= 0.0f) ? std::tanh(xR * 1.6f) : (std::atan(xR * 0.8f) * 0.7f)) * comp * 1.2f;
                    }

                    wL = juce::jlimit(-1.5f, 1.5f, wL);
                    wR = juce::jlimit(-1.5f, 1.5f, wR);

                    distToneStateL += toneCoeff * (wL - distToneStateL);
                    distToneStateR += toneCoeff * (wR - distToneStateR);

                    outL[i] = inL * (1.0f - mix) + distToneStateL * mix;
                    if (numChannels > 1)
                        outR[i] = inR * (1.0f - mix) + distToneStateR * mix;
                }
                break;
            }

            case FxModuleId::Phaser:
            {
                if (!params.phaserEnable || params.phaserMix <= 0.005f)
                    break;
                const float phaserInc = (params.phaserSpeed * invSampleRate) * juce::MathConstants<float>::twoPi;
                for (int i = 0; i < numSamples; ++i)
                {
                    phaserLfoPhase += phaserInc;
                    if (phaserLfoPhase > juce::MathConstants<float>::twoPi)
                        phaserLfoPhase -= juce::MathConstants<float>::twoPi;

                    const float modL = 0.5f + 0.5f * fastSin(phaserLfoPhase + params.phaserOffset * 6.28318f);
                    const float modR = 0.5f + 0.5f * fastCos(phaserLfoPhase + params.phaserOffset * 6.28318f);

                    const float minF = 180.0f;
                    const float maxF = 4200.0f * (0.3f + 0.7f * params.phaserWidth);
                    const float fL = minF + (maxF - minF) * modL;
                    const float fR = minF + (maxF - minF) * modR;

                    const float tL = std::tan(juce::MathConstants<float>::pi * std::min(0.45f, fL * invSampleRate));
                    const float tR = std::tan(juce::MathConstants<float>::pi * std::min(0.45f, fR * invSampleRate));
                    const float aL = (1.0f - tL) / (1.0f + tL);
                    const float aR = (1.0f - tR) / (1.0f + tR);

                    float yL = outL[i];
                    float yR = (numChannels > 1) ? outR[i] : yL;
                    for (size_t st = 0; st < 6; ++st)
                    {
                        const float outApL = -aL * yL + phaserApStateL[st];
                        phaserApStateL[st] = yL + aL * outApL;
                        yL = outApL;

                        const float outApR = -aR * yR + phaserApStateR[st];
                        phaserApStateR[st] = yR + aR * outApR;
                        yR = outApR;
                    }

                    outL[i] += params.phaserMix * 0.75f * yL;
                    if (numChannels > 1)
                        outR[i] += params.phaserMix * 0.75f * yR;
                }
                break;
            }

            case FxModuleId::Chorus:
            {
                if (!params.chorusEnable || params.chorusMix <= 0.005f)
                    break;
                const int voicesCount = juce::jlimit(2, 6, params.chorusVoices);
                const float rateInc = (params.chorusRate * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float baseDelaySamples = static_cast<float>(sampleRate) * 0.014f;
                const float depthSamples = static_cast<float>(sampleRate) * (0.001f + params.chorusDepth * 0.011f);
                const float mix = juce::jlimit(0.0f, 1.0f, params.chorusMix);
                const float spread = juce::jlimit(0.0f, 1.0f, params.chorusSpread);
                const float vNorm = 1.0f / std::sqrt(static_cast<float>(voicesCount));

                for (int i = 0; i < numSamples; ++i)
                {
                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;

                    chorusBufL[static_cast<size_t>(chorusWritePos)] = inL;
                    chorusBufR[static_cast<size_t>(chorusWritePos)] = inR;

                    chorusPhase += rateInc;
                    if (chorusPhase > juce::MathConstants<float>::twoPi)
                        chorusPhase -= juce::MathConstants<float>::twoPi;

                    float wetL = 0.0f;
                    float wetR = 0.0f;

                    for (int v = 0; v < voicesCount; ++v)
                    {
                        const float vPhase = chorusPhase + (static_cast<float>(v) / static_cast<float>(voicesCount)) * juce::MathConstants<float>::twoPi;
                        const float dSamp = baseDelaySamples + fastSin(vPhase) * depthSamples;
                        float readIdx = static_cast<float>(chorusWritePos) - dSamp;
                        while (readIdx < 0.0f)
                            readIdx += static_cast<float>(CHORUS_BUFFER_SIZE);

                        const int r0 = static_cast<int>(readIdx) & (CHORUS_BUFFER_SIZE - 1);
                        const int r1 = (r0 + 1) & (CHORUS_BUFFER_SIZE - 1);
                        const float frac = readIdx - std::floor(readIdx);

                        const float tapL = chorusBufL[static_cast<size_t>(r0)] * (1.0f - frac) + chorusBufL[static_cast<size_t>(r1)] * frac;
                        const float tapR = chorusBufR[static_cast<size_t>(r0)] * (1.0f - frac) + chorusBufR[static_cast<size_t>(r1)] * frac;

                        const float panV = (v & 1) ? spread : -spread;
                        wetL += tapL * (0.5f - 0.5f * panV) + tapR * (0.5f + 0.5f * panV);
                        wetR += tapR * (0.5f + 0.5f * panV) + tapL * (0.5f - 0.5f * panV);
                    }

                    wetL *= vNorm;
                    wetR *= vNorm;

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    if (numChannels > 1)
                        outR[i] = inR * (1.0f - mix) + wetR * mix;

                    chorusWritePos = (chorusWritePos + 1) & (CHORUS_BUFFER_SIZE - 1);
                }
                break;
            }

            case FxModuleId::ParametricEQ:
                break;

            case FxModuleId::Delay:
            {
                if (!params.delayEnable || params.delayMix <= 0.005f)
                    break;
                const float beatSec = 60.0f / static_cast<float>(juce::jlimit(40.0, 260.0, params.hostBpm));
                const float syncMult = 0.125f + std::round(params.delayTime * 7.0f) * 0.125f;
                const float delaySecL = juce::jlimit(0.02f, 1.8f, beatSec * syncMult);
                const float delaySecR = juce::jlimit(0.02f, 1.8f, delaySecL * (1.0f + params.delayPingPong * 0.25f));

                const int dSampL = juce::jlimit(32, MAX_DELAY_SAMPLES - 4, static_cast<int>(delaySecL * static_cast<float>(sampleRate)));
                const int dSampR = juce::jlimit(32, MAX_DELAY_SAMPLES - 4, static_cast<int>(delaySecR * static_cast<float>(sampleRate)));

                const float fb = juce::jlimit(0.0f, 0.86f, params.delayFeedback);
                const float pp = juce::jlimit(0.0f, 1.0f, params.delayPingPong);
                const float damp = 0.08f + params.delayFilter * 0.85f;
                const float mix = juce::jlimit(0.0f, 1.0f, params.delayMix);

                for (int i = 0; i < numSamples; ++i)
                {
                    const int readL = (delayWritePos - dSampL + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
                    const int readR = (delayWritePos - dSampR + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;

                    float tapL = delayBufL[static_cast<size_t>(readL)];
                    float tapR = delayBufR[static_cast<size_t>(readR)];
                    if (std::isnan(tapL) || std::isinf(tapL)) tapL = 0.0f;
                    if (std::isnan(tapR) || std::isinf(tapR)) tapR = 0.0f;

                    delayLpStateL += damp * (tapL - delayLpStateL);
                    delayLpStateR += damp * (tapR - delayLpStateR);
                    if (std::isnan(delayLpStateL) || std::isinf(delayLpStateL)) delayLpStateL = 0.0f;
                    if (std::isnan(delayLpStateR) || std::isinf(delayLpStateR)) delayLpStateR = 0.0f;

                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;

                    const float fbInL = delayLpStateL * (1.0f - pp * 0.7f) + delayLpStateR * (pp * 0.7f);
                    const float fbInR = delayLpStateR * (1.0f - pp * 0.7f) + delayLpStateL * (pp * 0.7f);

                    delayBufL[static_cast<size_t>(delayWritePos)] = std::tanh((inL + fbInL * fb) * 0.75f) * 1.33f;
                    delayBufR[static_cast<size_t>(delayWritePos)] = std::tanh((inR + fbInR * fb) * 0.75f) * 1.33f;

                    outL[i] = inL + delayLpStateL * mix;
                    if (numChannels > 1)
                        outR[i] = inR + delayLpStateR * mix;

                    delayWritePos = (delayWritePos + 1) % MAX_DELAY_SAMPLES;
                }
                break;
            }

            case FxModuleId::Reverb:
            {
                if (!params.reverbEnable || params.reverbMix <= 0.005f)
                    break;

                const float mix = juce::jlimit(0.0f, 1.0f, params.reverbMix);
                const float fb = juce::jlimit(0.40f, 0.88f, 0.50f + params.reverbDecay * 0.38f);
                const float damp = 0.12f + (1.0f - params.reverbDamping) * 0.75f;
                const float shimmerAmt = params.reverbShimmer * 0.32f;
                const int preDelaySamples = juce::jlimit(1, 4094, static_cast<int>(params.reverbPreDelay * 0.085f * static_cast<float>(sampleRate)));
                const float sizeScale = 0.55f + params.reverbSize * 0.45f;

                const int cLen0 = std::max(64, static_cast<int>(1116.0f * sizeScale));
                const int cLen1 = std::max(64, static_cast<int>(1356.0f * sizeScale));
                const int cLen2 = std::max(64, static_cast<int>(1491.0f * sizeScale));
                const int cLen3 = std::max(64, static_cast<int>(1617.0f * sizeScale));

                for (int i = 0; i < numSamples; ++i)
                {
                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;

                    revPreBufL[static_cast<size_t>(revPrePos)] = inL;
                    revPreBufR[static_cast<size_t>(revPrePos)] = inR;
                    const int preRead = (revPrePos - preDelaySamples + 4096) & 4095;
                    const float preL = revPreBufL[static_cast<size_t>(preRead)];
                    const float preR = revPreBufR[static_cast<size_t>(preRead)];
                    revPrePos = (revPrePos + 1) & 4095;

                    auto runComb = [&](std::array<float, 2048>& bufL, std::array<float, 2048>& bufR,
                                       int& pos, int len, float& lpL, float& lpR, float& sumL, float& sumR)
                    {
                        const float yL = bufL[static_cast<size_t>(pos)];
                        const float yR = bufR[static_cast<size_t>(pos)];
                        lpL += damp * (yL - lpL);
                        lpR += damp * (yR - lpR);
                        if (std::isnan(lpL) || std::isinf(lpL)) lpL = 0.0f;
                        if (std::isnan(lpR) || std::isinf(lpR)) lpR = 0.0f;
                        const float shimL = (std::abs(lpL) - 0.1f * lpL) * shimmerAmt;
                        const float shimR = (std::abs(lpR) - 0.1f * lpR) * shimmerAmt;
                        bufL[static_cast<size_t>(pos)] = std::tanh((preL * 0.25f + (lpL + shimL) * fb) * 0.75f) * 1.33f;
                        bufR[static_cast<size_t>(pos)] = std::tanh((preR * 0.25f + (lpR + shimR) * fb) * 0.75f) * 1.33f;
                        pos = (pos + 1) % len;
                        sumL += yL;
                        sumR += yR;
                    };

                    float tankL = 0.0f;
                    float tankR = 0.0f;
                    runComb(revCombBufL0, revCombBufR0, revPos0, cLen0, revLpL0, revLpR0, tankL, tankR);
                    runComb(revCombBufL1, revCombBufR1, revPos1, cLen1, revLpL1, revLpR1, tankL, tankR);
                    runComb(revCombBufL2, revCombBufR2, revPos2, cLen2, revLpL2, revLpR2, tankL, tankR);
                    runComb(revCombBufL3, revCombBufR3, revPos3, cLen3, revLpL3, revLpR3, tankL, tankR);

                    const float apOutL0 = -0.5f * tankL + revApBufL0[static_cast<size_t>(revApPos0)];
                    const float apOutR0 = -0.5f * tankR + revApBufR0[static_cast<size_t>(revApPos0)];
                    revApBufL0[static_cast<size_t>(revApPos0)] = tankL + 0.5f * apOutL0;
                    revApBufR0[static_cast<size_t>(revApPos0)] = tankR + 0.5f * apOutR0;
                    revApPos0 = (revApPos0 + 1) % 347;

                    const float apOutL1 = -0.5f * apOutL0 + revApBufL1[static_cast<size_t>(revApPos1)];
                    const float apOutR1 = -0.5f * apOutR0 + revApBufR1[static_cast<size_t>(revApPos1)];
                    revApBufL1[static_cast<size_t>(revApPos1)] = apOutL0 + 0.5f * apOutL1;
                    revApBufR1[static_cast<size_t>(revApPos1)] = apOutR0 + 0.5f * apOutR1;
                    revApPos1 = (revApPos1 + 1) % 113;

                    outL[i] = inL * (1.0f - mix * 0.45f) + apOutL1 * mix * 0.65f;
                    if (numChannels > 1)
                        outR[i] = inR * (1.0f - mix * 0.45f) + apOutR1 * mix * 0.65f;
                }
                break;
            }

            case FxModuleId::Compressor:
            {
                if (!params.compEnable || params.compMix <= 0.005f)
                    break;

                const float threshDb = juce::jlimit(-48.0f, 0.0f, params.compThreshold);
                const float ratio    = juce::jlimit(1.0f, 20.0f, params.compRatio);
                const float attSec   = std::max(0.0001f, params.compAttack * 0.001f);
                const float relSec   = std::max(0.005f,  params.compRelease * 0.001f);
                const float attCoeff = 1.0f - std::exp(-invSampleRate / attSec);
                const float relCoeff = 1.0f - std::exp(-invSampleRate / relSec);
                const float makeup   = juce::Decibels::decibelsToGain(params.compMakeup);
                const float mix      = juce::jlimit(0.0f, 1.0f, params.compMix);

                // Crossover coefficients for Multiband OTT mode (160Hz Low/Mid, 2500Hz Mid/High)
                const float xLowCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 160.0f * invSampleRate);
                const float xMidCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 2500.0f * invSampleRate);

                float maxCompGr = 0.0f;

                for (int i = 0; i < numSamples; ++i)
                {
                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;
                    float wetL = inL;
                    float wetR = inR;

                    if (params.compMode == 1) // Multiband OTT (Upward + Downward 3-Band Compression)
                    {
                        compLpLowL += xLowCoeff * (inL - compLpLowL);
                        compLpLowR += xLowCoeff * (inR - compLpLowR);
                        compLpMidL += xMidCoeff * (inL - compLpMidL);
                        compLpMidR += xMidCoeff * (inR - compLpMidR);

                        const float bandL[3] = { compLpLowL, compLpMidL - compLpLowL, inL - compLpMidL };
                        const float bandR[3] = { compLpLowR, compLpMidR - compLpLowR, inR - compLpMidR };
                        const float bandMakeup[3] = { 1.15f, 1.25f, 1.45f }; // Classic OTT high/low sheen

                        wetL = 0.0f;
                        wetR = 0.0f;

                        for (size_t b = 0; b < 3; ++b)
                        {
                            const float pk = std::max(std::abs(bandL[b]), std::abs(bandR[b]));
                            const float c = (pk > compEnvBand[b]) ? attCoeff : relCoeff;
                            compEnvBand[b] += c * (pk - compEnvBand[b]);

                            const float envDb = juce::Decibels::gainToDecibels(std::max(1e-4f, compEnvBand[b]));
                            float gainChangeDb = 0.0f;

                            if (envDb > threshDb)
                            {
                                // Downward compression above threshold
                                gainChangeDb = (threshDb - envDb) * (1.0f - 1.0f / ratio);
                                maxCompGr = std::max(maxCompGr, -gainChangeDb);
                            }
                            else if (envDb > -52.0f && envDb < (threshDb - 10.0f))
                            {
                                // Upward OTT compression pulling up quiet details
                                const float below = (threshDb - 10.0f) - envDb;
                                gainChangeDb = std::min(12.0f, below * 0.45f);
                            }

                            const float g = juce::Decibels::decibelsToGain(gainChangeDb) * bandMakeup[b];
                            wetL += bandL[b] * g;
                            wetR += bandR[b] * g;
                        }
                        wetL *= makeup * 0.78f;
                        wetR *= makeup * 0.78f;
                    }
                    else // Single-Band VCA / Opto / Punch Limiter
                    {
                        const float pk = std::max(std::abs(inL), std::abs(inR));
                        const float effAtt = (params.compMode == 2) ? (attCoeff * 0.35f) : attCoeff;
                        const float c = (pk > compEnvSingle) ? effAtt : relCoeff;
                        compEnvSingle += c * (pk - compEnvSingle);

                        const float envDb = juce::Decibels::gainToDecibels(std::max(1e-4f, compEnvSingle));
                        float gainChangeDb = 0.0f;
                        if (envDb > threshDb)
                        {
                            const float effRatio = (params.compMode == 3) ? std::max(10.0f, ratio) : ratio;
                            gainChangeDb = (threshDb - envDb) * (1.0f - 1.0f / effRatio);
                            maxCompGr = std::max(maxCompGr, -gainChangeDb);
                        }

                        const float g = juce::Decibels::decibelsToGain(gainChangeDb) * makeup;
                        wetL = inL * g;
                        wetR = inR * g;
                    }

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    if (numChannels > 1)
                        outR[i] = inR * (1.0f - mix) + wetR * mix;
                }

                pendingSnapshot.compGainReductionDb = juce::jlimit(0.0f, 24.0f, maxCompGr);
                break;
            }

            case FxModuleId::RingMod: // Zenology Modulation Pack (Ring Mod, Freq Shifter, Stereo Auto-Pan, Rotary)
            {
                if (!params.ringEnable || params.ringMix <= 0.005f)
                    break;

                const float mix = juce::jlimit(0.0f, 1.0f, params.ringMix);
                const float depth = juce::jlimit(0.0f, 1.0f, params.ringDepth);
                const float lfoInc = (params.ringLfoRate * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float baseFreq = juce::jlimit(20.0f, 2000.0f, params.ringFreq);

                for (int i = 0; i < numSamples; ++i)
                {
                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;

                    ringLfoPhase += lfoInc;
                    if (ringLfoPhase > juce::MathConstants<float>::twoPi)
                        ringLfoPhase -= juce::MathConstants<float>::twoPi;

                    const float lfoSin = fastSin(ringLfoPhase);
                    const float lfoCos = fastCos(ringLfoPhase);
                    const float modFreq = baseFreq * std::pow(2.0f, lfoSin * params.ringLfoDepth * 1.5f);
                    const float carInc = (modFreq * invSampleRate) * juce::MathConstants<float>::twoPi;

                    ringCarrierPhase += carInc;
                    if (ringCarrierPhase > juce::MathConstants<float>::twoPi)
                        ringCarrierPhase -= juce::MathConstants<float>::twoPi;

                    const float cSin = fastSin(ringCarrierPhase);
                    const float cCos = fastCos(ringCarrierPhase);

                    float wetL = inL;
                    float wetR = inR;

                    if (params.ringMode == 0) // Classic Diode Ring Modulator
                    {
                        const float modL = (1.0f - depth) + depth * cSin;
                        const float modR = (1.0f - depth) + depth * (cSin * 0.7f + cCos * 0.3f);
                        wetL = std::tanh(inL * modL * 1.35f);
                        wetR = std::tanh(inR * modR * 1.35f);
                    }
                    else if (params.ringMode == 1) // Zenology Frequency Shifter (Quadrature Sideband)
                    {
                        wetL = (inL * cCos - inR * cSin * depth) * 1.15f;
                        wetR = (inR * cCos + inL * cSin * depth) * 1.15f;
                    }
                    else if (params.ringMode == 2) // Stereo Auto-Pan + Harmonic Ring
                    {
                        const float panModL = 0.5f + 0.5f * lfoSin * depth;
                        const float panModR = 0.5f - 0.5f * lfoSin * depth;
                        const float subtleRing = (1.0f - params.ringLfoDepth * 0.5f) + params.ringLfoDepth * 0.5f * cSin;
                        wetL = inL * panModL * 1.4f * subtleRing;
                        wetR = inR * panModR * 1.4f * subtleRing;
                    }
                    else // Rotary Speaker Horn / Drum Modulation
                    {
                        const float hornL = 1.0f + 0.45f * depth * lfoSin;
                        const float hornR = 1.0f + 0.45f * depth * lfoCos;
                        wetL = std::tanh(inL * hornL * (1.0f + 0.25f * cSin));
                        wetR = std::tanh(inR * hornR * (1.0f + 0.25f * cCos));
                    }

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    if (numChannels > 1)
                        outR[i] = inR * (1.0f - mix) + wetR * mix;
                }
                break;
            }

            case FxModuleId::Bitcrusher: // Zenology Lo-Fi Pack (Bitcrusher, Sample Decimator, SP-1200, Radio)
            {
                if (!params.crushEnable || params.crushMix <= 0.005f)
                    break;

                const float mix = juce::jlimit(0.0f, 1.0f, params.crushMix);
                const float bits = juce::jlimit(1.0f, 16.0f, params.crushBits);
                const float levels = std::pow(2.0f, bits - 1.0f);
                const float dsFactor = juce::jlimit(1.0f, 64.0f, params.crushDownsample);
                const float holdStep = 1.0f / dsFactor;
                const float drive = 1.0f + params.crushDrive * 3.2f;
                const float toneCoeff = 0.06f + params.crushTone * 0.88f;

                for (int i = 0; i < numSamples; ++i)
                {
                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;

                    crushHoldPhase += holdStep;
                    if (crushHoldPhase >= 1.0f)
                    {
                        crushHoldPhase -= std::floor(crushHoldPhase);
                        const float preL = std::tanh(inL * drive);
                        const float preR = std::tanh(inR * drive);

                        if (params.crushMode == 0) // Digital Bitcrush + Sample Hold
                        {
                            crushHeldL = std::round(preL * levels) / levels;
                            crushHeldR = std::round(preR * levels) / levels;
                        }
                        else if (params.crushMode == 1) // Pure Sample-Rate Decimator + Aliasing Ring
                        {
                            const float fineLevels = std::max(8.0f, levels * 2.0f);
                            crushHeldL = std::round(preL * fineLevels) / fineLevels;
                            crushHeldR = std::round(preR * fineLevels) / fineLevels;
                        }
                        else if (params.crushMode == 2) // SP-1200 12-Bit Vintage Sampler Crunch
                        {
                            const float spLevels = std::min(2048.0f, std::max(4.0f, levels));
                            const float asymL = preL + 0.08f * preL * preL;
                            const float asymR = preR + 0.08f * preR * preR;
                            crushHeldL = std::round(asymL * spLevels) / spLevels;
                            crushHeldR = std::round(asymR * spLevels) / spLevels;
                        }
                        else // Radio Lo-Fi Band-Limited Crush
                        {
                            const float rLevels = std::max(3.0f, levels * 0.75f);
                            crushHeldL = std::round(preL * rLevels) / rLevels;
                            crushHeldR = std::round(preR * rLevels) / rLevels;
                        }
                    }

                    crushLpL += toneCoeff * (crushHeldL - crushLpL);
                    crushLpR += toneCoeff * (crushHeldR - crushLpR);

                    float wetL = crushLpL;
                    float wetR = crushLpR;
                    if (params.crushMode == 3) // Radio highpass mid emphasis
                    {
                        wetL = (crushHeldL - crushLpL * 0.45f) * 1.15f;
                        wetR = (crushHeldR - crushLpR * 0.45f) * 1.15f;
                    }

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    if (numChannels > 1)
                        outR[i] = inR * (1.0f - mix) + wetR * mix;
                }
                break;
            }

            case FxModuleId::Flanger: // Zenology Flanger Pack (Jet Flanger, Through-Zero, Barberpole, Step Flanger)
            {
                if (!params.flangerEnable || params.flangerMix <= 0.005f)
                    break;

                const float mix = juce::jlimit(0.0f, 1.0f, params.flangerMix);
                const float rateInc = (params.flangerRate * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float depth = juce::jlimit(0.0f, 1.0f, params.flangerDepth);
                const float fb = juce::jlimit(-0.86f, 0.86f, params.flangerFeedback);
                const float manual = juce::jlimit(0.0f, 1.0f, params.flangerManual);

                // Flanger delay range: 0.2ms to 8.5ms
                const float minDelaySamp = static_cast<float>(sampleRate) * (0.00025f + manual * 0.0025f);
                const float sweepSamp    = static_cast<float>(sampleRate) * (0.0004f + depth * 0.0055f);

                for (int i = 0; i < numSamples; ++i)
                {
                    const float inL = outL[i];
                    const float inR = (numChannels > 1) ? outR[i] : inL;

                    flangerPhase += rateInc;
                    if (flangerPhase > juce::MathConstants<float>::twoPi)
                        flangerPhase -= juce::MathConstants<float>::twoPi;

                    float lfoL = 0.5f + 0.5f * fastSin(flangerPhase);
                    float lfoR = 0.5f + 0.5f * fastCos(flangerPhase);

                    if (params.flangerMode == 2) // Barberpole Endless Upward Sweep
                    {
                        lfoL = flangerPhase / juce::MathConstants<float>::twoPi;
                        lfoR = std::fmod(lfoL + 0.5f, 1.0f);
                    }
                    else if (params.flangerMode == 3) // Step Flanger (8-Step Quantized LFO)
                    {
                        lfoL = std::round(lfoL * 8.0f) / 8.0f;
                        lfoR = std::round(lfoR * 8.0f) / 8.0f;
                    }

                    const float fbInL = inL + flangerFbL * fb;
                    const float fbInR = inR + flangerFbR * fb;
                    flangerBufL[static_cast<size_t>(flangerWritePos)] = std::tanh(fbInL * 0.75f) * 1.33f;
                    flangerBufR[static_cast<size_t>(flangerWritePos)] = std::tanh(fbInR * 0.75f) * 1.33f;

                    auto readFlangeTap = [&](const std::array<float, FLANGER_BUFFER_SIZE>& buf, float delaySamples) -> float
                    {
                        float rPos = static_cast<float>(flangerWritePos) - delaySamples;
                        while (rPos < 0.0f)
                            rPos += static_cast<float>(FLANGER_BUFFER_SIZE);
                        const int r0 = static_cast<int>(rPos) & (FLANGER_BUFFER_SIZE - 1);
                        const int r1 = (r0 + 1) & (FLANGER_BUFFER_SIZE - 1);
                        const float frac = rPos - std::floor(rPos);
                        return buf[static_cast<size_t>(r0)] * (1.0f - frac) + buf[static_cast<size_t>(r1)] * frac;
                    };

                    float tapL = readFlangeTap(flangerBufL, minDelaySamp + lfoL * sweepSamp);
                    float tapR = readFlangeTap(flangerBufR, minDelaySamp + lfoR * sweepSamp);
                    if (std::isnan(tapL) || std::isinf(tapL)) tapL = 0.0f;
                    if (std::isnan(tapR) || std::isinf(tapR)) tapR = 0.0f;
                    flangerFbL = tapL;
                    flangerFbR = tapR;

                    if (params.flangerMode == 1) // Through-Zero Flanger (Dry Delayed to Sweep Center)
                    {
                        const float centerDelay = minDelaySamp + 0.5f * sweepSamp;
                        const float dryTzL = readFlangeTap(flangerBufL, centerDelay);
                        const float dryTzR = readFlangeTap(flangerBufR, centerDelay);
                        outL[i] = inL * (1.0f - mix) + (dryTzL - tapL * 0.85f) * mix;
                        if (numChannels > 1)
                            outR[i] = inR * (1.0f - mix) + (dryTzR - tapR * 0.85f) * mix;
                    }
                    else
                    {
                        outL[i] = inL * (1.0f - mix * 0.5f) + tapL * mix * 0.78f;
                        if (numChannels > 1)
                            outR[i] = inR * (1.0f - mix * 0.5f) + tapR * mix * 0.78f;
                    }

                    flangerWritePos = (flangerWritePos + 1) & (FLANGER_BUFFER_SIZE - 1);
                }
                break;
            }

            case FxModuleId::Count:
            default:
                break;
        }
    }

    // -------------------------------------------------------------------------
    // Gross Beat / ShaperBox 3 Circular Buffer Time Modulation Engine
    // -------------------------------------------------------------------------
    void processTimeModulation(float* outL, float* outR, int numSamples, int numChannels, const SynthParameters& params)
    {
        if (timeModBufL.empty() || timeModBufR.empty())
            return;

        const float safeBpm = static_cast<float>(juce::jlimit(40.0, 260.0, params.hostBpm));
        const float beatsPerSec = safeBpm / 60.0f;
        const float beatSamples = static_cast<float>(sampleRate) / beatsPerSec;

        // Cycle rate in Hz from timeModRate (0: 2 Bars, 1: 1 Bar, 2: 1/2, 3: 1/4, 4: 1/8, 5: 1/16)
        float cycleHz = beatsPerSec * 0.25f;
        switch (juce::jlimit(0, 5, params.timeModRate))
        {
            case 0: cycleHz = beatsPerSec * 0.125f; break; // 2 Bars
            case 1: cycleHz = beatsPerSec * 0.25f;  break; // 1 Bar
            case 2: cycleHz = beatsPerSec * 0.5f;   break; // 1/2
            case 3: cycleHz = beatsPerSec * 1.0f;   break; // 1/4
            case 4: cycleHz = beatsPerSec * 2.0f;   break; // 1/8
            case 5: cycleHz = beatsPerSec * 4.0f;   break; // 1/16
            default: break;
        }

        const float phaseInc = cycleHz * invSampleRate;
        const float maxDelaySamples = std::min(static_cast<float>(TIME_MOD_BUFFER_SIZE - 64), beatSamples * 2.0f);
        const float depth = juce::jlimit(0.0f, 1.0f, params.timeModDepth);
        const float mix = juce::jlimit(0.0f, 1.0f, params.timeModMix);

        // Fast Inactive Bypass: If TimeMod is disabled or mix is 0, do fast buffer update and exit!
        if (!params.timeModEnable || mix <= 0.002f)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                timeModBufL[static_cast<size_t>(timeModWritePos)] = outL[i];
                timeModBufR[static_cast<size_t>(timeModWritePos)] = (numChannels > 1) ? outR[i] : outL[i];
                timeModWritePos = (timeModWritePos + 1) & (TIME_MOD_BUFFER_SIZE - 1);
            }
            timeModBarPhase = std::fmod(timeModBarPhase + phaseInc * static_cast<float>(numSamples), 1.0f);
            return;
        }

        const float smoothCoeff = 1.0f - juce::jlimit(0.0f, 0.992f, 0.70f + params.timeModSmooth * 0.292f);

        const float filtAmt = juce::jlimit(-1.0f, 1.0f, params.timeModFilter);
        const float lpCut = (filtAmt < 0.0f) ? (std::pow(1.0f + filtAmt, 2.0f) * 0.85f + 0.03f) : 0.98f;
        const float hpCut = (filtAmt > 0.0f) ? (filtAmt * 0.35f) : 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = outL[i];
            const float inR = (numChannels > 1) ? outR[i] : inL;

            // Always record dry signal into circular history buffer so enabling Time Mod is instant
            timeModBufL[static_cast<size_t>(timeModWritePos)] = inL;
            timeModBufR[static_cast<size_t>(timeModWritePos)] = inR;

            timeModBarPhase += phaseInc;
            if (timeModBarPhase >= 1.0f)
                timeModBarPhase -= std::floor(timeModBarPhase);

            if (params.timeModEnable && mix > 0.002f)
            {
                float targetOffsetNorm = 0.0f;
                float stutterGate = 1.0f;

                switch (params.timeModMode)
                {
                    case 0: // Time Warp Spline Curve (Gross Beat / ShaperBox 3 Curve)
                        targetOffsetNorm = timeModCurve.evaluateLUT(timeModBarPhase) * depth;
                        break;

                    case 1: // Stutter / Beat Repeat
                    {
                        const float subdiv = 2.0f + std::round(params.timeModStutter * 6.0f) * 2.0f;
                        const float slicePhase = std::fmod(timeModBarPhase * subdiv, 1.0f);
                        // Hold a repeating slice from the start of the bar/beat
                        const float repeatRamp = (timeModBarPhase - slicePhase / subdiv);
                        targetOffsetNorm = juce::jlimit(0.0f, 1.0f, repeatRamp * depth);
                        const float gateWidth = 1.0f - params.timeModStutter * 0.45f;
                        if (slicePhase > gateWidth)
                            stutterGate = std::max(0.0f, 1.0f - (slicePhase - gateWidth) * 12.0f);
                        break;
                    }

                    case 2: // Tape Stop / Vinyl Deceleration
                    {
                        const float curveExp = 1.6f + std::abs(params.timeModSpeed) * 2.4f;
                        targetOffsetNorm = std::pow(timeModBarPhase, curveExp) * depth;
                        stutterGate = 1.0f - 0.35f * targetOffsetNorm;
                        break;
                    }

                    case 3: // Reverse Buffer Grain
                    {
                        // Reading backward requires delay offset increasing at 2x real-time rate
                        const float revPhase = std::fmod(timeModBarPhase * 2.0f, 1.0f);
                        targetOffsetNorm = revPhase * depth;
                        // Window edges of reverse grain to prevent clicks
                        const float edgeWin = fastSin(revPhase * juce::MathConstants<float>::pi);
                        stutterGate = std::min(1.0f, edgeWin * 3.5f);
                        break;
                    }

                    case 4: // Half-Time (1/2x Gross Beat Speed)
                        targetOffsetNorm = (timeModBarPhase * 0.5f) * depth;
                        break;

                    case 5: // DJ Scratch / Turntable Scrub
                    default:
                    {
                        const float scratch = 0.5f - 0.5f * fastCos(timeModBarPhase * juce::MathConstants<float>::twoPi * 2.0f);
                        targetOffsetNorm = scratch * depth * 0.65f;
                        break;
                    }
                }

                const float desiredDelaySamples = targetOffsetNorm * maxDelaySamples;
                timeModPrevOffsetSamples += smoothCoeff * (desiredDelaySamples - timeModPrevOffsetSamples);

                float readPos = static_cast<float>(timeModWritePos) - timeModPrevOffsetSamples;
                while (readPos < 0.0f)
                    readPos += static_cast<float>(TIME_MOD_BUFFER_SIZE);

                const int r0 = static_cast<int>(readPos) & (TIME_MOD_BUFFER_SIZE - 1);
                const int r1 = (r0 + 1) & (TIME_MOD_BUFFER_SIZE - 1);
                const float frac = readPos - std::floor(readPos);

                const float tapL = (timeModBufL[static_cast<size_t>(r0)] * (1.0f - frac)
                                  + timeModBufL[static_cast<size_t>(r1)] * frac) * stutterGate;
                const float tapR = (timeModBufR[static_cast<size_t>(r0)] * (1.0f - frac)
                                  + timeModBufR[static_cast<size_t>(r1)] * frac) * stutterGate;

                timeModLpL += lpCut * (tapL - timeModLpL);
                timeModLpR += lpCut * (tapR - timeModLpR);
                timeModHpL += hpCut * (timeModLpL - timeModHpL);
                timeModHpR += hpCut * (timeModLpR - timeModHpR);

                const float wetL = (filtAmt > 0.0f) ? (timeModLpL - timeModHpL) : timeModLpL;
                const float wetR = (filtAmt > 0.0f) ? (timeModLpR - timeModHpR) : timeModLpR;

                outL[i] = inL * (1.0f - mix) + wetL * mix;
                if (numChannels > 1)
                    outR[i] = inR * (1.0f - mix) + wetR * mix;

                pendingSnapshot.timeModOffsetNorm = targetOffsetNorm;
            }
            else
            {
                pendingSnapshot.timeModOffsetNorm = timeModCurve.evaluateLUT(timeModBarPhase) * depth;
            }

            timeModWritePos = (timeModWritePos + 1) & (TIME_MOD_BUFFER_SIZE - 1);
        }

        pendingSnapshot.timeModCursor = timeModBarPhase;
        const int previewStep = std::max(1, static_cast<int>(maxDelaySamples) / 64);
        for (size_t b = 0; b < 64; ++b)
        {
            const int idx = (timeModWritePos - static_cast<int>((63 - b) * static_cast<size_t>(previewStep)) + TIME_MOD_BUFFER_SIZE) & (TIME_MOD_BUFFER_SIZE - 1);
            pendingSnapshot.timeModWavePreview[b] = juce::jlimit(-1.0f, 1.0f, timeModBufL[static_cast<size_t>(idx)]);
        }
    }

    double sampleRate = 44100.0;
    float  invSampleRate = 1.0f / 44100.0f;
    float  lastPlayedFreq = 130.81f;
    float  latestVoiceEnvGain = 0.0f;

    // -------------------------------------------------------------------------
    // Unison Parameter Smoothers (10ms-20ms ramp time, zero zipper noise)
    // -------------------------------------------------------------------------
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDetune;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedSpread;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBlend;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedUnisonVoices;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedPolyHeadroom;

    // Arpeggiator & Step Sequencer State
    mutable juce::CriticalSection arpLock;
    std::array<ArpStep, NUM_ARP_STEPS> arpSteps {};
    std::vector<HeldNote> arpHeldNotes;
    std::vector<int> activeArpTriggeredNotes;
    int   arpStepIndex = 0;
    int   arpNoteIndex = 0;
    int   arpDirection = 1;
    float arpPhase = 0.0f;
    bool  arpWasEnabled = false;
    std::atomic<int> currentArpStepAtomic { 0 };

    // Noise Generator State
    float  pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f;
    float  brownStateL = 0.0f, brownStateR = 0.0f;
    float  bitHoldPhase = 0.0f, bitHoldSampleL = 0.0f, bitHoldSampleR = 0.0f;
    float  airBandL = 0.0f, airLowL = 0.0f, airBandR = 0.0f, airLowR = 0.0f;
    float  noiseLpStateL = 0.0f, noiseLpStateR = 0.0f;
    float  noiseHpStateL = 0.0f, noiseHpStateR = 0.0f;

    // Dynamic FX Rack State
    mutable juce::CriticalSection fxOrderLock;
    std::vector<int> activeFxChain;
    std::array<int, MAX_FX_CHAIN_LEN> activeFxChainArray {};
    std::atomic<int> activeFxChainCount { 0 };

    float  phaserLfoPhase = 0.0f;
    float  tremMasterPhase = 0.0f;
    float  distToneStateL = 0.0f, distToneStateR = 0.0f;
    std::array<float, 6> phaserApStateL {};
    std::array<float, 6> phaserApStateR {};

    float  chorusPhase = 0.0f;
    int    chorusWritePos = 0;
    std::array<float, CHORUS_BUFFER_SIZE> chorusBufL {};
    std::array<float, CHORUS_BUFFER_SIZE> chorusBufR {};

    int    delayWritePos = 0;
    float  delayLpStateL = 0.0f, delayLpStateR = 0.0f;
    std::array<float, MAX_DELAY_SAMPLES> delayBufL {};
    std::array<float, MAX_DELAY_SAMPLES> delayBufR {};

    int    revPrePos = 0, revPos0 = 0, revPos1 = 0, revPos2 = 0, revPos3 = 0, revApPos0 = 0, revApPos1 = 0;
    float  revLpL0 = 0.0f, revLpR0 = 0.0f, revLpL1 = 0.0f, revLpR1 = 0.0f;
    float  revLpL2 = 0.0f, revLpR2 = 0.0f, revLpL3 = 0.0f, revLpR3 = 0.0f;
    std::array<float, 4096> revPreBufL {}, revPreBufR {};
    std::array<float, 2048> revCombBufL0 {}, revCombBufR0 {};
    std::array<float, 2048> revCombBufL1 {}, revCombBufR1 {};
    std::array<float, 2048> revCombBufL2 {}, revCombBufR2 {};
    std::array<float, 2048> revCombBufL3 {}, revCombBufR3 {};
    std::array<float, 512>  revApBufL0 {}, revApBufR0 {};
    std::array<float, 256>  revApBufL1 {}, revApBufR1 {};

    // Compressor State
    float  compEnvSingle = 0.01f;
    std::array<float, 3> compEnvBand { 0.01f, 0.01f, 0.01f };
    float  compLpLowL = 0.0f, compLpLowR = 0.0f;
    float  compLpMidL = 0.0f, compLpMidR = 0.0f;

    // Zenology FX Pack States (Ring Mod, Bitcrusher, Flanger)
    float  ringCarrierPhase = 0.0f;
    float  ringLfoPhase = 0.0f;
    float  crushHoldPhase = 0.0f;
    float  crushHeldL = 0.0f, crushHeldR = 0.0f;
    float  crushLpL = 0.0f, crushLpR = 0.0f;
    float  flangerPhase = 0.0f;
    int    flangerWritePos = 0;
    float  flangerFbL = 0.0f, flangerFbR = 0.0f;
    std::array<float, FLANGER_BUFFER_SIZE> flangerBufL {};
    std::array<float, FLANGER_BUFFER_SIZE> flangerBufR {};

    // Time Modulation Engine State (Gross Beat / ShaperBox 3 Circular Buffer)
    std::vector<float> timeModBufL;
    std::vector<float> timeModBufR;
    int    timeModWritePos = 0;
    float  timeModBarPhase = 0.0f;
    float  timeModTapePhase = 0.0f;
    float  timeModPrevOffsetSamples = 0.0f;
    float  timeModLpL = 0.0f, timeModLpR = 0.0f;
    float  timeModHpL = 0.0f, timeModHpR = 0.0f;
    SplineCurve timeModCurve;

    float  limiterEnv = 1.0f;
    std::vector<HeldNote> heldNotes;
    juce::Random rng;

    std::array<float, SINE_LUT_SIZE> sineLUT {};
    std::array<Voice, MAX_VOICES> voices {};
    std::array<SplineCurve, static_cast<size_t>(ArticulatorTarget::Count)> articulators {};
    ResynthesisEngine resynthEngine;

    VisualizerSnapshot pendingSnapshot {};
    std::array<VisualizerSnapshot, 2> visSnapshots {};
    std::atomic<int> activeVisIndex { 0 };
    std::atomic<int> activeVoicesAtomic { 0 };
};

} // namespace PrismorDSP
