#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/FactoryPresetBank.h"

PrismorAudioProcessor::PrismorAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PRISMOR_PARAMS", createParameterLayout())
{
    initCachedParamPointers();
    loadFactoryPreset(0); // Initialize to 00. INIT // Clean Default Saw
}

PrismorAudioProcessor::~PrismorAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout PrismorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto normRange = []() { return juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f); };
    auto bipolarRange = []() { return juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f); };

    const juce::StringArray warpModes { "Off", "Bend +/-", "Mirror", "Quantize", "Asymmetric", "Sync Fold" };

    // -------------------------------------------------------------------------
    // 1. Frequency Field 1 (Part A Oscillator / Pitch / Warp / Scale / Tilt)
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterInt>("oscAOct",          "Field 1 Octave",   -4, 4, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("oscASemi",         "Field 1 Semitone", -12, 12, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("oscAFine",       "Field 1 Fine Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("oscACoarse",     "Field 1 Coarse Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("startPhase",     "Field 1 Phase",   normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaseRandom",    "Phase Random",    normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timbre",         "Field 1 Timbre",  normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("warpMode",      "Field 1 Warp Mode", warpModes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("warpAmount",     "Field 1 Warp Amt", bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmonicScale",  "Field 1 Harm Scale",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.6f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmonicTilt",   "Field 1 Tilt",    bipolarRange(), 0.0f));

    // -------------------------------------------------------------------------
    // 2. Frequency Field 2 (Part B Independent Oscillator / Pitch / Warp / Scale)
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterInt>("oscBOct",          "Field 2 Octave",   -4, 4, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("oscBSemi",         "Field 2 Semitone", -12, 12, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("oscBFine",       "Field 2 Fine Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("oscBPitch",      "Field 2 Coarse Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("oscBPhase",      "Field 2 Phase",   normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timbreB",        "Field 2 Timbre",  normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("warpModeB",     "Field 2 Warp Mode", warpModes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("warpAmountB",    "Field 2 Warp Amt", bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmonicScaleB", "Field 2 Harm Scale",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.6f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmonicTiltB",  "Field 2 Tilt",    bipolarRange(), 0.0f));

    // -------------------------------------------------------------------------
    // 3. Prism, Harmonizer & Pluck
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("prismEnable",     "Prism Enable",    true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("prismAmount",    "Prism Amount",    normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("prismMode",     "Prism Mode",
        juce::StringArray{ "Inharmonic", "Hz Shift", "Odd/Even", "Quantum" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmShift",      "Harmonizer Shift", normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmGap",        "Harmonizer Gap",   normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("harmWidth",      "Harmonizer Width", normRange(), 0.45f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("pluckEnable",     "Pluck Enable",     true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pluckTime",      "Pluck Time",       normRange(), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pluckBlur",      "Pluck Blur",       normRange(), 0.30f));

    // -------------------------------------------------------------------------
    // 4. Voicing, True Legato, Glide & Serum-2 Style 16x Unison Engine
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterChoice>("voicingMode",   "Voicing Mode",
        juce::StringArray{ "Polyphonic", "Monophonic", "True Legato" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("maxVoices",        "Voice Limit",     1, 16, 8));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchFreq",      "Pitch Shift",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("portaTime",      "Portamento Time",  normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("portaCurve",     "Portamento Curve", bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("autoGlide",       "Auto Glide",       true));

    params.push_back(std::make_unique<juce::AudioParameterInt>("unisonVoices",     "Unison Voices",    1, 16, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("unisonDetune",   "Unison Detune",    normRange(), 0.18f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("unisonSpread",   "Unison Spread",    normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("unisonBlend",    "Unison Blend",     normRange(), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("unisonPhase",    "Unison Phase",     normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("unisonStackMode","Unison Stack Mode",
        juce::StringArray{ "Normal 1x", "+1 Octave", "+2 Octaves", "Sub + Oct", "Fifths", "Oct + 5th", "Minor Triad", "Major Triad" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("vibDepth",       "Vibrato Depth",    normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vibSpeed",       "Vibrato Speed",
        juce::NormalisableRange<float>(0.5f, 15.0f, 0.01f, 0.5f), 5.5f));

    // -------------------------------------------------------------------------
    // 5. Dual Spectral Filter & Drive (10 Topologies incl. Vox, Vocoder, Talkbox)
    // -------------------------------------------------------------------------
    const juce::StringArray filterTypes {
        "SVF LP 12dB",
        "SVF LP 24dB",
        "Crude Low Pass",
        "SVF HP 24dB",
        "Bandpass SVF",
        "Comb Filter",
        "Vox Formant (A-E-I-O-U)",
        "Custom Shape",
        "Vocoder Band-Bank",
        "Talkbox Mode"
    };

    params.push_back(std::make_unique<juce::AudioParameterBool>("filter1Enable",    "Filter 1 Enable",   true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filter1Type",    "Filter 1 Type",     filterTypes, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter1Cutoff",   "Filter 1 Cutoff",   normRange(), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter1Res",      "Filter 1 Res",      normRange(), 0.10f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter1KeyTrack", "Filter 1 KeyTrack", normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter1Width",    "Filter 1 Width",    normRange(), 0.45f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("filter2Enable",    "Filter 2 Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filter2Type",    "Filter 2 Type",     filterTypes, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter2Cutoff",   "Filter 2 Cutoff",   normRange(), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter2Res",      "Filter 2 Res",      normRange(), 0.20f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter2KeyTrack", "Filter 2 KeyTrack", normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter2Width",    "Filter 2 Width",    normRange(), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterRouting",   "Filter Routing",    normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterDrive",     "Filter Drive",      normRange(), 0.0f));

    // -------------------------------------------------------------------------
    // 6. Dedicated Serum-2 Style Noise Engine
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("noiseEnable",      "Noise Enable",      false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("noiseType",      "Noise Type",
        juce::StringArray{ "White Noise", "Pink Noise", "Brown Noise", "Vinyl Crackle", "Digital Bitnoise", "Breath / Air" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseLevel",      "Noise Level",       normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noisePitch",      "Noise Pitch",       normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseKeyTrack",   "Noise KeyTrack",    normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noisePhase",      "Noise Phase",       normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noisePan",        "Noise Pan",         bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseFilter",     "Noise LP/HP Filter", bipolarRange(), 0.0f));

    // -------------------------------------------------------------------------
    // 7. Built-in Arpeggiator & Step Sequencer (Serum 2 / u-he Hive 2 Style)
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpEnable",        "Arp Enable",        false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpMode",        "Arp Mode",
        juce::StringArray{ "Up", "Down", "Up/Down", "Random", "Chord", "Step Seq" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpOctaves",        "Arp Octaves",       1, 4, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpRate",        "Arp Rate",
        juce::StringArray{ "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arpGate",         "Arp Gate",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.01f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arpSwing",        "Arp Swing",
        juce::NormalisableRange<float>(0.0f, 0.75f, 0.01f), 0.0f));

    // -------------------------------------------------------------------------
    // 8. Dedicated FX Tab: 9 Dynamic Modular Multi-FX Units
    // -------------------------------------------------------------------------
    // FX 0: Spectral Blur
    params.push_back(std::make_unique<juce::AudioParameterBool>("blurEnable",       "Blur Enable",       true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("blurTime",        "Blur Time",         normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("blurHarmonic",    "Harmonic Blur",     normRange(), 0.0f));

    // FX 1: Harmonic Tremolo
    params.push_back(std::make_unique<juce::AudioParameterBool>("tremEnable",       "Tremolo Enable",    true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tremDepth",       "Tremolo Depth",     normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tremSpeed",       "Tremolo Speed",
        juce::NormalisableRange<float>(0.1f, 15.0f, 0.01f, 0.5f), 3.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tremGap",         "Tremolo Gap",       normRange(), 0.35f));

    // FX 2: Distortion / Saturation
    params.push_back(std::make_unique<juce::AudioParameterBool>("distEnable",       "Distortion Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("distType",       "Distortion Type",
        juce::StringArray{ "Tube", "Tape", "Hard Clip", "Wavefold", "Bitcrush Fuzz", "Asym Scream" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("distDrive",       "Distortion Drive",  normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("distTone",        "Distortion Tone",   normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("distMix",         "Distortion Mix",    normRange(), 0.50f));

    // FX 3: Spectral Phaser
    params.push_back(std::make_unique<juce::AudioParameterBool>("phaserEnable",     "Phaser Enable",     true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserMix",       "Phaser Mix",        normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserWidth",     "Phaser Width",      normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserOffset",    "Phaser Offset",     normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserSpeed",     "Phaser Speed",
        juce::NormalisableRange<float>(0.05f, 10.0f, 0.01f, 0.5f), 1.2f));

    // FX 4: Multi-Voice Chorus
    params.push_back(std::make_unique<juce::AudioParameterBool>("chorusEnable",     "Chorus Enable",     false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("chorusVoices",      "Chorus Voices",     2, 6, 4));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("chorusRate",      "Chorus Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("chorusDepth",     "Chorus Depth",      normRange(), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("chorusSpread",    "Chorus Spread",     normRange(), 0.80f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("chorusMix",       "Chorus Mix",        normRange(), 0.35f));

    // FX 5: 3-Band Parametric EQ
    params.push_back(std::make_unique<juce::AudioParameterBool>("eqEnable",         "EQ Enable",         true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqLowFreq",       "EQ Low Freq",
        juce::NormalisableRange<float>(30.0f, 400.0f, 1.0f, 0.5f), 110.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqLowGain",       "EQ Low Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqMidFreq",       "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 6000.0f, 1.0f, 0.45f), 1200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqMidGain",       "EQ Mid Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqMidQ",          "EQ Mid Q",
        juce::NormalisableRange<float>(0.3f, 8.0f, 0.05f, 0.5f), 1.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqHighFreq",      "EQ High Freq",
        juce::NormalisableRange<float>(1500.0f, 16000.0f, 10.0f, 0.5f), 5500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eqHighGain",      "EQ High Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f));

    // FX 6: Tempo-Sync Stereo / Ping-Pong Delay
    params.push_back(std::make_unique<juce::AudioParameterBool>("delayEnable",      "Delay Enable",      false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime",       "Delay Time/Sync",   normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback",   "Delay Feedback",    normRange(), 0.40f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayPingPong",   "Delay Ping-Pong",   normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFilter",     "Delay Filter",      normRange(), 0.60f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayMix",        "Delay Mix",         normRange(), 0.28f));

    // FX 7: Hall / Shimmer Reverb
    params.push_back(std::make_unique<juce::AudioParameterBool>("reverbEnable",     "Reverb Enable",     false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize",      "Reverb Size",       normRange(), 0.68f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDecay",     "Reverb Decay",      normRange(), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbPreDelay",  "Reverb Pre-Delay",  normRange(), 0.18f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDamping",   "Reverb Damping",    normRange(), 0.40f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbShimmer",   "Reverb Shimmer",    normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbMix",       "Reverb Mix",        normRange(), 0.25f));

    // FX 8: Single-Band & Multiband OTT Compressor
    params.push_back(std::make_unique<juce::AudioParameterBool>("compEnable",       "Compressor Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("compMode",       "Compressor Mode",
        juce::StringArray{ "Single-Band VCA", "Multiband OTT", "Opto Smooth", "Punch Limiter" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("compThreshold",   "Comp Threshold",
        juce::NormalisableRange<float>(-48.0f, 0.0f, 0.1f), -18.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("compRatio",       "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("compAttack",      "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.45f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("compRelease",     "Comp Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.45f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("compMakeup",      "Comp Makeup dB",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("compMix",         "Comp Mix",          normRange(), 0.75f));

    // FX 9: Zenology Modulation Pack (Ring Modulator / Freq Shifter / Auto-Pan / Rotary)
    params.push_back(std::make_unique<juce::AudioParameterBool>("ringEnable",       "Ring Mod Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ringMode",       "Ring Mod Mode",
        juce::StringArray{ "Ring Mod", "Freq Shifter", "Stereo Auto-Pan", "Rotary Speaker" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ringFreq",        "Ring Carrier Hz",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.45f), 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ringDepth",       "Ring Mod Depth",    normRange(), 0.70f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ringLfoRate",     "Ring LFO Rate",
        juce::NormalisableRange<float>(0.05f, 15.0f, 0.01f, 0.5f), 1.20f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ringLfoDepth",    "Ring LFO Depth",    normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ringMix",         "Ring Mod Mix",      normRange(), 0.50f));

    // FX 10: Zenology Lo-Fi Pack (Bitcrusher / Sample Decimator / Vintage SP-1200)
    params.push_back(std::make_unique<juce::AudioParameterBool>("crushEnable",      "Bitcrush Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("crushMode",      "Bitcrush Mode",
        juce::StringArray{ "Digital Crush", "Sample Decimate", "SP-1200 Vintage", "Radio Lo-Fi" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crushBits",       "Crush Bit Depth",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.1f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crushDownsample", "Crush Downsample",
        juce::NormalisableRange<float>(1.0f, 64.0f, 0.1f, 0.45f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crushDrive",      "Crush Drive",       normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crushTone",       "Crush Tone",        normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crushMix",        "Crush Mix",         normRange(), 0.55f));

    // FX 11: Zenology Flanger Pack (Jet Flanger / Through-Zero / Barberpole / Step Flanger)
    params.push_back(std::make_unique<juce::AudioParameterBool>("flangerEnable",    "Flanger Enable",    false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("flangerMode",    "Flanger Mode",
        juce::StringArray{ "Jet Flanger", "Through-Zero", "Barberpole", "Step Flanger" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flangerRate",     "Flanger Rate",
        juce::NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.5f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flangerDepth",    "Flanger Depth",     normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flangerFeedback", "Flanger Feedback",
        juce::NormalisableRange<float>(-0.95f, 0.95f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flangerManual",   "Flanger Manual",    normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("flangerMix",      "Flanger Mix",       normRange(), 0.50f));

    // Bode Frequency Shifter (Linear Hilbert SSB Frequency Shifter)
    params.push_back(std::make_unique<juce::AudioParameterBool>("bodeEnable",        "Bode Enable",       false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("bodeShift",        "Bode Shift Hz",
        juce::NormalisableRange<float>(-5000.0f, 5000.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("bodeFeedback",     "Bode Feedback",
        juce::NormalisableRange<float>(-0.88f, 0.88f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("bodeMix",          "Bode Mix",          normRange(), 0.50f));

    // -------------------------------------------------------------------------
    // 8.5. Time Modulation Engine (Gross Beat / ShaperBox 3 Buffer Time-Shaping)
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>("timeModEnable",    "Time Mod Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("timeModMode",    "Time Mod Mode",
        juce::StringArray{ "Time Warp Curve", "Stutter Repeat", "Tape Stop", "Reverse Grain", "Half-Time 1/2x", "DJ Scratch" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("timeModRate",    "Time Mod Rate",
        juce::StringArray{ "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timeModDepth",    "Time Warp Depth",   normRange(), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timeModStutter",  "Stutter Density",   normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timeModSpeed",    "Tape Speed Slope",  bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timeModSmooth",   "De-Click Smooth",   normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timeModFilter",   "Time DJ Filter",    bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("timeModMix",      "Time Mod Mix",      normRange(), 0.85f));

    // -------------------------------------------------------------------------
    // 8.8. Serum-2 Style Physical AHDSR Envelope Knobs (Graphical Articulator)
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envAttack",       "Env Attack",        normRange(), 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envHold",         "Env Hold",          normRange(), 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envDecay",        "Env Decay",         normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envSustain",      "Env Sustain",       normRange(), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envRelease",      "Env Release",       normRange(), 0.25f));

    // -------------------------------------------------------------------------
    // 9. A/B System, Strum & Master Output / Limiter (Bottom Panel)
    // -------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>("abMix",           "A/B Part Mix",      normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("velLink",         "Velocity Link",     normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("partPan",         "Part Pan",          bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("strumTime",       "Strum Time",        normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("strumTension",    "Strum Slope",       bipolarRange(), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterGainDb",    "Master Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("limiterEnable",    "Limiter Enable",    true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("limiterThreshDb", "Limiter Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.5f));

    return { params.begin(), params.end() };
}

void PrismorAudioProcessor::initCachedParamPointers()
{
    auto getRaw = [this](const char* id) -> std::atomic<float>* {
        return apvts.getRawParameterValue(id);
    };

    cachedParams.oscAOct         = getRaw("oscAOct");
    cachedParams.oscASemi        = getRaw("oscASemi");
    cachedParams.oscAFine        = getRaw("oscAFine");
    cachedParams.oscACoarse      = getRaw("oscACoarse");
    cachedParams.startPhase      = getRaw("startPhase");
    cachedParams.phaseRandom     = getRaw("phaseRandom");
    cachedParams.timbre          = getRaw("timbre");
    cachedParams.warpMode        = getRaw("warpMode");
    cachedParams.warpAmount      = getRaw("warpAmount");
    cachedParams.harmonicScale   = getRaw("harmonicScale");
    cachedParams.harmonicTilt    = getRaw("harmonicTilt");

    cachedParams.oscBOct         = getRaw("oscBOct");
    cachedParams.oscBSemi        = getRaw("oscBSemi");
    cachedParams.oscBFine        = getRaw("oscBFine");
    cachedParams.oscBPitch       = getRaw("oscBPitch");
    cachedParams.oscBPhase       = getRaw("oscBPhase");
    cachedParams.timbreB         = getRaw("timbreB");
    cachedParams.warpModeB       = getRaw("warpModeB");
    cachedParams.warpAmountB     = getRaw("warpAmountB");
    cachedParams.harmonicScaleB  = getRaw("harmonicScaleB");
    cachedParams.harmonicTiltB   = getRaw("harmonicTiltB");

    cachedParams.prismEnable     = getRaw("prismEnable");
    cachedParams.prismAmount     = getRaw("prismAmount");
    cachedParams.prismMode       = getRaw("prismMode");
    cachedParams.harmShift       = getRaw("harmShift");
    cachedParams.harmGap         = getRaw("harmGap");
    cachedParams.harmWidth       = getRaw("harmWidth");

    cachedParams.pluckEnable     = getRaw("pluckEnable");
    cachedParams.pluckTime       = getRaw("pluckTime");
    cachedParams.pluckBlur       = getRaw("pluckBlur");

    cachedParams.voicingMode     = getRaw("voicingMode");
    cachedParams.maxVoices       = getRaw("maxVoices");
    cachedParams.pitchFreq       = getRaw("pitchFreq");
    cachedParams.portaTime       = getRaw("portaTime");
    cachedParams.portaCurve      = getRaw("portaCurve");
    cachedParams.autoGlide       = getRaw("autoGlide");

    cachedParams.unisonVoices    = getRaw("unisonVoices");
    cachedParams.unisonDetune    = getRaw("unisonDetune");
    cachedParams.unisonSpread    = getRaw("unisonSpread");
    cachedParams.unisonBlend     = getRaw("unisonBlend");
    cachedParams.unisonPhase     = getRaw("unisonPhase");
    cachedParams.unisonStackMode = getRaw("unisonStackMode");

    cachedParams.vibDepth        = getRaw("vibDepth");
    cachedParams.vibSpeed        = getRaw("vibSpeed");

    cachedParams.filter1Enable   = getRaw("filter1Enable");
    cachedParams.filter1Type     = getRaw("filter1Type");
    cachedParams.filter1Cutoff   = getRaw("filter1Cutoff");
    cachedParams.filter1Res      = getRaw("filter1Res");
    cachedParams.filter1KeyTrack = getRaw("filter1KeyTrack");
    cachedParams.filter1Width    = getRaw("filter1Width");

    cachedParams.filter2Enable   = getRaw("filter2Enable");
    cachedParams.filter2Type     = getRaw("filter2Type");
    cachedParams.filter2Cutoff   = getRaw("filter2Cutoff");
    cachedParams.filter2Res      = getRaw("filter2Res");
    cachedParams.filter2KeyTrack = getRaw("filter2KeyTrack");
    cachedParams.filter2Width    = getRaw("filter2Width");

    cachedParams.filterRouting   = getRaw("filterRouting");
    cachedParams.filterDrive     = getRaw("filterDrive");

    cachedParams.noiseEnable     = getRaw("noiseEnable");
    cachedParams.noiseType       = getRaw("noiseType");
    cachedParams.noiseLevel      = getRaw("noiseLevel");
    cachedParams.noisePitch      = getRaw("noisePitch");
    cachedParams.noiseKeyTrack   = getRaw("noiseKeyTrack");
    cachedParams.noisePhase      = getRaw("noisePhase");
    cachedParams.noisePan        = getRaw("noisePan");
    cachedParams.noiseFilter     = getRaw("noiseFilter");

    cachedParams.arpEnable       = getRaw("arpEnable");
    cachedParams.arpMode         = getRaw("arpMode");
    cachedParams.arpOctaves      = getRaw("arpOctaves");
    cachedParams.arpRate         = getRaw("arpRate");
    cachedParams.arpGate         = getRaw("arpGate");
    cachedParams.arpSwing        = getRaw("arpSwing");

    cachedParams.blurEnable      = getRaw("blurEnable");
    cachedParams.blurTime        = getRaw("blurTime");
    cachedParams.blurHarmonic    = getRaw("blurHarmonic");

    cachedParams.tremEnable      = getRaw("tremEnable");
    cachedParams.tremDepth       = getRaw("tremDepth");
    cachedParams.tremSpeed       = getRaw("tremSpeed");
    cachedParams.tremGap         = getRaw("tremGap");

    cachedParams.distEnable      = getRaw("distEnable");
    cachedParams.distType        = getRaw("distType");
    cachedParams.distDrive       = getRaw("distDrive");
    cachedParams.distTone        = getRaw("distTone");
    cachedParams.distMix         = getRaw("distMix");

    cachedParams.phaserEnable    = getRaw("phaserEnable");
    cachedParams.phaserMix       = getRaw("phaserMix");
    cachedParams.phaserWidth     = getRaw("phaserWidth");
    cachedParams.phaserOffset    = getRaw("phaserOffset");
    cachedParams.phaserSpeed     = getRaw("phaserSpeed");

    cachedParams.chorusEnable    = getRaw("chorusEnable");
    cachedParams.chorusVoices    = getRaw("chorusVoices");
    cachedParams.chorusRate      = getRaw("chorusRate");
    cachedParams.chorusDepth     = getRaw("chorusDepth");
    cachedParams.chorusSpread    = getRaw("chorusSpread");
    cachedParams.chorusMix       = getRaw("chorusMix");

    cachedParams.eqEnable        = getRaw("eqEnable");
    cachedParams.eqLowFreq       = getRaw("eqLowFreq");
    cachedParams.eqLowGain       = getRaw("eqLowGain");
    cachedParams.eqMidFreq       = getRaw("eqMidFreq");
    cachedParams.eqMidGain       = getRaw("eqMidGain");
    cachedParams.eqMidQ          = getRaw("eqMidQ");
    cachedParams.eqHighFreq      = getRaw("eqHighFreq");
    cachedParams.eqHighGain      = getRaw("eqHighGain");

    cachedParams.delayEnable     = getRaw("delayEnable");
    cachedParams.delayTime       = getRaw("delayTime");
    cachedParams.delayFeedback   = getRaw("delayFeedback");
    cachedParams.delayPingPong   = getRaw("delayPingPong");
    cachedParams.delayFilter     = getRaw("delayFilter");
    cachedParams.delayMix        = getRaw("delayMix");

    cachedParams.reverbEnable    = getRaw("reverbEnable");
    cachedParams.reverbSize      = getRaw("reverbSize");
    cachedParams.reverbDecay     = getRaw("reverbDecay");
    cachedParams.reverbPreDelay  = getRaw("reverbPreDelay");
    cachedParams.reverbDamping   = getRaw("reverbDamping");
    cachedParams.reverbShimmer   = getRaw("reverbShimmer");
    cachedParams.reverbMix       = getRaw("reverbMix");

    cachedParams.compEnable      = getRaw("compEnable");
    cachedParams.compMode        = getRaw("compMode");
    cachedParams.compThreshold   = getRaw("compThreshold");
    cachedParams.compRatio       = getRaw("compRatio");
    cachedParams.compAttack      = getRaw("compAttack");
    cachedParams.compRelease     = getRaw("compRelease");
    cachedParams.compMakeup      = getRaw("compMakeup");
    cachedParams.compMix         = getRaw("compMix");

    cachedParams.ringEnable      = getRaw("ringEnable");
    cachedParams.ringMode        = getRaw("ringMode");
    cachedParams.ringFreq        = getRaw("ringFreq");
    cachedParams.ringDepth       = getRaw("ringDepth");
    cachedParams.ringLfoRate     = getRaw("ringLfoRate");
    cachedParams.ringLfoDepth    = getRaw("ringLfoDepth");
    cachedParams.ringMix         = getRaw("ringMix");

    cachedParams.crushEnable     = getRaw("crushEnable");
    cachedParams.crushMode       = getRaw("crushMode");
    cachedParams.crushBits       = getRaw("crushBits");
    cachedParams.crushDownsample = getRaw("crushDownsample");
    cachedParams.crushDrive      = getRaw("crushDrive");
    cachedParams.crushTone       = getRaw("crushTone");
    cachedParams.crushMix        = getRaw("crushMix");

    cachedParams.flangerEnable   = getRaw("flangerEnable");
    cachedParams.flangerMode     = getRaw("flangerMode");
    cachedParams.flangerRate     = getRaw("flangerRate");
    cachedParams.flangerDepth    = getRaw("flangerDepth");
    cachedParams.flangerFeedback = getRaw("flangerFeedback");
    cachedParams.flangerManual   = getRaw("flangerManual");
    cachedParams.flangerMix      = getRaw("flangerMix");

    cachedParams.bodeEnable      = getRaw("bodeEnable");
    cachedParams.bodeShift       = getRaw("bodeShift");
    cachedParams.bodeFeedback    = getRaw("bodeFeedback");
    cachedParams.bodeMix         = getRaw("bodeMix");

    cachedParams.timeModEnable   = getRaw("timeModEnable");
    cachedParams.timeModMode     = getRaw("timeModMode");
    cachedParams.timeModRate     = getRaw("timeModRate");
    cachedParams.timeModDepth    = getRaw("timeModDepth");
    cachedParams.timeModStutter  = getRaw("timeModStutter");
    cachedParams.timeModSpeed    = getRaw("timeModSpeed");
    cachedParams.timeModSmooth   = getRaw("timeModSmooth");
    cachedParams.timeModFilter   = getRaw("timeModFilter");
    cachedParams.timeModMix      = getRaw("timeModMix");

    cachedParams.envAttack       = getRaw("envAttack");
    cachedParams.envHold         = getRaw("envHold");
    cachedParams.envDecay        = getRaw("envDecay");
    cachedParams.envSustain      = getRaw("envSustain");
    cachedParams.envRelease      = getRaw("envRelease");

    cachedParams.abMix           = getRaw("abMix");
    cachedParams.velLink         = getRaw("velLink");
    cachedParams.partPan         = getRaw("partPan");
    cachedParams.strumTime       = getRaw("strumTime");
    cachedParams.strumTension    = getRaw("strumTension");

    cachedParams.masterGainDb    = getRaw("masterGainDb");
    cachedParams.limiterEnable   = getRaw("limiterEnable");
    cachedParams.limiterThreshDb = getRaw("limiterThreshDb");
}

PrismorDSP::SynthParameters PrismorAudioProcessor::readParametersFromAPVTS() const noexcept
{
    PrismorDSP::SynthParameters p;

    auto getF = [](const std::atomic<float>* ptr) noexcept -> float {
        return ptr ? ptr->load(std::memory_order_relaxed) : 0.0f;
    };

    // Field 1 (Part A)
    p.oscAOct         = static_cast<int>(std::round(getF(cachedParams.oscAOct)));
    p.oscASemi        = static_cast<int>(std::round(getF(cachedParams.oscASemi)));
    p.oscAFine        = getF(cachedParams.oscAFine);
    p.oscACoarse      = getF(cachedParams.oscACoarse);
    p.startPhase      = getF(cachedParams.startPhase);
    p.phaseRandom     = getF(cachedParams.phaseRandom);
    p.timbre          = getF(cachedParams.timbre);
    p.warpMode        = static_cast<int>(getF(cachedParams.warpMode));
    p.warpAmount      = getF(cachedParams.warpAmount);
    p.harmonicScale   = getF(cachedParams.harmonicScale);
    p.harmonicTilt    = getF(cachedParams.harmonicTilt);

    // Field 2 (Part B)
    p.oscBOct         = static_cast<int>(std::round(getF(cachedParams.oscBOct)));
    p.oscBSemi        = static_cast<int>(std::round(getF(cachedParams.oscBSemi)));
    p.oscBFine        = getF(cachedParams.oscBFine);
    p.oscBPitch       = getF(cachedParams.oscBPitch);
    p.oscBPhase       = getF(cachedParams.oscBPhase);
    p.timbreB         = getF(cachedParams.timbreB);
    p.warpModeB       = static_cast<int>(getF(cachedParams.warpModeB));
    p.warpAmountB     = getF(cachedParams.warpAmountB);
    p.harmonicScaleB  = getF(cachedParams.harmonicScaleB);
    p.harmonicTiltB   = getF(cachedParams.harmonicTiltB);

    // Prism, Harmonizer & Pluck
    p.prismEnable     = (getF(cachedParams.prismEnable) > 0.5f);
    p.prismAmount     = getF(cachedParams.prismAmount);
    p.prismMode       = static_cast<int>(getF(cachedParams.prismMode));
    p.harmShift       = getF(cachedParams.harmShift);
    p.harmGap         = getF(cachedParams.harmGap);
    p.harmWidth       = getF(cachedParams.harmWidth);

    p.pluckEnable     = (getF(cachedParams.pluckEnable) > 0.5f);
    p.pluckTime       = getF(cachedParams.pluckTime);
    p.pluckBlur       = getF(cachedParams.pluckBlur);

    // Voicing, Legato & Serum-2 Unison
    p.voicingMode     = static_cast<int>(getF(cachedParams.voicingMode));
    p.maxVoices       = static_cast<int>(getF(cachedParams.maxVoices));
    p.pitchFreq       = getF(cachedParams.pitchFreq);
    p.portaTime       = getF(cachedParams.portaTime);
    p.portaCurve      = getF(cachedParams.portaCurve);
    p.autoGlide       = (getF(cachedParams.autoGlide) > 0.5f);

    p.unisonVoices    = static_cast<int>(getF(cachedParams.unisonVoices));
    p.unisonDetune    = getF(cachedParams.unisonDetune);
    p.unisonSpread    = getF(cachedParams.unisonSpread);
    p.unisonBlend     = getF(cachedParams.unisonBlend);
    p.unisonPhase     = getF(cachedParams.unisonPhase);
    p.unisonStackMode = static_cast<int>(getF(cachedParams.unisonStackMode));

    p.vibDepth        = getF(cachedParams.vibDepth);
    p.vibSpeed        = getF(cachedParams.vibSpeed);

    // Dual Spectral Filters & Drive
    p.filter1Enable   = (getF(cachedParams.filter1Enable) > 0.5f);
    p.filter1Type     = static_cast<int>(getF(cachedParams.filter1Type));
    p.filter1Cutoff   = getF(cachedParams.filter1Cutoff);
    p.filter1Res      = getF(cachedParams.filter1Res);
    p.filter1KeyTrack = getF(cachedParams.filter1KeyTrack);
    p.filter1Width    = getF(cachedParams.filter1Width);

    p.filter2Enable   = (getF(cachedParams.filter2Enable) > 0.5f);
    p.filter2Type     = static_cast<int>(getF(cachedParams.filter2Type));
    p.filter2Cutoff   = getF(cachedParams.filter2Cutoff);
    p.filter2Res      = getF(cachedParams.filter2Res);
    p.filter2KeyTrack = getF(cachedParams.filter2KeyTrack);
    p.filter2Width    = getF(cachedParams.filter2Width);

    p.filterRouting   = getF(cachedParams.filterRouting);
    p.filterDrive     = getF(cachedParams.filterDrive);

    // Noise Engine
    p.noiseEnable     = (getF(cachedParams.noiseEnable) > 0.5f);
    p.noiseType       = static_cast<int>(getF(cachedParams.noiseType));
    p.noiseLevel      = getF(cachedParams.noiseLevel);
    p.noisePitch      = getF(cachedParams.noisePitch);
    p.noiseKeyTrack   = getF(cachedParams.noiseKeyTrack);
    p.noisePhase      = getF(cachedParams.noisePhase);
    p.noisePan        = getF(cachedParams.noisePan);
    p.noiseFilter     = getF(cachedParams.noiseFilter);

    // Arpeggiator & Step Sequencer
    p.arpEnable       = (getF(cachedParams.arpEnable) > 0.5f);
    p.arpMode         = static_cast<int>(getF(cachedParams.arpMode));
    p.arpOctaves      = static_cast<int>(getF(cachedParams.arpOctaves));
    p.arpRate         = static_cast<int>(getF(cachedParams.arpRate));
    p.arpGate         = getF(cachedParams.arpGate);
    p.arpSwing        = getF(cachedParams.arpSwing);

    // 12 Modular FX Units
    p.blurEnable      = (getF(cachedParams.blurEnable) > 0.5f);
    p.blurTime        = getF(cachedParams.blurTime);
    p.blurHarmonic    = getF(cachedParams.blurHarmonic);

    p.tremEnable      = (getF(cachedParams.tremEnable) > 0.5f);
    p.tremDepth       = getF(cachedParams.tremDepth);
    p.tremSpeed       = getF(cachedParams.tremSpeed);
    p.tremGap         = getF(cachedParams.tremGap);

    p.distEnable      = (getF(cachedParams.distEnable) > 0.5f);
    p.distType        = static_cast<int>(getF(cachedParams.distType));
    p.distDrive       = getF(cachedParams.distDrive);
    p.distTone        = getF(cachedParams.distTone);
    p.distMix         = getF(cachedParams.distMix);

    p.phaserEnable    = (getF(cachedParams.phaserEnable) > 0.5f);
    p.phaserMix       = getF(cachedParams.phaserMix);
    p.phaserWidth     = getF(cachedParams.phaserWidth);
    p.phaserOffset    = getF(cachedParams.phaserOffset);
    p.phaserSpeed     = getF(cachedParams.phaserSpeed);

    p.chorusEnable    = (getF(cachedParams.chorusEnable) > 0.5f);
    p.chorusVoices    = static_cast<int>(getF(cachedParams.chorusVoices));
    p.chorusRate      = getF(cachedParams.chorusRate);
    p.chorusDepth     = getF(cachedParams.chorusDepth);
    p.chorusSpread    = getF(cachedParams.chorusSpread);
    p.chorusMix       = getF(cachedParams.chorusMix);

    p.eqEnable        = (getF(cachedParams.eqEnable) > 0.5f);
    p.eqLowFreq       = getF(cachedParams.eqLowFreq);
    p.eqLowGain       = getF(cachedParams.eqLowGain);
    p.eqMidFreq       = getF(cachedParams.eqMidFreq);
    p.eqMidGain       = getF(cachedParams.eqMidGain);
    p.eqMidQ          = getF(cachedParams.eqMidQ);
    p.eqHighFreq      = getF(cachedParams.eqHighFreq);
    p.eqHighGain      = getF(cachedParams.eqHighGain);

    p.delayEnable     = (getF(cachedParams.delayEnable) > 0.5f);
    p.delayTime       = getF(cachedParams.delayTime);
    p.delayFeedback   = getF(cachedParams.delayFeedback);
    p.delayPingPong   = getF(cachedParams.delayPingPong);
    p.delayFilter     = getF(cachedParams.delayFilter);
    p.delayMix        = getF(cachedParams.delayMix);

    p.reverbEnable    = (getF(cachedParams.reverbEnable) > 0.5f);
    p.reverbSize      = getF(cachedParams.reverbSize);
    p.reverbDecay     = getF(cachedParams.reverbDecay);
    p.reverbPreDelay  = getF(cachedParams.reverbPreDelay);
    p.reverbDamping   = getF(cachedParams.reverbDamping);
    p.reverbShimmer   = getF(cachedParams.reverbShimmer);
    p.reverbMix       = getF(cachedParams.reverbMix);

    p.compEnable      = (getF(cachedParams.compEnable) > 0.5f);
    p.compMode        = static_cast<int>(getF(cachedParams.compMode));
    p.compThreshold   = getF(cachedParams.compThreshold);
    p.compRatio       = getF(cachedParams.compRatio);
    p.compAttack      = getF(cachedParams.compAttack);
    p.compRelease     = getF(cachedParams.compRelease);
    p.compMakeup      = getF(cachedParams.compMakeup);
    p.compMix         = getF(cachedParams.compMix);

    p.ringEnable      = (getF(cachedParams.ringEnable) > 0.5f);
    p.ringMode        = static_cast<int>(getF(cachedParams.ringMode));
    p.ringFreq        = getF(cachedParams.ringFreq);
    p.ringDepth       = getF(cachedParams.ringDepth);
    p.ringLfoRate     = getF(cachedParams.ringLfoRate);
    p.ringLfoDepth    = getF(cachedParams.ringLfoDepth);
    p.ringMix         = getF(cachedParams.ringMix);

    p.crushEnable     = (getF(cachedParams.crushEnable) > 0.5f);
    p.crushMode       = static_cast<int>(getF(cachedParams.crushMode));
    p.crushBits       = getF(cachedParams.crushBits);
    p.crushDownsample = getF(cachedParams.crushDownsample);
    p.crushDrive      = getF(cachedParams.crushDrive);
    p.crushTone       = getF(cachedParams.crushTone);
    p.crushMix        = getF(cachedParams.crushMix);

    p.flangerEnable   = (getF(cachedParams.flangerEnable) > 0.5f);
    p.flangerMode     = static_cast<int>(getF(cachedParams.flangerMode));
    p.flangerRate     = getF(cachedParams.flangerRate);
    p.flangerDepth    = getF(cachedParams.flangerDepth);
    p.flangerFeedback = getF(cachedParams.flangerFeedback);
    p.flangerManual   = getF(cachedParams.flangerManual);
    p.flangerMix      = getF(cachedParams.flangerMix);

    // Bode Frequency Shifter
    p.bodeEnable      = (getF(cachedParams.bodeEnable) > 0.5f);
    p.bodeShift       = getF(cachedParams.bodeShift);
    p.bodeFeedback    = getF(cachedParams.bodeFeedback);
    p.bodeMix         = getF(cachedParams.bodeMix);

    // Time Modulation Engine
    p.timeModEnable   = (getF(cachedParams.timeModEnable) > 0.5f);
    p.timeModMode     = static_cast<int>(getF(cachedParams.timeModMode));
    p.timeModRate     = static_cast<int>(getF(cachedParams.timeModRate));
    p.timeModDepth    = getF(cachedParams.timeModDepth);
    p.timeModStutter  = getF(cachedParams.timeModStutter);
    p.timeModSpeed    = getF(cachedParams.timeModSpeed);
    p.timeModSmooth   = getF(cachedParams.timeModSmooth);
    p.timeModFilter   = getF(cachedParams.timeModFilter);
    p.timeModMix      = getF(cachedParams.timeModMix);

    // AHDSR Envelope Knobs
    p.envAttack       = getF(cachedParams.envAttack);
    p.envHold         = getF(cachedParams.envHold);
    p.envDecay        = getF(cachedParams.envDecay);
    p.envSustain      = getF(cachedParams.envSustain);
    p.envRelease      = getF(cachedParams.envRelease);

    // Master & A/B
    p.abMix           = getF(cachedParams.abMix);
    p.velLink         = getF(cachedParams.velLink);
    p.partPan         = getF(cachedParams.partPan);
    p.strumTime       = getF(cachedParams.strumTime);
    p.strumTension    = getF(cachedParams.strumTension);

    p.masterGainDb    = getF(cachedParams.masterGainDb);
    p.limiterEnable   = (getF(cachedParams.limiterEnable) > 0.5f);
    p.limiterThreshDb = getF(cachedParams.limiterThreshDb);

    return p;
}

void PrismorAudioProcessor::loadFilterSubPreset(int subPresetIndex)
{
    auto setParam = [this](const char* id, float plainVal)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(plainVal));
    };

    switch (subPresetIndex)
    {
        case 1: // Talkbox Wow Lead
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    9.0f); // Talkbox Mode
            setParam("filter1Cutoff",  0.52f);
            setParam("filter1Res",     0.62f);
            setParam("filter1Width",   0.48f);
            setParam("filter2Enable",  1.0f);
            setParam("filter2Type",    6.0f); // Vox Formant
            setParam("filter2Cutoff",  0.40f);
            setParam("filter2Res",     0.45f);
            setParam("filterRouting",  0.50f);
            setParam("filterDrive",    0.42f);
            break;

        case 2: // Reese Sub Lowpass
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    2.0f); // Crude Low Pass
            setParam("filter1Cutoff",  0.38f);
            setParam("filter1Res",     0.58f);
            setParam("filter1Width",   0.32f);
            setParam("filter2Enable",  0.0f);
            setParam("filterDrive",    0.65f);
            break;

        case 3: // Vocal Choir Formant (A-E-I-O-U)
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    6.0f); // Vox Formant
            setParam("filter1Cutoff",  0.48f);
            setParam("filter1Res",     0.68f);
            setParam("filter1Width",   0.52f);
            setParam("filter2Enable",  1.0f);
            setParam("filter2Type",    0.0f); // SVF LP 12dB
            setParam("filter2Cutoff",  0.74f);
            setParam("filterRouting",  0.0f);
            setParam("filterDrive",    0.20f);
            break;

        case 4: // Robotic Vocoder Bank
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    8.0f); // Vocoder Band-Bank
            setParam("filter1Cutoff",  0.56f);
            setParam("filter1Res",     0.78f);
            setParam("filter1Width",   0.28f);
            setParam("filter2Enable",  1.0f);
            setParam("filter2Type",    9.0f); // Talkbox Mode
            setParam("filter2Cutoff",  0.62f);
            setParam("filter2Res",     0.45f);
            setParam("filterRouting",  0.45f);
            setParam("filterDrive",    0.35f);
            break;

        case 5: // Metallic Comb Flange
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    5.0f); // Comb Filter
            setParam("filter1Cutoff",  0.64f);
            setParam("filter1Res",     0.74f);
            setParam("filter1Width",   0.25f);
            setParam("filter2Enable",  0.0f);
            setParam("filterDrive",    0.38f);
            break;

        case 6: // Screaming Acid 24dB
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    1.0f); // SVF LP 24dB
            setParam("filter1Cutoff",  0.42f);
            setParam("filter1Res",     0.85f);
            setParam("filter1Width",   0.22f);
            setParam("filter2Enable",  0.0f);
            setParam("filterDrive",    0.78f);
            break;

        case 7: // Dual Bandpass Notch
            setParam("filter1Enable",  1.0f);
            setParam("filter1Type",    4.0f); // Bandpass SVF
            setParam("filter1Cutoff",  0.38f);
            setParam("filter1Res",     0.65f);
            setParam("filter2Enable",  1.0f);
            setParam("filter2Type",    4.0f); // Bandpass SVF
            setParam("filter2Cutoff",  0.72f);
            setParam("filter2Res",     0.65f);
            setParam("filterRouting",  1.0f); // Parallel
            setParam("filterDrive",    0.45f);
            break;

        default:
            break;
    }
}

void PrismorAudioProcessor::loadDistortionSubPreset(int subPresetIndex)
{
    auto setParam = [this](const char* id, float plainVal)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(plainVal));
    };

    switch (subPresetIndex)
    {
        case 1: // Warm Tube Saturation
            setParam("distEnable", 1.0f);
            setParam("distType",   0.0f); // Tube
            setParam("distDrive",  0.38f);
            setParam("distTone",   0.62f);
            setParam("distMix",    0.52f);
            break;

        case 2: // Screaming Acid Drive
            setParam("distEnable", 1.0f);
            setParam("distType",   5.0f); // Asym Scream
            setParam("distDrive",  0.74f);
            setParam("distTone",   0.78f);
            setParam("distMix",    0.65f);
            break;

        case 3: // Bitcrush Fuzz
            setParam("distEnable", 1.0f);
            setParam("distType",   4.0f); // Bitcrush Fuzz
            setParam("distDrive",  0.68f);
            setParam("distTone",   0.58f);
            setParam("distMix",    0.55f);
            break;

        case 4: // Heavy Wavefold
            setParam("distEnable", 1.0f);
            setParam("distType",   3.0f); // Wavefold
            setParam("distDrive",  0.76f);
            setParam("distTone",   0.68f);
            setParam("distMix",    0.60f);
            break;

        case 5: // Tape Master Glue
            setParam("distEnable", 1.0f);
            setParam("distType",   1.0f); // Tape
            setParam("distDrive",  0.28f);
            setParam("distTone",   0.54f);
            setParam("distMix",    0.45f);
            break;

        case 6: // Brutal Neuro Clipper
            setParam("distEnable", 1.0f);
            setParam("distType",   2.0f); // Hard Clip
            setParam("distDrive",  0.85f);
            setParam("distTone",   0.72f);
            setParam("distMix",    0.70f);
            break;

        default:
            break;
    }
}

juce::StringArray PrismorAudioProcessor::getFilterSubPresetNames() const
{
    return {
        "Talkbox Wow Lead",
        "Reese Sub Lowpass",
        "Vocal Choir (A-E-I-O-U)",
        "Robotic Vocoder Bank",
        "Metallic Comb Flange",
        "Screaming Acid 24dB",
        "Dual Bandpass Notch"
    };
}

juce::StringArray PrismorAudioProcessor::getDistortionSubPresetNames() const
{
    return {
        "Warm Tube Saturation",
        "Screaming Acid Drive",
        "Bitcrush Fuzz",
        "Heavy Wavefold",
        "Tape Master Glue",
        "Brutal Neuro Clipper"
    };
}

juce::File PrismorAudioProcessor::getUserPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CyberSpectralAudio")
                   .getChildFile("Prismor")
                   .getChildFile("UserPresets");
    if (!dir.exists())
        dir.createDirectory();
    return dir;
}

bool PrismorAudioProcessor::exportPresetToFile(const juce::File& file, const juce::String& name, const juce::String& category, const juce::String& author, const juce::String& description)
{
    auto state = apvts.copyState();
    state.setProperty("presetName", name, nullptr);
    state.setProperty("category", category, nullptr);
    state.setProperty("author", author, nullptr);
    state.setProperty("description", description, nullptr);

    juce::ValueTree artTree("Articulators");
    for (int i = 0; i < static_cast<int>(PrismorDSP::ArticulatorTarget::Count); ++i)
    {
        const auto target = static_cast<PrismorDSP::ArticulatorTarget>(i);
        artTree.appendChild(synthEngine.getArticulator(target).toValueTree("Curve_" + juce::String(i)), nullptr);
    }
    artTree.appendChild(synthEngine.getTimeModCurve().toValueTree("TimeModCurve"), nullptr);
    state.appendChild(artTree, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
        return xml->writeTo(file);
    return false;
}

bool PrismorAudioProcessor::saveUserPreset(const juce::String& name, const juce::String& category, const juce::String& author, const juce::String& description)
{
    auto userDir = getUserPresetDirectory();
    auto catDir = userDir.getChildFile(category.isEmpty() ? "USER" : category);
    if (!catDir.exists())
        catDir.createDirectory();

    auto file = catDir.getChildFile(name + ".prismor");
    return exportPresetToFile(file, name, category, author, description);
}

bool PrismorAudioProcessor::loadPresetFromXml(const juce::XmlElement& xmlState)
{
    if (xmlState.hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(xmlState);
        auto artTree = tree.getChildWithName("Articulators");
        if (artTree.isValid())
        {
            for (int i = 0; i < static_cast<int>(PrismorDSP::ArticulatorTarget::Count); ++i)
            {
                auto c = artTree.getChildWithName("Curve_" + juce::String(i));
                if (c.isValid())
                    synthEngine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(i)).fromValueTree(c);
            }
            auto tm = artTree.getChildWithName("TimeModCurve");
            if (tm.isValid())
                synthEngine.getTimeModCurve().fromValueTree(tm);
            tree.removeChild(artTree, nullptr);
        }
        apvts.replaceState(tree);
        return true;
    }
    return false;
}

bool PrismorAudioProcessor::importPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr && loadPresetFromXml(*xml))
    {
        auto userDir = getUserPresetDirectory();
        auto dest = userDir.getChildFile(file.getFileName());
        if (dest != file)
            file.copyFileTo(dest);
        return true;
    }
    return false;
}

std::vector<PrismorAudioProcessor::FactoryPresetMetadata> PrismorAudioProcessor::getFactoryPresetBank() const
{
    const auto& rawEntries = PrismorDSP::FactoryPresetBank::getEntries();
    std::vector<FactoryPresetMetadata> meta;
    meta.reserve(rawEntries.size() + 32);

    for (const auto& e : rawEntries)
    {
        meta.push_back({
            e.index,
            juce::String(e.name),
            juce::String(e.category),
            juce::String(e.author),
            juce::String(e.wavesUsed),
            juce::String(e.fxUsed),
            juce::String(e.description),
            false,
            juce::File()
        });
    }

    // Scan User Presets Folder
    auto userDir = getUserPresetDirectory();
    if (userDir.isDirectory())
    {
        auto userFiles = userDir.findChildFiles(juce::File::findFiles, true, "*.prismor;*.xml");
        int uIdx = static_cast<int>(meta.size());
        for (const auto& f : userFiles)
        {
            auto xml = juce::XmlDocument::parse(f);
            juce::String pName = f.getFileNameWithoutExtension();
            juce::String pCat = "USER";
            juce::String pAuthor = "User";
            juce::String pDesc = "User created preset";
            juce::String pWaves = "CUSTOM";
            juce::String pFx = "USER FX";

            if (xml != nullptr)
            {
                if (xml->hasAttribute("presetName")) pName = xml->getStringAttribute("presetName");
                if (xml->hasAttribute("category")) pCat = xml->getStringAttribute("category");
                if (xml->hasAttribute("author")) pAuthor = xml->getStringAttribute("author");
                if (xml->hasAttribute("description")) pDesc = xml->getStringAttribute("description");
            }

            meta.push_back({
                uIdx++,
                pName,
                pCat,
                pAuthor,
                pWaves,
                pFx,
                pDesc,
                true,
                f
            });
        }
    }

    return meta;
}

juce::StringArray PrismorAudioProcessor::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (const auto& p : getFactoryPresetBank())
        names.add(p.name);
    return names;
}

void PrismorAudioProcessor::loadFactoryPreset(int index)
{
    const auto bank = getFactoryPresetBank();
    if (bank.empty()) return;
    const int clamped = juce::jlimit(0, static_cast<int>(bank.size()) - 1, index);
    currentPreset = clamped;

    const auto& item = bank[static_cast<size_t>(clamped)];
    if (item.isUserPreset && item.userFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(item.userFile);
        if (xml != nullptr)
            loadPresetFromXml(*xml);
        return;
    }

    auto setParam = [this](const char* id, float plainVal)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(plainVal));
    };

    PrismorDSP::FactoryPresetBank::applyPreset(clamped, setParam, synthEngine);
}

const juce::String PrismorAudioProcessor::getName() const { return JucePlugin_Name; }
bool PrismorAudioProcessor::acceptsMidi() const { return true; }
bool PrismorAudioProcessor::producesMidi() const { return false; }
bool PrismorAudioProcessor::isMidiEffect() const { return false; }
double PrismorAudioProcessor::getTailLengthSeconds() const { return 2.0; }

int PrismorAudioProcessor::getNumPrograms() { return PrismorDSP::FactoryPresetBank::getPresetCount(); }
int PrismorAudioProcessor::getCurrentProgram() { return currentPreset; }
void PrismorAudioProcessor::setCurrentProgram(int index) { loadFactoryPreset(index); }
const juce::String PrismorAudioProcessor::getProgramName(int index)
{
    const auto names = getFactoryPresetNames();
    return names[juce::jlimit(0, names.size() - 1, index)];
}
void PrismorAudioProcessor::changeProgramName(int, const juce::String&) {}

void PrismorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    loadMeasurer.reset(sampleRate, samplesPerBlock);
    synthEngine.prepare(sampleRate, samplesPerBlock);
    keyboardState.reset();
}

void PrismorAudioProcessor::releaseResources()
{
    synthEngine.resetAllVoices();
}

bool PrismorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void PrismorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    juce::AudioProcessLoadMeasurer::ScopedTimer timer(loadMeasurer, numSamples);

    juce::ScopedNoDenormals noDenormals;

    // 1. Mandatory Buffer Initialization:
    // Explicitly clear all channels of the incoming audio buffer before ANY processing.
    // Prevents uninitialized host memory or leftover DAW audio from causing bitcrush-like noise.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);

    keyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true);

    auto params = readParametersFromAPVTS();
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm())
                params.hostBpm = *bpm;
        }
    }
    synthEngine.renderBlock(buffer, midiMessages, params);

    // -------------------------------------------------------------------------
    // Master Safety Stage: Fast Zero-Latency Soft Clipper / Safety Limiter
    // Smoothly catches and saturates any summed peaks exceeding 0 dBFS before the host
    // Eliminates all harsh digital hard-clipping and bitcrushed distortion
    // -------------------------------------------------------------------------
    const int numChannels = buffer.getNumChannels();
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
                // Smooth C1-continuous analog saturation curve: transparent below 0.90 (-0.9 dBFS)
                // Smoothly saturates to asymptotic 1.0 ceiling without digital clipping
                const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
                channelData[i] = sign * (0.90f + 0.10f * std::tanh((absX - 0.90f) * 10.0f));
            }
            channelData[i] = juce::jlimit(-1.0f, 1.0f, channelData[i]);
        }
    }
}

bool PrismorAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PrismorAudioProcessor::createEditor()
{
    return new PrismorAudioProcessorEditor(*this);
}

void PrismorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::ValueTree artTree("Articulators");
    for (int i = 0; i < static_cast<int>(PrismorDSP::ArticulatorTarget::Count); ++i)
    {
        const auto target = static_cast<PrismorDSP::ArticulatorTarget>(i);
        artTree.appendChild(synthEngine.getArticulator(target).toValueTree("Curve_" + juce::String(i)), nullptr);
    }
    artTree.appendChild(synthEngine.getTimeModCurve().toValueTree("TimeModCurve"), nullptr);
    state.appendChild(artTree, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PrismorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xmlState);
        auto artTree = tree.getChildWithName("Articulators");
        if (artTree.isValid())
        {
            for (int i = 0; i < static_cast<int>(PrismorDSP::ArticulatorTarget::Count); ++i)
            {
                auto c = artTree.getChildWithName("Curve_" + juce::String(i));
                if (c.isValid())
                    synthEngine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(i)).fromValueTree(c);
            }
            auto tm = artTree.getChildWithName("TimeModCurve");
            if (tm.isValid())
                synthEngine.getTimeModCurve().fromValueTree(tm);
            tree.removeChild(artTree, nullptr);
        }
        apvts.replaceState(tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PrismorAudioProcessor();
}
