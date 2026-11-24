#include "PluginProcessor.h"
#include "WebViewEditor.h"
#include "DSP/FactoryPresetBank.h"

namespace
{
inline juce::ParameterID pid(const char* name)
{
    return juce::ParameterID { name, 1 };
}

void snapDiscreteParameters(juce::AudioProcessor& processor)
{
    for (auto* baseParam : processor.getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(baseParam))
        {
            if (ranged->isDiscrete() || ranged->isBoolean())
            {
                const float rawNorm = baseParam->getValue();
                const float snappedNorm = ranged->convertTo0to1(ranged->convertFrom0to1(rawNorm));
                if (! juce::approximatelyEqual(rawNorm, snappedNorm))
                    baseParam->setValueNotifyingHost(snappedNorm);
            }
        }
    }
}
} // namespace

PrismorAudioProcessor::PrismorAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PRISMOR_PARAMS", createParameterLayout())
{
    initCachedParamPointers();
    stereoScratchBuffer.setSize(2, kMaxProcChunk, false, true, true);
    loadFactoryPreset(0);
}

PrismorAudioProcessor::~PrismorAudioProcessor() = default;

const juce::StringArray& PrismorAudioProcessor::getAllParameterIds()
{
    static const juce::StringArray ids {
        "oscAOct", "oscASemi", "oscAFine", "oscACoarse", "startPhase", "phaseRandom", "timbre", "warpMode", "warpAmount", "harmonicScale", "harmonicTilt",
        "oscBOct", "oscBSemi", "oscBFine", "oscBPitch", "oscBPhase", "timbreB", "warpModeB", "warpAmountB", "harmonicScaleB", "harmonicTiltB",
        "prismEnable", "prismAmount", "prismMode", "harmShift", "harmGap", "harmWidth",
        "pluckEnable", "pluckTime", "pluckBlur",
        "voicingMode", "maxVoices", "pitchFreq", "portaTime", "portaCurve", "autoGlide",
        "unisonVoices", "unisonDetune", "unisonSpread", "unisonBlend", "unisonPhase", "unisonStackMode", "vibDepth", "vibSpeed",
        "filter1Enable", "filter1Type", "filter1Cutoff", "filter1Res", "filter1KeyTrack", "filter1Width",
        "filter2Enable", "filter2Type", "filter2Cutoff", "filter2Res", "filter2KeyTrack", "filter2Width",
        "filterRouting", "filterDrive",
        "noiseEnable", "noiseType", "noiseLevel", "noisePitch", "noiseKeyTrack", "noisePhase", "noisePan", "noiseFilter",
        "arpEnable", "arpMode", "arpOctaves", "arpRate", "arpGate", "arpSwing",
        "blurEnable", "blurTime", "blurHarmonic",
        "tremEnable", "tremDepth", "tremSpeed", "tremGap",
        "distEnable", "distType", "distDrive", "distTone", "distMix",
        "phaserEnable", "phaserMix", "phaserWidth", "phaserOffset", "phaserSpeed",
        "chorusEnable", "chorusVoices", "chorusRate", "chorusDepth", "chorusSpread", "chorusMix",
        "eqEnable", "eqLowFreq", "eqLowGain", "eqMidFreq", "eqMidGain", "eqMidQ", "eqHighFreq", "eqHighGain",
        "delayEnable", "delayTime", "delayFeedback", "delayPingPong", "delayFilter", "delayMix",
        "reverbEnable", "reverbSize", "reverbDecay", "reverbPreDelay", "reverbDamping", "reverbShimmer", "reverbMix",
        "compEnable", "compMode", "compThreshold", "compRatio", "compAttack", "compRelease", "compMakeup", "compMix",
        "ringEnable", "ringMode", "ringFreq", "ringDepth", "ringLfoRate", "ringLfoDepth", "ringMix",
        "crushEnable", "crushMode", "crushBits", "crushDownsample", "crushDrive", "crushTone", "crushMix",
        "flangerEnable", "flangerMode", "flangerRate", "flangerDepth", "flangerFeedback", "flangerManual", "flangerMix",
        "bodeEnable", "bodeShift", "bodeFeedback", "bodeMix",
        "timeModEnable", "timeModMode", "timeModRate", "timeModDepth", "timeModStutter", "timeModSpeed", "timeModSmooth", "timeModFilter", "timeModMix",
        "envAttack", "envHold", "envDecay", "envSustain", "envRelease",
        "abMix", "velLink", "partPan", "strumTime", "strumTension",
        "masterGainDb", "limiterEnable", "limiterThreshDb"
    };
    return ids;
}

juce::AudioProcessorValueTreeState::ParameterLayout PrismorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto normRange    = []() { return juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f); };
    auto bipolarRange = []() { return juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f); };
    const juce::StringArray warpModes { "Off", "Bend +/-", "Mirror", "Quantize", "Asymmetric", "Sync Fold" };

    // 1. Frequency Field 1 (Part A)
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("oscAOct"),         "Field 1 Octave",     -4, 4, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("oscASemi"),        "Field 1 Semitone",   -12, 12, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("oscAFine"),      "Field 1 Fine Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("oscACoarse"),    "Field 1 Coarse Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("startPhase"),    "Field 1 Phase",      normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("phaseRandom"),   "Phase Random",       normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timbre"),        "Field 1 Timbre",     normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("warpMode"),     "Field 1 Warp Mode",  warpModes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("warpAmount"),    "Field 1 Warp Amt",   bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmonicScale"), "Field 1 Harm Scale",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.6f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmonicTilt"),  "Field 1 Tilt",       bipolarRange(), 0.0f));

    // 2. Frequency Field 2 (Part B)
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("oscBOct"),         "Field 2 Octave",     -4, 4, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("oscBSemi"),        "Field 2 Semitone",   -12, 12, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("oscBFine"),      "Field 2 Fine Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("oscBPitch"),     "Field 2 Coarse Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("oscBPhase"),     "Field 2 Phase",      normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timbreB"),       "Field 2 Timbre",     normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("warpModeB"),    "Field 2 Warp Mode",  warpModes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("warpAmountB"),   "Field 2 Warp Amt",   bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmonicScaleB"),"Field 2 Harm Scale",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.6f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmonicTiltB"), "Field 2 Tilt",       bipolarRange(), 0.0f));

    // 3. Prism, Harmonizer & Pluck
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("prismEnable"),    "Prism Enable",       true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("prismAmount"),   "Prism Amount",       normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("prismMode"),    "Prism Mode",
        juce::StringArray{ "Inharmonic", "Hz Shift", "Odd/Even", "Quantum" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmShift"),     "Harmonizer Shift",   normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmGap"),       "Harmonizer Gap",     normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("harmWidth"),     "Harmonizer Width",   normRange(), 0.45f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("pluckEnable"),    "Pluck Enable",       true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("pluckTime"),     "Pluck Time",         normRange(), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("pluckBlur"),     "Pluck Blur",         normRange(), 0.30f));

    // 4. Voicing & Unison
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("voicingMode"),  "Voicing Mode",
        juce::StringArray{ "Polyphonic", "Monophonic", "True Legato" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("maxVoices"),       "Voice Limit",        1, 16, 8));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("pitchFreq"),     "Pitch Shift",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("portaTime"),     "Portamento Time",    normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("portaCurve"),    "Portamento Curve",   bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("autoGlide"),      "Auto Glide",         true));

    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("unisonVoices"),    "Unison Voices",      1, 16, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("unisonDetune"),  "Unison Detune",      normRange(), 0.18f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("unisonSpread"),  "Unison Spread",      normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("unisonBlend"),   "Unison Blend",       normRange(), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("unisonPhase"),   "Unison Phase",       normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("unisonStackMode"), "Unison Stack Mode",
        juce::StringArray{ "Normal 1x", "+1 Octave", "+2 Octaves", "Sub + Oct", "Fifths", "Oct + 5th", "Minor Triad", "Major Triad" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("vibDepth"),      "Vibrato Depth",      normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("vibSpeed"),      "Vibrato Speed",
        juce::NormalisableRange<float>(0.5f, 15.0f, 0.01f, 0.5f), 5.5f));

    // 5. Dual Spectral Filter & Drive
    const juce::StringArray filterTypes {
        "SVF LP 12dB", "SVF LP 24dB", "Crude Low Pass", "SVF HP 24dB", "Bandpass SVF",
        "Comb Filter", "Vox Formant (A-E-I-O-U)", "Custom Shape", "Vocoder Band-Bank", "Talkbox Mode"
    };

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("filter1Enable"),   "Filter 1 Enable",   true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("filter1Type"),   "Filter 1 Type",     filterTypes, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter1Cutoff"),  "Filter 1 Cutoff",   normRange(), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter1Res"),     "Filter 1 Res",      normRange(), 0.10f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter1KeyTrack"),"Filter 1 KeyTrack", normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter1Width"),   "Filter 1 Width",    normRange(), 0.45f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("filter2Enable"),   "Filter 2 Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("filter2Type"),   "Filter 2 Type",     filterTypes, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter2Cutoff"),  "Filter 2 Cutoff",   normRange(), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter2Res"),     "Filter 2 Res",      normRange(), 0.20f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter2KeyTrack"),"Filter 2 KeyTrack", normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filter2Width"),   "Filter 2 Width",    normRange(), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterRouting"),  "Filter Routing",    normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("filterDrive"),    "Filter Drive",      normRange(), 0.0f));

    // 6. Noise Engine
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("noiseEnable"),     "Noise Enable",      false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("noiseType"),     "Noise Type",
        juce::StringArray{ "White Noise", "Pink Noise", "Brown Noise", "Vinyl Crackle", "Digital Bitnoise", "Breath / Air" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noiseLevel"),     "Noise Level",       normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noisePitch"),     "Noise Pitch",       normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noiseKeyTrack"),  "Noise KeyTrack",    normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noisePhase"),     "Noise Phase",       normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noisePan"),       "Noise Pan",         bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("noiseFilter"),    "Noise LP/HP Filter", bipolarRange(), 0.0f));

    // 7. Arpeggiator
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("arpEnable"),       "Arp Enable",        false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("arpMode"),       "Arp Mode",
        juce::StringArray{ "Up", "Down", "Up/Down", "Random", "Chord", "Step Seq" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("arpOctaves"),       "Arp Octaves",       1, 4, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("arpRate"),       "Arp Rate",
        juce::StringArray{ "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("arpGate"),        "Arp Gate",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.01f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("arpSwing"),       "Arp Swing",
        juce::NormalisableRange<float>(0.0f, 0.75f, 0.01f), 0.0f));

    // 8. Multi-FX Rack
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("blurEnable"),      "Blur Enable",       true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("blurTime"),       "Blur Time",         normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("blurHarmonic"),   "Harmonic Blur",     normRange(), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("tremEnable"),      "Tremolo Enable",    true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("tremDepth"),      "Tremolo Depth",     normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("tremSpeed"),      "Tremolo Speed",
        juce::NormalisableRange<float>(0.1f, 15.0f, 0.01f, 0.5f), 3.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("tremGap"),        "Tremolo Gap",       normRange(), 0.35f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("distEnable"),      "Distortion Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("distType"),      "Distortion Type",
        juce::StringArray{ "Tube", "Tape", "Hard Clip", "Wavefold", "Bitcrush Fuzz", "Asym Scream" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("distDrive"),      "Distortion Drive",  normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("distTone"),       "Distortion Tone",   normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("distMix"),        "Distortion Mix",    normRange(), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("phaserEnable"),    "Phaser Enable",     true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("phaserMix"),      "Phaser Mix",        normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("phaserWidth"),    "Phaser Width",      normRange(), 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("phaserOffset"),   "Phaser Offset",     normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("phaserSpeed"),    "Phaser Speed",
        juce::NormalisableRange<float>(0.05f, 10.0f, 0.01f, 0.5f), 1.2f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("chorusEnable"),    "Chorus Enable",     false));
    params.push_back(std::make_unique<juce::AudioParameterInt>(pid("chorusVoices"),     "Chorus Voices",     2, 6, 4));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusRate"),     "Chorus Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusDepth"),    "Chorus Depth",      normRange(), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusSpread"),   "Chorus Spread",     normRange(), 0.80f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("chorusMix"),      "Chorus Mix",        normRange(), 0.35f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("eqEnable"),        "EQ Enable",         true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqLowFreq"),      "EQ Low Freq",
        juce::NormalisableRange<float>(30.0f, 400.0f, 1.0f, 0.5f), 110.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqLowGain"),      "EQ Low Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqMidFreq"),      "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 6000.0f, 1.0f, 0.45f), 1200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqMidGain"),      "EQ Mid Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqMidQ"),         "EQ Mid Q",
        juce::NormalisableRange<float>(0.3f, 8.0f, 0.05f, 0.5f), 1.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqHighFreq"),     "EQ High Freq",
        juce::NormalisableRange<float>(1500.0f, 16000.0f, 10.0f, 0.5f), 5500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("eqHighGain"),     "EQ High Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("delayEnable"),     "Delay Enable",      false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayTime"),      "Delay Time/Sync",   normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayFeedback"),  "Delay Feedback",    normRange(), 0.40f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayPingPong"),  "Delay Ping-Pong",   normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayFilter"),    "Delay Filter",      normRange(), 0.60f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("delayMix"),       "Delay Mix",         normRange(), 0.28f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("reverbEnable"),    "Reverb Enable",     false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbSize"),     "Reverb Size",       normRange(), 0.68f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbDecay"),    "Reverb Decay",      normRange(), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbPreDelay"), "Reverb Pre-Delay",  normRange(), 0.18f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbDamping"),  "Reverb Damping",    normRange(), 0.40f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbShimmer"),  "Reverb Shimmer",    normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("reverbMix"),      "Reverb Mix",        normRange(), 0.25f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("compEnable"),      "Compressor Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("compMode"),      "Compressor Mode",
        juce::StringArray{ "Single-Band VCA", "Multiband OTT", "Opto Smooth", "Punch Limiter" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("compThreshold"),  "Comp Threshold",
        juce::NormalisableRange<float>(-48.0f, 0.0f, 0.1f), -18.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("compRatio"),      "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("compAttack"),     "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.45f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("compRelease"),    "Comp Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.45f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("compMakeup"),     "Comp Makeup dB",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("compMix"),        "Comp Mix",          normRange(), 0.75f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("ringEnable"),      "Ring Mod Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("ringMode"),      "Ring Mod Mode",
        juce::StringArray{ "Ring Mod", "Freq Shifter", "Stereo Auto-Pan", "Rotary Speaker" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ringFreq"),       "Ring Carrier Hz",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.45f), 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ringDepth"),      "Ring Mod Depth",    normRange(), 0.70f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ringLfoRate"),    "Ring LFO Rate",
        juce::NormalisableRange<float>(0.05f, 15.0f, 0.01f, 0.5f), 1.20f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ringLfoDepth"),   "Ring LFO Depth",    normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("ringMix"),        "Ring Mod Mix",      normRange(), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("crushEnable"),     "Bitcrush Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("crushMode"),     "Bitcrush Mode",
        juce::StringArray{ "Digital Crush", "Sample Decimate", "SP-1200 Vintage", "Radio Lo-Fi" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("crushBits"),      "Crush Bit Depth",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.1f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("crushDownsample"),"Crush Downsample",
        juce::NormalisableRange<float>(1.0f, 64.0f, 0.1f, 0.45f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("crushDrive"),     "Crush Drive",       normRange(), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("crushTone"),      "Crush Tone",        normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("crushMix"),       "Crush Mix",         normRange(), 0.55f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("flangerEnable"),   "Flanger Enable",    false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("flangerMode"),   "Flanger Mode",
        juce::StringArray{ "Jet Flanger", "Through-Zero", "Barberpole", "Step Flanger" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("flangerRate"),    "Flanger Rate",
        juce::NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.5f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("flangerDepth"),   "Flanger Depth",     normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("flangerFeedback"),"Flanger Feedback",
        juce::NormalisableRange<float>(-0.95f, 0.95f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("flangerManual"),  "Flanger Manual",    normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("flangerMix"),     "Flanger Mix",       normRange(), 0.50f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("bodeEnable"),      "Bode Enable",       false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("bodeShift"),      "Bode Shift Hz",
        juce::NormalisableRange<float>(-5000.0f, 5000.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("bodeFeedback"),   "Bode Feedback",
        juce::NormalisableRange<float>(-0.88f, 0.88f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("bodeMix"),        "Bode Mix",          normRange(), 0.50f));

    // 8.5. Time Modulation
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("timeModEnable"),   "Time Mod Enable",   false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("timeModMode"),   "Time Mod Mode",
        juce::StringArray{ "Time Warp Curve", "Stutter Repeat", "Tape Stop", "Reverse Grain", "Half-Time 1/2x", "DJ Scratch" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(pid("timeModRate"),   "Time Mod Rate",
        juce::StringArray{ "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timeModDepth"),   "Time Warp Depth",   normRange(), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timeModStutter"), "Stutter Density",   normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timeModSpeed"),   "Tape Speed Slope",  bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timeModSmooth"),  "De-Click Smooth",   normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timeModFilter"),  "Time DJ Filter",    bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("timeModMix"),     "Time Mod Mix",      normRange(), 0.85f));

    // 8.8. AHDSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("envAttack"),      "Env Attack",        normRange(), 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("envHold"),        "Env Hold",          normRange(), 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("envDecay"),       "Env Decay",         normRange(), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("envSustain"),     "Env Sustain",       normRange(), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("envRelease"),     "Env Release",       normRange(), 0.25f));

    // 9. Master & A/B
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("abMix"),          "A/B Part Mix",      normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("velLink"),        "Velocity Link",     normRange(), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("partPan"),        "Part Pan",          bipolarRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("strumTime"),      "Strum Time",        normRange(), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("strumTension"),   "Strum Slope",       bipolarRange(), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("masterGainDb"),   "Master Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(pid("limiterEnable"),   "Limiter Enable",    true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(pid("limiterThreshDb"),"Limiter Ceiling",
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

    p.oscAOct         = static_cast<int>(std::round(getF(cachedParams.oscAOct)));
    p.oscASemi        = static_cast<int>(std::round(getF(cachedParams.oscASemi)));
    p.oscAFine        = getF(cachedParams.oscAFine);
    p.oscACoarse      = getF(cachedParams.oscACoarse);
    p.startPhase      = getF(cachedParams.startPhase);
    p.phaseRandom     = getF(cachedParams.phaseRandom);
    p.timbre          = getF(cachedParams.timbre);
    p.warpMode        = static_cast<int>(std::round(getF(cachedParams.warpMode)));
    p.warpAmount      = getF(cachedParams.warpAmount);
    p.harmonicScale   = getF(cachedParams.harmonicScale);
    p.harmonicTilt    = getF(cachedParams.harmonicTilt);

    p.oscBOct         = static_cast<int>(std::round(getF(cachedParams.oscBOct)));
    p.oscBSemi        = static_cast<int>(std::round(getF(cachedParams.oscBSemi)));
    p.oscBFine        = getF(cachedParams.oscBFine);
    p.oscBPitch       = getF(cachedParams.oscBPitch);
    p.oscBPhase       = getF(cachedParams.oscBPhase);
    p.timbreB         = getF(cachedParams.timbreB);
    p.warpModeB       = static_cast<int>(std::round(getF(cachedParams.warpModeB)));
    p.warpAmountB     = getF(cachedParams.warpAmountB);
    p.harmonicScaleB  = getF(cachedParams.harmonicScaleB);
    p.harmonicTiltB   = getF(cachedParams.harmonicTiltB);

    p.prismEnable     = (getF(cachedParams.prismEnable) >= 0.5f);
    p.prismAmount     = getF(cachedParams.prismAmount);
    p.prismMode       = static_cast<int>(std::round(getF(cachedParams.prismMode)));
    p.harmShift       = getF(cachedParams.harmShift);
    p.harmGap         = getF(cachedParams.harmGap);
    p.harmWidth       = getF(cachedParams.harmWidth);

    p.pluckEnable     = (getF(cachedParams.pluckEnable) >= 0.5f);
    p.pluckTime       = getF(cachedParams.pluckTime);
    p.pluckBlur       = getF(cachedParams.pluckBlur);

    p.voicingMode     = static_cast<int>(std::round(getF(cachedParams.voicingMode)));
    p.maxVoices       = static_cast<int>(std::round(getF(cachedParams.maxVoices)));
    p.pitchFreq       = getF(cachedParams.pitchFreq);
    p.portaTime       = getF(cachedParams.portaTime);
    p.portaCurve      = getF(cachedParams.portaCurve);
    p.autoGlide       = (getF(cachedParams.autoGlide) >= 0.5f);

    p.unisonVoices    = static_cast<int>(std::round(getF(cachedParams.unisonVoices)));
    p.unisonDetune    = getF(cachedParams.unisonDetune);
    p.unisonSpread    = getF(cachedParams.unisonSpread);
    p.unisonBlend     = getF(cachedParams.unisonBlend);
    p.unisonPhase     = getF(cachedParams.unisonPhase);
    p.unisonStackMode = static_cast<int>(std::round(getF(cachedParams.unisonStackMode)));

    p.vibDepth        = getF(cachedParams.vibDepth);
    p.vibSpeed        = getF(cachedParams.vibSpeed);

    p.filter1Enable   = (getF(cachedParams.filter1Enable) >= 0.5f);
    p.filter1Type     = static_cast<int>(std::round(getF(cachedParams.filter1Type)));
    p.filter1Cutoff   = getF(cachedParams.filter1Cutoff);
    p.filter1Res      = getF(cachedParams.filter1Res);
    p.filter1KeyTrack = getF(cachedParams.filter1KeyTrack);
    p.filter1Width    = getF(cachedParams.filter1Width);

    p.filter2Enable   = (getF(cachedParams.filter2Enable) >= 0.5f);
    p.filter2Type     = static_cast<int>(std::round(getF(cachedParams.filter2Type)));
    p.filter2Cutoff   = getF(cachedParams.filter2Cutoff);
    p.filter2Res      = getF(cachedParams.filter2Res);
    p.filter2KeyTrack = getF(cachedParams.filter2KeyTrack);
    p.filter2Width    = getF(cachedParams.filter2Width);

    p.filterRouting   = getF(cachedParams.filterRouting);
    p.filterDrive     = getF(cachedParams.filterDrive);

    p.noiseEnable     = (getF(cachedParams.noiseEnable) >= 0.5f);
    p.noiseType       = static_cast<int>(std::round(getF(cachedParams.noiseType)));
    p.noiseLevel      = getF(cachedParams.noiseLevel);
    p.noisePitch      = getF(cachedParams.noisePitch);
    p.noiseKeyTrack   = getF(cachedParams.noiseKeyTrack);
    p.noisePhase      = getF(cachedParams.noisePhase);
    p.noisePan        = getF(cachedParams.noisePan);
    p.noiseFilter     = getF(cachedParams.noiseFilter);

    p.arpEnable       = (getF(cachedParams.arpEnable) >= 0.5f);
    p.arpMode         = static_cast<int>(std::round(getF(cachedParams.arpMode)));
    p.arpOctaves      = static_cast<int>(std::round(getF(cachedParams.arpOctaves)));
    p.arpRate         = static_cast<int>(std::round(getF(cachedParams.arpRate)));
    p.arpGate         = getF(cachedParams.arpGate);
    p.arpSwing        = getF(cachedParams.arpSwing);

    p.blurEnable      = (getF(cachedParams.blurEnable) >= 0.5f);
    p.blurTime        = getF(cachedParams.blurTime);
    p.blurHarmonic    = getF(cachedParams.blurHarmonic);

    p.tremEnable      = (getF(cachedParams.tremEnable) >= 0.5f);
    p.tremDepth       = getF(cachedParams.tremDepth);
    p.tremSpeed       = getF(cachedParams.tremSpeed);
    p.tremGap         = getF(cachedParams.tremGap);

    p.distEnable      = (getF(cachedParams.distEnable) >= 0.5f);
    p.distType        = static_cast<int>(std::round(getF(cachedParams.distType)));
    p.distDrive       = getF(cachedParams.distDrive);
    p.distTone        = getF(cachedParams.distTone);
    p.distMix         = getF(cachedParams.distMix);

    p.phaserEnable    = (getF(cachedParams.phaserEnable) >= 0.5f);
    p.phaserMix       = getF(cachedParams.phaserMix);
    p.phaserWidth     = getF(cachedParams.phaserWidth);
    p.phaserOffset    = getF(cachedParams.phaserOffset);
    p.phaserSpeed     = getF(cachedParams.phaserSpeed);

    p.chorusEnable    = (getF(cachedParams.chorusEnable) >= 0.5f);
    p.chorusVoices    = static_cast<int>(std::round(getF(cachedParams.chorusVoices)));
    p.chorusRate      = getF(cachedParams.chorusRate);
    p.chorusDepth     = getF(cachedParams.chorusDepth);
    p.chorusSpread    = getF(cachedParams.chorusSpread);
    p.chorusMix       = getF(cachedParams.chorusMix);

    p.eqEnable        = (getF(cachedParams.eqEnable) >= 0.5f);
    p.eqLowFreq       = getF(cachedParams.eqLowFreq);
    p.eqLowGain       = getF(cachedParams.eqLowGain);
    p.eqMidFreq       = getF(cachedParams.eqMidFreq);
    p.eqMidGain       = getF(cachedParams.eqMidGain);
    p.eqMidQ          = getF(cachedParams.eqMidQ);
    p.eqHighFreq      = getF(cachedParams.eqHighFreq);
    p.eqHighGain      = getF(cachedParams.eqHighGain);

    p.delayEnable     = (getF(cachedParams.delayEnable) >= 0.5f);
    p.delayTime       = getF(cachedParams.delayTime);
    p.delayFeedback   = getF(cachedParams.delayFeedback);
    p.delayPingPong   = getF(cachedParams.delayPingPong);
    p.delayFilter     = getF(cachedParams.delayFilter);
    p.delayMix        = getF(cachedParams.delayMix);

    p.reverbEnable    = (getF(cachedParams.reverbEnable) >= 0.5f);
    p.reverbSize      = getF(cachedParams.reverbSize);
    p.reverbDecay     = getF(cachedParams.reverbDecay);
    p.reverbPreDelay  = getF(cachedParams.reverbPreDelay);
    p.reverbDamping   = getF(cachedParams.reverbDamping);
    p.reverbShimmer   = getF(cachedParams.reverbShimmer);
    p.reverbMix       = getF(cachedParams.reverbMix);

    p.compEnable      = (getF(cachedParams.compEnable) >= 0.5f);
    p.compMode        = static_cast<int>(std::round(getF(cachedParams.compMode)));
    p.compThreshold   = getF(cachedParams.compThreshold);
    p.compRatio       = getF(cachedParams.compRatio);
    p.compAttack      = getF(cachedParams.compAttack);
    p.compRelease     = getF(cachedParams.compRelease);
    p.compMakeup      = getF(cachedParams.compMakeup);
    p.compMix         = getF(cachedParams.compMix);

    p.ringEnable      = (getF(cachedParams.ringEnable) >= 0.5f);
    p.ringMode        = static_cast<int>(std::round(getF(cachedParams.ringMode)));
    p.ringFreq        = getF(cachedParams.ringFreq);
    p.ringDepth       = getF(cachedParams.ringDepth);
    p.ringLfoRate     = getF(cachedParams.ringLfoRate);
    p.ringLfoDepth    = getF(cachedParams.ringLfoDepth);
    p.ringMix         = getF(cachedParams.ringMix);

    p.crushEnable     = (getF(cachedParams.crushEnable) >= 0.5f);
    p.crushMode       = static_cast<int>(std::round(getF(cachedParams.crushMode)));
    p.crushBits       = getF(cachedParams.crushBits);
    p.crushDownsample = getF(cachedParams.crushDownsample);
    p.crushDrive      = getF(cachedParams.crushDrive);
    p.crushTone       = getF(cachedParams.crushTone);
    p.crushMix        = getF(cachedParams.crushMix);

    p.flangerEnable   = (getF(cachedParams.flangerEnable) >= 0.5f);
    p.flangerMode     = static_cast<int>(std::round(getF(cachedParams.flangerMode)));
    p.flangerRate     = getF(cachedParams.flangerRate);
    p.flangerDepth    = getF(cachedParams.flangerDepth);
    p.flangerFeedback = getF(cachedParams.flangerFeedback);
    p.flangerManual   = getF(cachedParams.flangerManual);
    p.flangerMix      = getF(cachedParams.flangerMix);

    p.bodeEnable      = (getF(cachedParams.bodeEnable) >= 0.5f);
    p.bodeShift       = getF(cachedParams.bodeShift);
    p.bodeFeedback    = getF(cachedParams.bodeFeedback);
    p.bodeMix         = getF(cachedParams.bodeMix);

    p.timeModEnable   = (getF(cachedParams.timeModEnable) >= 0.5f);
    p.timeModMode     = static_cast<int>(std::round(getF(cachedParams.timeModMode)));
    p.timeModRate     = static_cast<int>(std::round(getF(cachedParams.timeModRate)));
    p.timeModDepth    = getF(cachedParams.timeModDepth);
    p.timeModStutter  = getF(cachedParams.timeModStutter);
    p.timeModSpeed    = getF(cachedParams.timeModSpeed);
    p.timeModSmooth   = getF(cachedParams.timeModSmooth);
    p.timeModFilter   = getF(cachedParams.timeModFilter);
    p.timeModMix      = getF(cachedParams.timeModMix);

    p.envAttack       = getF(cachedParams.envAttack);
    p.envHold         = getF(cachedParams.envHold);
    p.envDecay        = getF(cachedParams.envDecay);
    p.envSustain      = getF(cachedParams.envSustain);
    p.envRelease      = getF(cachedParams.envRelease);

    p.abMix           = getF(cachedParams.abMix);
    p.velLink         = getF(cachedParams.velLink);
    p.partPan         = getF(cachedParams.partPan);
    p.strumTime       = getF(cachedParams.strumTime);
    p.strumTension    = getF(cachedParams.strumTension);

    p.masterGainDb    = getF(cachedParams.masterGainDb);
    p.limiterEnable   = (getF(cachedParams.limiterEnable) >= 0.5f);
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
        case 1:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 9.0f);
            setParam("filter1Cutoff", 0.52f); setParam("filter1Res", 0.62f); setParam("filter1Width", 0.48f);
            setParam("filter2Enable", 1.0f); setParam("filter2Type", 6.0f);
            setParam("filter2Cutoff", 0.40f); setParam("filter2Res", 0.45f);
            setParam("filterRouting", 0.50f); setParam("filterDrive", 0.42f);
            break;
        case 2:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 2.0f);
            setParam("filter1Cutoff", 0.38f); setParam("filter1Res", 0.58f); setParam("filter1Width", 0.32f);
            setParam("filter2Enable", 0.0f); setParam("filterDrive", 0.65f);
            break;
        case 3:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 6.0f);
            setParam("filter1Cutoff", 0.48f); setParam("filter1Res", 0.68f); setParam("filter1Width", 0.52f);
            setParam("filter2Enable", 1.0f); setParam("filter2Type", 0.0f); setParam("filter2Cutoff", 0.74f);
            setParam("filterRouting", 0.0f); setParam("filterDrive", 0.20f);
            break;
        case 4:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 8.0f);
            setParam("filter1Cutoff", 0.56f); setParam("filter1Res", 0.78f); setParam("filter1Width", 0.28f);
            setParam("filter2Enable", 1.0f); setParam("filter2Type", 9.0f);
            setParam("filter2Cutoff", 0.62f); setParam("filter2Res", 0.45f);
            setParam("filterRouting", 0.45f); setParam("filterDrive", 0.35f);
            break;
        case 5:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 5.0f);
            setParam("filter1Cutoff", 0.64f); setParam("filter1Res", 0.74f); setParam("filter1Width", 0.25f);
            setParam("filter2Enable", 0.0f); setParam("filterDrive", 0.38f);
            break;
        case 6:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 1.0f);
            setParam("filter1Cutoff", 0.42f); setParam("filter1Res", 0.85f); setParam("filter1Width", 0.22f);
            setParam("filter2Enable", 0.0f); setParam("filterDrive", 0.78f);
            break;
        case 7:
            setParam("filter1Enable", 1.0f); setParam("filter1Type", 4.0f);
            setParam("filter1Cutoff", 0.38f); setParam("filter1Res", 0.65f);
            setParam("filter2Enable", 1.0f); setParam("filter2Type", 4.0f);
            setParam("filter2Cutoff", 0.72f); setParam("filter2Res", 0.65f);
            setParam("filterRouting", 1.0f); setParam("filterDrive", 0.45f);
            break;
        default: break;
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
        case 1: setParam("distEnable", 1.0f); setParam("distType", 0.0f); setParam("distDrive", 0.38f); setParam("distTone", 0.62f); setParam("distMix", 0.52f); break;
        case 2: setParam("distEnable", 1.0f); setParam("distType", 5.0f); setParam("distDrive", 0.74f); setParam("distTone", 0.78f); setParam("distMix", 0.65f); break;
        case 3: setParam("distEnable", 1.0f); setParam("distType", 4.0f); setParam("distDrive", 0.68f); setParam("distTone", 0.58f); setParam("distMix", 0.55f); break;
        case 4: setParam("distEnable", 1.0f); setParam("distType", 3.0f); setParam("distDrive", 0.76f); setParam("distTone", 0.68f); setParam("distMix", 0.60f); break;
        case 5: setParam("distEnable", 1.0f); setParam("distType", 1.0f); setParam("distDrive", 0.28f); setParam("distTone", 0.54f); setParam("distMix", 0.45f); break;
        case 6: setParam("distEnable", 1.0f); setParam("distType", 2.0f); setParam("distDrive", 0.85f); setParam("distTone", 0.72f); setParam("distMix", 0.70f); break;
        default: break;
    }
}

juce::StringArray PrismorAudioProcessor::getFilterSubPresetNames() const
{
    return { "Talkbox Wow Lead", "Reese Sub Lowpass", "Vocal Choir (A-E-I-O-U)", "Robotic Vocoder Bank", "Metallic Comb Flange", "Screaming Acid 24dB", "Dual Bandpass Notch" };
}

juce::StringArray PrismorAudioProcessor::getDistortionSubPresetNames() const
{
    return { "Warm Tube Saturation", "Screaming Acid Drive", "Bitcrush Fuzz", "Heavy Wavefold", "Tape Master Glue", "Brutal Neuro Clipper" };
}

juce::File PrismorAudioProcessor::getUserPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CyberSpectralAudio")
                   .getChildFile("Prismor")
                   .getChildFile("UserPresets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

std::vector<PrismorAudioProcessor::FactoryPresetMetadata> PrismorAudioProcessor::getFactoryPresetBank() const
{
    const auto& rawEntries = PrismorDSP::FactoryPresetBank::getEntries();
    std::vector<FactoryPresetMetadata> meta;
    meta.reserve(rawEntries.size());

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
    return meta;
}

std::vector<PrismorAudioProcessor::FactoryPresetMetadata> PrismorAudioProcessor::getUserPresetMetadata() const
{
    std::vector<FactoryPresetMetadata> meta;
    auto userDir = getUserPresetDirectory();
    if (! userDir.isDirectory())
        return meta;

    auto userFiles = userDir.findChildFiles(juce::File::findFiles, true, "*.prismor;*.xml");
    int uIdx = PrismorDSP::FactoryPresetBank::getPresetCount();
    for (const auto& f : userFiles)
    {
        auto xml = juce::XmlDocument::parse(f);
        juce::String pName = f.getFileNameWithoutExtension();
        juce::String pCat = "USER";
        juce::String pAuthor = "User";
        juce::String pDesc = "User created preset";

        if (xml != nullptr)
        {
            if (xml->hasAttribute("presetName"))  pName = xml->getStringAttribute("presetName");
            if (xml->hasAttribute("category"))    pCat = xml->getStringAttribute("category");
            if (xml->hasAttribute("author"))      pAuthor = xml->getStringAttribute("author");
            if (xml->hasAttribute("description")) pDesc = xml->getStringAttribute("description");
        }

        meta.push_back({ uIdx++, pName, pCat, pAuthor, "CUSTOM", "USER FX", pDesc, true, f });
    }
    return meta;
}

juce::StringArray PrismorAudioProcessor::getFactoryPresetNames() const
{
    const auto& rawEntries = PrismorDSP::FactoryPresetBank::getEntries();
    juce::StringArray names;
    for (const auto& e : rawEntries)
        names.add(juce::String(e.name));
    return names;
}

void PrismorAudioProcessor::loadFactoryPreset(int index)
{
    const int count = PrismorDSP::FactoryPresetBank::getPresetCount();
    if (count <= 0) return;

    const int clamped = juce::jlimit(0, count - 1, index);
    currentPreset = clamped;

    auto setParam = [this](const char* id, float plainVal)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(plainVal));
    };

    PrismorDSP::FactoryPresetBank::applyPreset(clamped, setParam, synthEngine);
}

bool PrismorAudioProcessor::exportPresetToFile(const juce::File& file, const juce::String& name, const juce::String& category, const juce::String& author, const juce::String& description)
{
    auto state = apvts.copyState();
    appendEngineStateToTree(state);
    state.setProperty("presetName", name, nullptr);
    state.setProperty("category", category, nullptr);
    state.setProperty("author", author, nullptr);
    state.setProperty("description", description, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
        return xml->writeTo(file);
    return false;
}

bool PrismorAudioProcessor::saveUserPreset(const juce::String& name, const juce::String& category, const juce::String& author, const juce::String& description)
{
    auto userDir = getUserPresetDirectory();
    auto catDir = userDir.getChildFile(category.isEmpty() ? "USER" : category);
    if (! catDir.exists())
        catDir.createDirectory();

    auto file = catDir.getChildFile(juce::File::createLegalFileName(name) + ".prismor");
    return exportPresetToFile(file, name, category, author, description);
}

bool PrismorAudioProcessor::loadPresetFromXml(const juce::XmlElement& xmlState)
{
    if (xmlState.hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(xmlState);
        extractEngineStateFromTree(tree);
        apvts.replaceState(tree);
        snapDiscreteParameters(*this);
        return true;
    }
    return false;
}

bool PrismorAudioProcessor::importPresetFromFile(const juce::File& file)
{
    if (! file.existsAsFile())
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

const juce::String PrismorAudioProcessor::getName() const { return JucePlugin_Name; }
bool PrismorAudioProcessor::acceptsMidi() const { return true; }
bool PrismorAudioProcessor::producesMidi() const { return false; }
bool PrismorAudioProcessor::isMidiEffect() const { return false; }
double PrismorAudioProcessor::getTailLengthSeconds() const { return 2.5; }

int PrismorAudioProcessor::getNumPrograms() { return PrismorDSP::FactoryPresetBank::getPresetCount(); }
int PrismorAudioProcessor::getCurrentProgram() { return currentPreset; }
void PrismorAudioProcessor::setCurrentProgram(int index) { loadFactoryPreset(index); }
const juce::String PrismorAudioProcessor::getProgramName(int index)
{
    const auto& entries = PrismorDSP::FactoryPresetBank::getEntries();
    if (entries.empty()) return "INIT";
    const int safeIdx = juce::jlimit(0, static_cast<int>(entries.size()) - 1, index);
    return juce::String(entries[static_cast<size_t>(safeIdx)].name);
}
void PrismorAudioProcessor::changeProgramName(int, const juce::String&) {}

void PrismorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    loadMeasurer.reset(sampleRate, samplesPerBlock);
    synthEngine.prepare(sampleRate, samplesPerBlock);
    stereoScratchBuffer.setSize(2, kMaxProcChunk, false, true, true);
    keyboardState.reset();
}

void PrismorAudioProcessor::releaseResources()
{
    synthEngine.resetAllVoices();
}

bool PrismorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo();
}

void PrismorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    juce::AudioProcessLoadMeasurer::ScopedTimer timer(loadMeasurer, numSamples);
    juce::ScopedNoDenormals noDenormals;

    for (int ch = 0; ch < numChannels; ++ch)
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

    if (numChannels >= 2)
    {
        synthEngine.renderBlock(buffer, midiMessages, params);
    }
    else
    {
        // Mono output bus: slice into <= kMaxProcChunk stereo chunks and sum to mono safely
        float* monoOut = buffer.getWritePointer(0);
        int offset = 0;
        juce::MidiBuffer chunkMidi;

        while (offset < numSamples)
        {
            const int chunk = std::min(kMaxProcChunk, numSamples - offset);
            chunkMidi.clear();
            for (const auto meta : midiMessages)
            {
                if (meta.samplePosition >= offset && meta.samplePosition < offset + chunk)
                    chunkMidi.addEvent(meta.getMessage(), meta.samplePosition - offset);
            }

            juce::AudioBuffer<float> sub(stereoScratchBuffer.getArrayOfWritePointers(), 2, 0, chunk);
            sub.clear();
            synthEngine.renderBlock(sub, chunkMidi, params);

            const float* srcL = sub.getReadPointer(0);
            const float* srcR = sub.getReadPointer(1);
            for (int i = 0; i < chunk; ++i)
                monoOut[offset + i] = 0.5f * (srcL[i] + srcR[i]);

            offset += chunk;
        }
    }

    // Final safety ceiling & NaN/Inf guard
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            channelData[i] = PrismorDSP::softClipC1(channelData[i], 0.90f, 1.0f);
    }
}

bool PrismorAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PrismorAudioProcessor::createEditor()
{
    return new WebViewEditor(*this);
}

void PrismorAudioProcessor::appendEngineStateToTree(juce::ValueTree& state) const
{
    state.setProperty("currentPreset", currentPreset, nullptr);

    juce::ValueTree artTree("Articulators");
    for (int i = 0; i < static_cast<int>(PrismorDSP::ArticulatorTarget::Count); ++i)
    {
        const auto target = static_cast<PrismorDSP::ArticulatorTarget>(i);
        artTree.appendChild(synthEngine.getArticulator(target).toValueTree("Curve_" + juce::String(i)), nullptr);
    }
    artTree.appendChild(synthEngine.getTimeModCurve().toValueTree("TimeModCurve"), nullptr);
    state.appendChild(artTree, nullptr);

    juce::ValueTree fxTree("FxChain");
    juce::StringArray fxOrderStrs;
    for (int modId : synthEngine.getActiveFxChain())
        fxOrderStrs.add(juce::String(modId));
    fxTree.setProperty("order", fxOrderStrs.joinIntoString(","), nullptr);
    state.appendChild(fxTree, nullptr);

    juce::ValueTree waveTree("WaveTypes");
    waveTree.setProperty("waveA", synthEngine.getResynthesisEngine().getActiveWaveType(0), nullptr);
    waveTree.setProperty("waveB", synthEngine.getResynthesisEngine().getActiveWaveType(1), nullptr);
    state.appendChild(waveTree, nullptr);

    juce::ValueTree arpTree("ArpSteps");
    for (int i = 0; i < PrismorDSP::SpectralSynthEngine::NUM_ARP_STEPS; ++i)
    {
        const auto s = synthEngine.getArpStep(i);
        juce::ValueTree st("Step_" + juce::String(i));
        st.setProperty("enabled", s.enabled, nullptr);
        st.setProperty("velocity", s.velocity, nullptr);
        st.setProperty("octave", s.octaveOffset, nullptr);
        st.setProperty("semi", s.semitoneTranspose, nullptr);
        arpTree.appendChild(st, nullptr);
    }
    state.appendChild(arpTree, nullptr);
}

void PrismorAudioProcessor::extractEngineStateFromTree(juce::ValueTree& tree)
{
    if (tree.hasProperty("currentPreset"))
        currentPreset = static_cast<int>(tree.getProperty("currentPreset", 0));

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

    auto fxTree = tree.getChildWithName("FxChain");
    if (fxTree.isValid())
    {
        const juce::String orderStr = fxTree.getProperty("order", "").toString();
        if (orderStr.isNotEmpty())
        {
            std::vector<int> chain;
            for (const auto& tok : juce::StringArray::fromTokens(orderStr, ",", ""))
                chain.push_back(tok.getIntValue());
            if (! chain.empty())
                synthEngine.setActiveFxChain(chain);
        }
        tree.removeChild(fxTree, nullptr);
    }

    auto waveTree = tree.getChildWithName("WaveTypes");
    if (waveTree.isValid())
    {
        const int wA = static_cast<int>(waveTree.getProperty("waveA", 0));
        const int wB = static_cast<int>(waveTree.getProperty("waveB", 1));
        synthEngine.getResynthesisEngine().resetPartTableToWaveform(0, wA);
        synthEngine.getResynthesisEngine().resetPartTableToWaveform(1, wB);
        tree.removeChild(waveTree, nullptr);
    }

    auto arpTree = tree.getChildWithName("ArpSteps");
    if (arpTree.isValid())
    {
        for (int i = 0; i < PrismorDSP::SpectralSynthEngine::NUM_ARP_STEPS; ++i)
        {
            auto st = arpTree.getChildWithName("Step_" + juce::String(i));
            if (st.isValid())
            {
                PrismorDSP::ArpStep s;
                s.enabled = static_cast<bool>(st.getProperty("enabled", true));
                s.velocity = static_cast<float>(st.getProperty("velocity", 0.85f));
                s.octaveOffset = static_cast<int>(st.getProperty("octave", 0));
                s.semitoneTranspose = static_cast<int>(st.getProperty("semi", 0));
                synthEngine.setArpStep(i, s);
            }
        }
        tree.removeChild(arpTree, nullptr);
    }
}

void PrismorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Synchronously flush every APVTS parameter's exact current plain value into the ValueTree
    for (const auto& paramId : getAllParameterIds())
    {
        if (auto* param = apvts.getParameter(paramId))
        {
            const float plainVal = param->convertFrom0to1(param->getValue());
            bool found = false;
            for (int c = 0; c < state.getNumChildren(); ++c)
            {
                auto child = state.getChild(c);
                if (child.hasType("PARAM") && child.getProperty("id").toString() == paramId)
                {
                    child.setProperty("value", plainVal, nullptr);
                    found = true;
                    break;
                }
            }
            if (! found)
            {
                juce::ValueTree vt("PARAM");
                vt.setProperty("id", paramId, nullptr);
                vt.setProperty("value", plainVal, nullptr);
                state.appendChild(vt, nullptr);
            }
        }
    }

    appendEngineStateToTree(state);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void PrismorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml(*xmlState);
        extractEngineStateFromTree(tree);
        apvts.replaceState(tree);
        snapDiscreteParameters(*this);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PrismorAudioProcessor();
}
