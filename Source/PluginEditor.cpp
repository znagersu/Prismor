#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace PrismorUI;

PrismorAudioProcessorEditor::PrismorAudioProcessorEditor(PrismorAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      timeModEditor(p.getSynthEngine()),
      presetBrowserView(p),
      articulatorEditor(p.getSynthEngine()),
      arpStepGrid(p.getSynthEngine()),
      visualizer3D(p.getSynthEngine())
{
    setLookAndFeel(&rapidLookAndFeel);

    auto& apvts = audioProcessor.getAPVTS();

    // -------------------------------------------------------------------------
    // TOP BAR: 4-TAB SWITCHER (SYNTH / FX RACK / TIME MOD / PRESETS) & THEME
    // -------------------------------------------------------------------------
    tabSynthBtn.getProperties().set("isNavTab", true);
    tabFxBtn.getProperties().set("isNavTab", true);
    tabTimeModBtn.getProperties().set("isNavTab", true);
    tabPresetsBtn.getProperties().set("isNavTab", true);

    tabSynthBtn.onClick   = [this]() { setActiveTab(0); };
    tabFxBtn.onClick      = [this]() { setActiveTab(1); };
    tabTimeModBtn.onClick = [this]() { setActiveTab(2); };
    tabPresetsBtn.onClick = [this]() { setActiveTab(3); };

    addAndMakeVisible(tabSynthBtn);
    addAndMakeVisible(tabFxBtn);
    addAndMakeVisible(tabTimeModBtn);
    addAndMakeVisible(tabPresetsBtn);

    themeSelector.addItemList(Palette::getThemeNames(), 1);
    themeSelector.setSelectedItemIndex(Palette::currentThemeIndex, juce::dontSendNotification);
    themeSelector.getProperties().set("accent", "amber");
    themeSelector.onChange = [this]()
    {
        applyValhallaTheme(themeSelector.getSelectedItemIndex());
    };
    addAndMakeVisible(themeSelector);

    // -------------------------------------------------------------------------
    // TOP-RIGHT MASTER VOLUME & HARDWARE MACRO KNOBS (Brushed Silver Header)
    // -------------------------------------------------------------------------
    auto setupHdrKnob = [this, &apvts](std::unique_ptr<juce::Slider>& knob,
                                       std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach,
                                       const juce::String& paramId,
                                       const juce::String& accent,
                                       bool isMaster)
    {
        knob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        knob->setName(paramId);
        knob->getProperties().set("paramId", paramId);
        knob->getProperties().set("headerKnob", true);
        knob->getProperties().set("masterKnob", isMaster);
        knob->getProperties().set("accent", accent);
        addAndMakeVisible(*knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, *knob);
    };

    setupHdrKnob(headerMacro1Knob, headerMacro1Attach, "filter1Cutoff",   "cyan",  false);
    setupHdrKnob(headerMacro2Knob, headerMacro2Attach, "filterDrive",     "cyan",  false);
    setupHdrKnob(headerThreshKnob, headerThreshAttach, "limiterThreshDb", "amber", false);
    setupHdrKnob(headerMasterKnob, headerMasterAttach, "masterGainDb",    "amber", true);

    // -------------------------------------------------------------------------
    // SYNTH TAB (tabGroup = 0)
    // -------------------------------------------------------------------------
    const juce::StringArray warpItems { "WARP: OFF", "BEND +/-", "MIRROR", "QUANTIZE", "ASYMMETRIC", "SYNC FOLD" };
    const juce::StringArray waveItems = PrismorDSP::ResynthesisEngine::getWaveformNames();

    // Card 01: Frequency Field 1 (Part A) + Serum-2 Pitch Boxes + Zenology Wave Selector
    oscAOctBox      = &addPitchBox("oscAOct",    "OCT",  "",   0, "cyan",  0);
    oscASemiBox     = &addPitchBox("oscASemi",   "SEMI", "",   0, "cyan",  0);
    oscAFineBox     = &addPitchBox("oscAFine",   "FIN",  "c",  0, "cyan",  0);
    oscACoarseBox   = &addPitchBox("oscACoarse", "CRS",  "st", 1, "amber", 0);

    startPhaseKnob  = &addKnob("startPhase",    "PHASE A", "cyan",  false, 0);
    phaseRandomKnob = &addKnob("phaseRandom",   "RANDOM",  "cyan",  false, 0);
    vibDepthKnob    = &addKnob("vibDepth",      "VIBRATO", "cyan",  false, 0);
    harmScaleKnob   = &addKnob("harmonicScale", "SCALE A", "cyan",  false, 0);
    harmTiltKnob    = &addKnob("harmonicTilt",  "TILT A",  "amber", true,  0);
    warpAmountKnob  = &addKnob("warpAmount",    "WARP A",  "cyan",  true,  0);
    timbreFader     = &addVerticalFader("timbre", "TIMBRE A", "cyan", 0);

    oscAWaveBox.addItemList(waveItems, 1);
    oscAWaveBox.setSelectedItemIndex(0, juce::dontSendNotification);
    oscAWaveBox.getProperties().set("accent", "cyan");
    oscAWaveBox.onChange = [this]()
    {
        const int idx = oscAWaveBox.getSelectedItemIndex();
        if (idx >= 0)
        {
            audioProcessor.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(0, idx);
            visualizer3D.repaint();
            repaint();
        }
    };
    addAndMakeVisible(oscAWaveBox);

    warpModeBox.addItemList(warpItems, 1);
    warpModeBox.getProperties().set("accent", "cyan");
    addAndMakeVisible(warpModeBox);
    warpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "warpMode", warpModeBox);

    // Card 02: Frequency Field 2 (Part B Independent Harmonic Field) + Serum-2 Pitch Boxes + Zenology Wave Selector
    // Strict RAPID Color Scheme: Oscillators styled in Ice-Blue ("cyan")
    oscBOctBox      = &addPitchBox("oscBOct",   "OCT",  "",   0, "cyan",  0);
    oscBSemiBox     = &addPitchBox("oscBSemi",  "SEMI", "",   0, "cyan",  0);
    oscBFineBox     = &addPitchBox("oscBFine",  "FIN",  "c",  0, "cyan",  0);
    oscBCoarseBox   = &addPitchBox("oscBPitch", "CRS",  "st", 1, "amber", 0);

    vibSpeedKnob       = &addKnob("vibSpeed",       "VIB SPD", "cyan",  false, 0);
    oscBPhaseKnob      = &addKnob("oscBPhase",      "PHASE B", "cyan",  false, 0);
    pluckTimeSynthKnob = &addKnob("pluckTime",      "PLUCK",   "amber", false, 0);
    harmScaleBKnob     = &addKnob("harmonicScaleB", "SCALE B", "cyan",  false, 0);
    harmTiltBKnob      = &addKnob("harmonicTiltB",  "TILT B",  "amber", true,  0);
    warpAmountBKnob    = &addKnob("warpAmountB",    "WARP B",  "cyan",  true,  0);
    timbreBFader       = &addVerticalFader("timbreB", "TIMBRE B", "cyan", 0);

    oscBWaveBox.addItemList(waveItems, 1);
    oscBWaveBox.setSelectedItemIndex(4, juce::dontSendNotification);
    oscBWaveBox.getProperties().set("accent", "cyan");
    oscBWaveBox.onChange = [this]()
    {
        const int idx = oscBWaveBox.getSelectedItemIndex();
        if (idx >= 0)
        {
            audioProcessor.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(1, idx);
            visualizer3D.repaint();
            repaint();
        }
    };
    addAndMakeVisible(oscBWaveBox);

    warpModeBBox.addItemList(warpItems, 1);
    warpModeBBox.getProperties().set("accent", "magenta");
    addAndMakeVisible(warpModeBBox);
    warpModeBAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "warpModeB", warpModeBBox);

    visualizer3D.onWaveformSelected = [this](int partIdx, int waveType)
    {
        if (partIdx == 0)
            oscAWaveBox.setSelectedItemIndex(waveType, juce::dontSendNotification);
        else
            oscBWaveBox.setSelectedItemIndex(waveType, juce::dontSendNotification);
    };

    // Card 03: Prism & Harmonizer
    prismEnableBtn  = &addBypassToggle("prismEnable", "ON", "magenta", 0);
    prismAmountKnob = &addKnob("prismAmount", "PRISM AMT", "magenta", false, 0);
    prismModeBox.addItemList({ "INHARMONIC", "HZ SHIFT", "ODD / EVEN", "QUANTUM" }, 1);
    prismModeBox.getProperties().set("accent", "magenta");
    addAndMakeVisible(prismModeBox);
    prismModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "prismMode", prismModeBox);

    harmShiftFader  = &addVerticalFader("harmShift", "SHIFT", "magenta", 0);
    harmGapFader    = &addVerticalFader("harmGap",   "GAP",   "magenta", 0);
    harmWidthFader  = &addVerticalFader("harmWidth", "WIDTH", "magenta", 0);

    // Card 04: Voicing, Legato, Glide & 16x Serum-2 Unison Engine
    voicingModeBox.addItemList({ "POLYPHONIC", "MONOPHONIC", "TRUE LEGATO" }, 1);
    voicingModeBox.getProperties().set("accent", "cyan");
    addAndMakeVisible(voicingModeBox);
    voicingModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "voicingMode", voicingModeBox);

    unisonStackModeBox.addItemList({
        "STACK: 1x NORMAL", "STACK: +1 OCT", "STACK: +2 OCT", "STACK: SUB+OCT",
        "STACK: FIFTHS", "STACK: OCT+5TH", "STACK: MIN TRIAD", "STACK: MAJ TRIAD"
    }, 1);
    unisonStackModeBox.getProperties().set("accent", "magenta");
    addAndMakeVisible(unisonStackModeBox);
    unisonStackModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "unisonStackMode", unisonStackModeBox);

    autoGlideBtn     = &addBypassToggle("autoGlide", "AUTO GLIDE", "amber", 0);
    maxVoicesKnob    = &addKnob("maxVoices",    "VOICES",   "cyan",    false, 0);
    pitchFreqKnob    = &addKnob("pitchFreq",    "PITCH ST", "cyan",    true,  0);
    portaTimeKnob    = &addKnob("portaTime",    "GLIDE",    "amber",   false, 0);
    portaCurveKnob   = &addKnob("portaCurve",   "CURVE",    "amber",   true,  0);

    unisonVoicesKnob = &addKnob("unisonVoices", "UNISON 1-16", "magenta", false, 0);
    unisonDetuneKnob = &addKnob("unisonDetune", "DETUNE",      "magenta", false, 0);
    unisonSpreadKnob = &addKnob("unisonSpread", "SPREAD",      "magenta", false, 0);
    unisonBlendKnob  = &addKnob("unisonBlend",  "BLEND",       "amber",   false, 0);
    unisonPhaseKnob  = &addKnob("unisonPhase",  "PHASE",       "magenta", false, 0);

    unisonDetuneLinkBtn.setClickingTogglesState(true);
    unisonDetuneLinkBtn.setToggleState(false, juce::dontSendNotification); // Default OFF for authentic Serum 2 independent control
    unisonDetuneLinkBtn.setButtonText("DET-LINK");
    unisonDetuneLinkBtn.setColour(juce::TextButton::buttonColourId, Palette::cardHeader);
    unisonDetuneLinkBtn.setColour(juce::TextButton::buttonOnColourId, Palette::deepMagenta.withAlpha(0.40f));
    unisonDetuneLinkBtn.setColour(juce::TextButton::textColourOnId, Palette::electricCyan);
    unisonDetuneLinkBtn.setColour(juce::TextButton::textColourOffId, Palette::textDim);
    unisonDetuneLinkBtn.onClick = [this]()
    {
        unisonDetuneLinkActive = unisonDetuneLinkBtn.getToggleState();
    };
    addAndMakeVisible(unisonDetuneLinkBtn);

    unisonVoicesKnob->onValueChange = [this]()
    {
        if (unisonDetuneLinkActive && unisonVoicesKnob->isMouseButtonDown())
        {
            auto& procApvts = audioProcessor.getAPVTS();
            const int v = juce::roundToInt(unisonVoicesKnob->getValue());
            if (v > 1)
            {
                // In DETUNE LINK macro mode: intelligently auto-open detune and spread if currently near-zero
                if (auto* paramDet = procApvts.getParameter("unisonDetune"))
                {
                    const float curDet = paramDet->getValue();
                    if (curDet < 0.05f)
                    {
                        const float norm = static_cast<float>(v - 1) / 15.0f;
                        paramDet->setValueNotifyingHost(paramDet->convertTo0to1(0.18f + norm * 0.25f));
                    }
                }
                if (auto* paramSpr = procApvts.getParameter("unisonSpread"))
                {
                    const float curSpr = paramSpr->getValue();
                    if (curSpr < 0.10f)
                    {
                        const float norm = static_cast<float>(v - 1) / 15.0f;
                        paramSpr->setValueNotifyingHost(paramSpr->convertTo0to1(0.60f + norm * 0.35f));
                    }
                }
            }
        }
    };

    // Card 05: Dual Spectral Filter (with Vox Formant, Vocoder Band-Bank & Talkbox) + Sub-Presets
    const juce::StringArray filterModeItems {
        "SVF LP 12dB", "SVF LP 24dB", "CRUDE LOW PASS", "SVF HP 24dB",
        "BANDPASS SVF", "COMB FILTER", "VOX FORMANT (A-E-I-O-U)", "CUSTOM SHAPE",
        "VOCODER BAND-BANK", "TALKBOX MODE"
    };

    filterSubPresetBox.addItemList(audioProcessor.getFilterSubPresetNames(), 1);
    filterSubPresetBox.setTextWhenNothingSelected("FILTER SUB-PRESET...");
    filterSubPresetBox.getProperties().set("accent", "amber");
    filterSubPresetBox.onChange = [this]()
    {
        const int idx = filterSubPresetBox.getSelectedItemIndex();
        if (idx >= 0)
        {
            audioProcessor.loadFilterSubPreset(idx + 1);
            visualizer3D.repaint();
            repaint();
        }
    };
    addAndMakeVisible(filterSubPresetBox);

    filter1EnableBtn = &addBypassToggle("filter1Enable", "F1 ON", "amber", 0);
    filter1TypeBox.addItemList(filterModeItems, 1);
    filter1TypeBox.getProperties().set("accent", "amber");
    addAndMakeVisible(filter1TypeBox);
    filter1TypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "filter1Type", filter1TypeBox);

    filter1CutoffKnob = &addKnob("filter1Cutoff",   "CUTOFF", "amber", false, 0);
    filter1ResKnob    = &addKnob("filter1Res",      "RES",    "amber", false, 0);
    filter1KeyKnob    = &addKnob("filter1KeyTrack", "KEYTRK", "amber", false, 0);
    filter1WidthKnob  = &addKnob("filter1Width",    "WIDTH",  "amber", false, 0);

    filter2EnableBtn = &addBypassToggle("filter2Enable", "F2 ON", "amber", 0);
    filter2TypeBox.addItemList(filterModeItems, 1);
    filter2TypeBox.getProperties().set("accent", "amber");
    addAndMakeVisible(filter2TypeBox);
    filter2TypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "filter2Type", filter2TypeBox);

    filter2CutoffKnob = &addKnob("filter2Cutoff",   "CUTOFF", "amber", false, 0);
    filter2ResKnob    = &addKnob("filter2Res",      "RES",    "amber", false, 0);
    filter2KeyKnob    = &addKnob("filter2KeyTrack", "KEYTRK", "amber", false, 0);
    filter2WidthKnob  = &addKnob("filter2Width",    "WIDTH",  "amber", false, 0);

    filterRoutingKnob = &addKnob("filterRouting",   "SER/PAR", "magenta", true,  0);
    filterDriveKnob   = &addKnob("filterDrive",     "DRIVE",   "amber",   false, 0);

    // Card 06: Dedicated Serum-2 Style Noise Generator
    noiseEnableBtn = &addBypassToggle("noiseEnable", "NOISE ON", "emerald", 0);
    noiseTypeBox.addItemList({
        "WHITE NOISE", "PINK NOISE", "BROWN NOISE",
        "VINYL CRACKLE", "DIGITAL BITNOISE", "BREATH / AIR"
    }, 1);
    noiseTypeBox.getProperties().set("accent", "cyan");
    addAndMakeVisible(noiseTypeBox);
    noiseTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "noiseType", noiseTypeBox);

    noiseLevelKnob    = &addKnob("noiseLevel",    "LEVEL",   "cyan",    false, 0);
    noisePitchKnob    = &addKnob("noisePitch",    "PITCH",   "cyan",    false, 0);
    noiseKeyTrackKnob = &addKnob("noiseKeyTrack", "KEYTRK",  "amber",   false, 0);
    noisePhaseKnob    = &addKnob("noisePhase",    "PHASE",   "cyan",    false, 0);
    noisePanKnob      = &addKnob("noisePan",      "PAN",     "magenta", true,  0);
    noiseFilterKnob   = &addKnob("noiseFilter",   "LP / HP", "amber",   true,  0);

    // -------------------------------------------------------------------------
    // FX RACK TAB CONTROLS (tabGroup = 1, fxModuleId = 0..11)
    // -------------------------------------------------------------------------
    // FX 0: Spectral Blur & Pluck
    blurEnableBtn  = &addBypassToggle("blurEnable",  "ON",         "amber", 1, 0);
    blurTimeKnob   = &addKnob("blurTime",     "TIME BLUR",  "amber", false, 1, 0);
    blurHarmKnob   = &addKnob("blurHarmonic", "HARM BLUR",  "amber", false, 1, 0);
    pluckEnableBtn = &addBypassToggle("pluckEnable", "PLUCK",      "amber", 1, 0);
    pluckTimeKnob  = &addKnob("pluckTime",    "PLUCK TIME", "amber", false, 1, 0);
    pluckBlurKnob  = &addKnob("pluckBlur",    "PLUCK BLUR", "amber", false, 1, 0);

    // FX 1: Harmonic Tremolo
    tremEnableBtn  = &addBypassToggle("tremEnable", "ON", "cyan", 1, 1);
    tremDepthKnob  = &addKnob("tremDepth", "TREM DEPTH", "cyan", false, 1, 1);
    tremSpeedKnob  = &addKnob("tremSpeed", "TREM RATE",  "cyan", false, 1, 1);
    tremGapKnob    = &addKnob("tremGap",   "STEREO GAP", "cyan", false, 1, 1);

    // FX 2: Distortion / Saturation + Sub-Presets
    distEnableBtn  = &addBypassToggle("distEnable", "ON", "magenta", 1, 2);
    distTypeBox.addItemList({ "TUBE SAT", "TAPE WARM", "HARD CLIP", "WAVEFOLD", "BITCRUSH FUZZ", "ASYM SCREAM" }, 1);
    distTypeBox.getProperties().set("accent", "magenta");
    addChildComponent(distTypeBox);
    distTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "distType", distTypeBox);

    distSubPresetBox.addItemList(audioProcessor.getDistortionSubPresetNames(), 1);
    distSubPresetBox.setTextWhenNothingSelected("PRESET...");
    distSubPresetBox.getProperties().set("accent", "amber");
    distSubPresetBox.onChange = [this]()
    {
        const int idx = distSubPresetBox.getSelectedItemIndex();
        if (idx >= 0)
        {
            audioProcessor.loadDistortionSubPreset(idx + 1);
            repaint();
        }
    };
    addChildComponent(distSubPresetBox);

    distDriveKnob  = &addKnob("distDrive", "DRIVE", "magenta", false, 1, 2);
    distToneKnob   = &addKnob("distTone",  "TONE",  "amber",   false, 1, 2);
    distMixKnob    = &addKnob("distMix",   "MIX",   "magenta", false, 1, 2);

    // FX 3: Spectral Phaser
    phaserEnableBtn = &addBypassToggle("phaserEnable", "ON", "magenta", 1, 3);
    phaserMixKnob   = &addKnob("phaserMix",    "MIX",    "magenta", false, 1, 3);
    phaserWidthKnob = &addKnob("phaserWidth",  "WIDTH",  "magenta", false, 1, 3);
    phaserOffsetKnob= &addKnob("phaserOffset", "OFFSET", "magenta", false, 1, 3);
    phaserSpeedKnob = &addKnob("phaserSpeed",  "SPEED",  "magenta", false, 1, 3);

    // FX 4: Multi-Voice Chorus
    chorusEnableBtn = &addBypassToggle("chorusEnable", "ON", "cyan", 1, 4);
    chorusVoicesKnob= &addKnob("chorusVoices", "VOICES", "cyan",    false, 1, 4);
    chorusRateKnob  = &addKnob("chorusRate",   "RATE",   "cyan",    false, 1, 4);
    chorusDepthKnob = &addKnob("chorusDepth",  "DEPTH",  "cyan",    false, 1, 4);
    chorusSpreadKnob= &addKnob("chorusSpread", "SPREAD", "magenta", false, 1, 4);
    chorusMixKnob   = &addKnob("chorusMix",    "MIX",    "cyan",    false, 1, 4);

    // FX 5: 3-Band Parametric EQ
    eqEnableBtn     = &addBypassToggle("eqEnable", "ON", "amber", 1, 5);
    eqLowGainKnob   = &addKnob("eqLowGain",  "LOW dB",  "amber",   true,  1, 5);
    eqMidGainKnob   = &addKnob("eqMidGain",  "MID dB",  "cyan",    true,  1, 5);
    eqHighGainKnob  = &addKnob("eqHighGain", "HIGH dB", "magenta", true,  1, 5);
    eqLowFreqKnob   = &addKnob("eqLowFreq",  "LOW Hz",  "amber",   false, 1, 5);
    eqMidFreqKnob   = &addKnob("eqMidFreq",  "MID Hz",  "cyan",    false, 1, 5);
    eqMidQKnob      = &addKnob("eqMidQ",     "MID Q",   "cyan",    false, 1, 5);
    eqHighFreqKnob  = &addKnob("eqHighFreq", "HIGH Hz", "magenta", false, 1, 5);

    for (auto* k : { eqLowGainKnob, eqMidGainKnob, eqHighGainKnob, eqLowFreqKnob, eqMidFreqKnob, eqMidQKnob, eqHighFreqKnob })
    {
        k->onValueChange = [this]() { if (currentTab == 1) repaint(); };
    }

    // FX 6: Stereo / Ping-Pong Delay
    delayEnableBtn   = &addBypassToggle("delayEnable", "ON", "amber", 1, 6);
    delayTimeKnob    = &addKnob("delayTime",     "TIME/SYNC", "amber", false, 1, 6);
    delayFeedbackKnob= &addKnob("delayFeedback", "FEEDBACK",  "amber", false, 1, 6);
    delayPingPongKnob= &addKnob("delayPingPong", "PING-PONG", "cyan",  false, 1, 6);
    delayFilterKnob  = &addKnob("delayFilter",   "DAMPING",   "amber", false, 1, 6);
    delayMixKnob     = &addKnob("delayMix",      "MIX",       "amber", false, 1, 6);

    // FX 7: Shimmer / Hall Reverb
    reverbEnableBtn   = &addBypassToggle("reverbEnable", "ON", "emerald", 1, 7);
    reverbSizeKnob    = &addKnob("reverbSize",     "SIZE",    "cyan",    false, 1, 7);
    reverbDecayKnob   = &addKnob("reverbDecay",    "DECAY",   "cyan",    false, 1, 7);
    reverbPreDelayKnob= &addKnob("reverbPreDelay", "PRE-DLY", "amber",   false, 1, 7);
    reverbDampingKnob = &addKnob("reverbDamping",  "DAMPING", "amber",   false, 1, 7);
    reverbShimmerKnob = &addKnob("reverbShimmer",  "SHIMMER", "magenta", false, 1, 7);
    reverbMixKnob     = &addKnob("reverbMix",      "MIX",     "cyan",    false, 1, 7);

    // FX 8: Multiband OTT / Single-Band Compressor
    compEnableBtn     = &addBypassToggle("compEnable", "ON", "cyan", 1, 8);
    compModeBox.addItemList({ "SINGLE VCA", "MULTIBAND OTT", "OPTO SMOOTH", "PUNCH LIMIT" }, 1);
    compModeBox.getProperties().set("accent", "cyan");
    addChildComponent(compModeBox);
    compModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "compMode", compModeBox);

    compThresholdKnob = &addKnob("compThreshold", "THRESH",  "cyan",    false, 1, 8);
    compRatioKnob     = &addKnob("compRatio",     "RATIO",   "cyan",    false, 1, 8);
    compAttackKnob    = &addKnob("compAttack",    "ATTACK",  "amber",   false, 1, 8);
    compReleaseKnob   = &addKnob("compRelease",   "RELEASE", "amber",   false, 1, 8);
    compMakeupKnob    = &addKnob("compMakeup",    "MAKEUP",  "magenta", false, 1, 8);
    compMixKnob       = &addKnob("compMix",       "MIX",     "emerald", false, 1, 8);

    // FX 9: Zenology Modulation Pack (Ring Mod / Freq Shifter / Auto-Pan / Rotary Cab)
    ringEnableBtn = &addBypassToggle("ringEnable", "ON", "magenta", 1, 9);
    ringModeBox.addItemList({ "RING MOD", "FREQ SHIFTER", "STEREO AUTO-PAN", "ROTARY SPEAKER" }, 1);
    ringModeBox.getProperties().set("accent", "magenta");
    addChildComponent(ringModeBox);
    ringModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "ringMode", ringModeBox);

    ringFreqKnob   = &addKnob("ringFreq",   "FREQ Hz", "magenta", false, 1, 9);
    ringDepthKnob  = &addKnob("ringDepth",  "DEPTH",   "magenta", false, 1, 9);
    ringRateKnob   = &addKnob("ringRate",   "LFO SPD", "amber",   false, 1, 9);
    ringSpreadKnob = &addKnob("ringSpread", "SPREAD",  "cyan",    false, 1, 9);
    ringMixKnob    = &addKnob("ringMix",    "MIX",     "magenta", false, 1, 9);

    // FX 10: Zenology Lo-Fi Pack (Bitcrusher / Sample Decimator / SP-1200 / Radio)
    crushEnableBtn = &addBypassToggle("crushEnable", "ON", "amber", 1, 10);
    crushModeBox.addItemList({ "DIGITAL CRUSH", "DECIMATOR", "SP-1200 VINTAGE", "RADIO LO-FI" }, 1);
    crushModeBox.getProperties().set("accent", "amber");
    addChildComponent(crushModeBox);
    crushModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "crushMode", crushModeBox);

    crushBitsKnob       = &addKnob("crushBits",       "BITS",    "amber",   false, 1, 10);
    crushDownsampleKnob = &addKnob("crushDownsample", "DOWNSMP", "amber",   false, 1, 10);
    crushJitterKnob     = &addKnob("crushJitter",     "JITTER",  "magenta", false, 1, 10);
    crushFilterKnob     = &addKnob("crushFilter",     "TONE",    "cyan",    false, 1, 10);
    crushMixKnob        = &addKnob("crushMix",        "MIX",     "amber",   false, 1, 10);

    // FX 11: Zenology Flanger Pack (Jet / Through-Zero / Barberpole / Step Flanger)
    flangerEnableBtn = &addBypassToggle("flangerEnable", "ON", "cyan", 1, 11);
    flangerModeBox.addItemList({ "JET FLANGER", "THROUGH-ZERO", "BARBERPOLE", "STEP FLANGER" }, 1);
    flangerModeBox.getProperties().set("accent", "cyan");
    addChildComponent(flangerModeBox);
    flangerModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "flangerMode", flangerModeBox);

    flangerRateKnob     = &addKnob("flangerRate",     "RATE",    "cyan",    false, 1, 11);
    flangerDepthKnob    = &addKnob("flangerDepth",    "DEPTH",   "cyan",    false, 1, 11);
    flangerFeedbackKnob = &addKnob("flangerFeedback", "FEEDBCK", "magenta", true,  1, 11);
    flangerSpreadKnob   = &addKnob("flangerSpread",   "SPREAD",  "amber",   false, 1, 11);
    flangerMixKnob      = &addKnob("flangerMix",      "MIX",     "cyan",    false, 1, 11);

    // -------------------------------------------------------------------------
    // TIME MODULATION TAB CONTROLS (tabGroup = 4)
    // -------------------------------------------------------------------------
    addChildComponent(timeModEditor);

    timeModEnableBtn = &addBypassToggle("timeModEnable", "TIME MOD ON", "emerald", 4);

    timeModModeBox.addItemList({
        "TIME WARP CURVE", "STUTTER REPEAT", "TAPE STOP",
        "REVERSE GRAIN", "HALF-TIME 1/2x", "DJ SCRATCH"
    }, 1);
    timeModModeBox.getProperties().set("accent", "amber");
    addChildComponent(timeModModeBox);
    timeModModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "timeModMode", timeModModeBox);

    timeModRateBox.addItemList({ "2 BARS", "1 BAR", "1/2", "1/4", "1/8", "1/16" }, 1);
    timeModRateBox.getProperties().set("accent", "cyan");
    addChildComponent(timeModRateBox);
    timeModRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "timeModRate", timeModRateBox);

    timeModDepthKnob   = &addKnob("timeModDepth",   "WARP DEPTH", "amber",   false, 4);
    timeModStutterKnob = &addKnob("timeModStutter", "STUTTER",    "cyan",    false, 4);
    timeModSpeedKnob   = &addKnob("timeModSpeed",   "TAPE SPEED", "magenta", false, 4);
    timeModSmoothKnob  = &addKnob("timeModSmooth",  "SMOOTHING",  "cyan",    false, 4);
    timeModFilterKnob  = &addKnob("timeModFilter",  "LP / HP",    "amber",   true,  4);
    timeModMixKnob     = &addKnob("timeModMix",     "WET MIX",    "emerald", false, 4);

    timeModEditor.onTimePresetSelected = [this](int padIdx)
    {
        if (auto* pEnable = audioProcessor.getAPVTS().getParameter("timeModEnable"))
            pEnable->setValueNotifyingHost(1.0f);

        // Map pattern pad to appropriate Time Mod engine mode
        int targetMode = 0; // Time Warp Curve
        if (padIdx == 0) targetMode = 4;      // Half-Time 1/2x
        else if (padIdx == 1) targetMode = 1; // 1/8 Stutter
        else if (padIdx == 2) targetMode = 2; // Tape Stop
        else if (padIdx == 3) targetMode = 3; // Reverse Bar
        else if (padIdx == 4) targetMode = 5; // DJ Scratch
        timeModModeBox.setSelectedItemIndex(targetMode, juce::sendNotificationSync);
        repaint();
    };

    // -------------------------------------------------------------------------
    // DEDICATED PRESET BROWSER TAB (currentTab == 3)
    // -------------------------------------------------------------------------
    addChildComponent(presetBrowserView);
    presetBrowserView.onPresetLoaded = [this](int idx)
    {
        presetSelector.setSelectedItemIndex(idx, juce::dontSendNotification);
        syncWaveSelectorsFromEngine();
        articulatorEditor.repaint();
        arpStepGrid.repaint();
        timeModEditor.repaint();
        visualizer3D.repaint();
        updateFxControlsVisibility();
        resized();
        repaint();
    };

    // -------------------------------------------------------------------------
    // BOTTOM PANEL (Always Visible: tabGroup = 2, Arp = 3, AHDSR = 5)
    // -------------------------------------------------------------------------
    auto updateABTabStyles = [this](int activePart)
    {
        partAButton.setColour(juce::TextButton::buttonColourId,
            activePart == 0 ? Palette::electricCyan.withAlpha(0.28f) : Palette::trackDark);
        partAButton.setColour(juce::TextButton::textColourOffId,
            activePart == 0 ? Palette::electricCyan : Palette::textSecondary);

        partBButton.setColour(juce::TextButton::buttonColourId,
            activePart == 1 ? Palette::deepMagenta.withAlpha(0.28f) : Palette::trackDark);
        partBButton.setColour(juce::TextButton::textColourOffId,
            activePart == 1 ? Palette::deepMagenta : Palette::textSecondary);

        copyABButton.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
        copyABButton.setColour(juce::TextButton::textColourOffId, Palette::energeticAmber);
    };

    updateABTabStyles(0);

    visualizer3D.onActivePartChanged = [this, updateABTabStyles](int newPart)
    {
        updateABTabStyles(newPart);
        repaint();
    };

    partAButton.onClick = [this, updateABTabStyles]()
    {
        visualizer3D.setActiveEditPart(0);
        updateABTabStyles(0);
        repaint();
    };
    partBButton.onClick = [this, updateABTabStyles]()
    {
        visualizer3D.setActiveEditPart(1);
        updateABTabStyles(1);
        repaint();
    };
    copyABButton.onClick = [this]()
    {
        const int src = visualizer3D.getActiveEditPart();
        const int dst = 1 - src;
        audioProcessor.getSynthEngine().getResynthesisEngine().copyPartTable(src, dst);
        syncWaveSelectorsFromEngine();
        visualizer3D.repaint();
        repaint();
    };

    addAndMakeVisible(partAButton);
    addAndMakeVisible(partBButton);
    addAndMakeVisible(copyABButton);

    abMixFader  = &addHorizontalFader("abMix", "A / B CROSSFADE", "cyan", true, 2);
    abMixFader->onValueChange = [this]() { repaint(cardABSystem); };

    velLinkKnob = &addKnob("velLink", "VEL LINK", "cyan",    false, 2);
    partPanKnob = &addKnob("partPan", "PANNING",  "magenta", true,  2);

    // Card 09 Mode Switcher: Shaper Articulator (+ AHDSR) vs Arpeggiator & 16-Step Sequencer
    modeArticulatorBtn.onClick = [this]() { setBottomCenterMode(0); };
    modeArpSeqBtn.onClick      = [this]() { setBottomCenterMode(1); };
    addAndMakeVisible(modeArticulatorBtn);
    addAndMakeVisible(modeArpSeqBtn);

    addAndMakeVisible(articulatorEditor);
    addChildComponent(arpStepGrid);

    // Serum-2 Style Physical AHDSR Knobs for the Graphical Articulator (tabGroup = 5)
    // Strict RAPID Warm Gold / Amber Color Coding (#E5A43B)
    envAttackKnob  = &addKnob("envAttack",  "ATTACK",  "amber", false, 5);
    envHoldKnob    = &addKnob("envHold",    "HOLD",    "amber", false, 5);
    envDecayKnob   = &addKnob("envDecay",   "DECAY",   "amber", false, 5);
    envSustainKnob = &addKnob("envSustain", "SUSTAIN", "amber", false, 5);
    envReleaseKnob = &addKnob("envRelease", "RELEASE", "amber", false, 5);

    auto bindAHDSRKnob = [this](juce::Slider* knob)
    {
        knob->onDragStart = [this]()
        {
            audioProcessor.getSynthEngine().getArticulator(articulatorEditor.getTarget()).applyAHDSRParameters(
                static_cast<float>(envAttackKnob->getValue()),
                static_cast<float>(envHoldKnob->getValue()),
                static_cast<float>(envDecayKnob->getValue()),
                static_cast<float>(envSustainKnob->getValue()),
                static_cast<float>(envReleaseKnob->getValue()));
            articulatorEditor.repaint();
        };
        knob->onValueChange = [this]()
        {
            if (envAttackKnob == nullptr || envHoldKnob == nullptr || envDecayKnob == nullptr
                || envSustainKnob == nullptr || envReleaseKnob == nullptr)
                return;
            if (envAttackKnob->isMouseButtonDown() || envHoldKnob->isMouseButtonDown()
                || envDecayKnob->isMouseButtonDown() || envSustainKnob->isMouseButtonDown()
                || envReleaseKnob->isMouseButtonDown())
            {
                audioProcessor.getSynthEngine().getArticulator(articulatorEditor.getTarget()).applyAHDSRParameters(
                    static_cast<float>(envAttackKnob->getValue()),
                    static_cast<float>(envHoldKnob->getValue()),
                    static_cast<float>(envDecayKnob->getValue()),
                    static_cast<float>(envSustainKnob->getValue()),
                    static_cast<float>(envReleaseKnob->getValue()));
                articulatorEditor.repaint();
            }
        };
    };
    for (auto* k : { envAttackKnob, envHoldKnob, envDecayKnob, envSustainKnob, envReleaseKnob })
        bindAHDSRKnob(k);

    arpEnableBtn = &addBypassToggle("arpEnable", "ARP ON", "emerald", 2);
    arpModeBox.addItemList({ "UP", "DOWN", "UP / DOWN", "RANDOM", "CHORD", "STEP SEQ" }, 1);
    arpModeBox.getProperties().set("accent", "cyan");
    addChildComponent(arpModeBox);
    arpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arpMode", arpModeBox);

    arpRateBox.addItemList({ "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 1);
    arpRateBox.getProperties().set("accent", "amber");
    addChildComponent(arpRateBox);
    arpRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "arpRate", arpRateBox);

    arpOctavesKnob = &addKnob("arpOctaves", "OCTAVES", "cyan",    false, 3);
    arpGateKnob    = &addKnob("arpGate",    "GATE",    "amber",   false, 3);
    arpSwingKnob   = &addKnob("arpSwing",   "SWING",   "magenta", false, 3);

    auto styleNavBtn = [](juce::TextButton& b, juce::Colour col)
    {
        b.setColour(juce::TextButton::buttonColourId, Palette::panelRecessed);
        b.setColour(juce::TextButton::textColourOffId, col);
    };
    styleNavBtn(prevPresetBtn, Palette::iceBlue);
    styleNavBtn(nextPresetBtn, Palette::iceBlue);
    styleNavBtn(initPresetBtn, Palette::goldOrange);

    presetSelector.addItemList(audioProcessor.getFactoryPresetNames(), 1);
    presetSelector.setSelectedItemIndex(audioProcessor.getCurrentPresetIndex(), juce::dontSendNotification);
    presetSelector.getProperties().set("isPresetSlot", true);
    presetSelector.onChange = [this]()
    {
        const int idx = presetSelector.getSelectedItemIndex();
        if (idx >= 0)
        {
            audioProcessor.loadFactoryPreset(idx);
            syncWaveSelectorsFromEngine();
            articulatorEditor.repaint();
            arpStepGrid.repaint();
            timeModEditor.repaint();
            presetBrowserView.repaint();
            visualizer3D.repaint();
            updateFxControlsVisibility();
            resized();
            repaint();
        }
    };

    prevPresetBtn.onClick = [this]()
    {
        const int num = presetSelector.getNumItems();
        const int nextIdx = (presetSelector.getSelectedItemIndex() - 1 + num) % num;
        presetSelector.setSelectedItemIndex(nextIdx, juce::sendNotificationSync);
    };
    nextPresetBtn.onClick = [this]()
    {
        const int num = presetSelector.getNumItems();
        const int nextIdx = (presetSelector.getSelectedItemIndex() + 1) % num;
        presetSelector.setSelectedItemIndex(nextIdx, juce::sendNotificationSync);
    };
    initPresetBtn.onClick = [this]()
    {
        presetSelector.setSelectedItemIndex(0, juce::sendNotificationSync);
    };

    addAndMakeVisible(prevPresetBtn);
    addAndMakeVisible(nextPresetBtn);
    addAndMakeVisible(initPresetBtn);
    addAndMakeVisible(presetSelector);

    strumTimeKnob     = &addKnob("strumTime",       "STRUM TIME",  "cyan",    false, 2);
    strumTensionKnob  = &addKnob("strumTension",    "STRUM SLOPE", "cyan",    true,  2);
    masterGainKnob    = &addKnob("masterGainDb",    "MASTER dB",   "amber",   true,  2);
    limiterThreshKnob = &addKnob("limiterThreshDb", "CEILING dB",  "magenta", false, 2);
    limiterEnableBtn  = &addBypassToggle("limiterEnable", "LIMITER", "emerald", 2);

    addAndMakeVisible(visualizer3D);

    syncWaveSelectorsFromEngine();
    setSize(1240, 800);
    setBottomCenterMode(0);
    setActiveTab(0);
    startTimerHz(60);
}

PrismorAudioProcessorEditor::~PrismorAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PrismorAudioProcessorEditor::syncWaveSelectorsFromEngine()
{
    const auto& resynth = audioProcessor.getSynthEngine().getResynthesisEngine();
    oscAWaveBox.setSelectedItemIndex(resynth.getActiveWaveType(0), juce::dontSendNotification);
    oscBWaveBox.setSelectedItemIndex(resynth.getActiveWaveType(1), juce::dontSendNotification);
}

void PrismorAudioProcessorEditor::applyValhallaTheme(int themeIdx)
{
    Palette::applyTheme(themeIdx);
    rapidLookAndFeel.refreshThemeColours();

    const int activePart = visualizer3D.getActiveEditPart();
    partAButton.setColour(juce::TextButton::buttonColourId,
        activePart == 0 ? Palette::electricCyan.withAlpha(0.28f) : Palette::trackDark);
    partAButton.setColour(juce::TextButton::textColourOffId,
        activePart == 0 ? Palette::electricCyan : Palette::textSecondary);

    partBButton.setColour(juce::TextButton::buttonColourId,
        activePart == 1 ? Palette::deepMagenta.withAlpha(0.28f) : Palette::trackDark);
    partBButton.setColour(juce::TextButton::textColourOffId,
        activePart == 1 ? Palette::deepMagenta : Palette::textSecondary);

    copyABButton.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
    copyABButton.setColour(juce::TextButton::textColourOffId, Palette::energeticAmber);

    setActiveTab(currentTab);
    setBottomCenterMode(bottomCenterMode);

    prevPresetBtn.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
    prevPresetBtn.setColour(juce::TextButton::textColourOffId, Palette::electricCyan);
    nextPresetBtn.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
    nextPresetBtn.setColour(juce::TextButton::textColourOffId, Palette::electricCyan);
    initPresetBtn.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
    initPresetBtn.setColour(juce::TextButton::textColourOffId, Palette::energeticAmber);

    sendLookAndFeelChange();
    visualizer3D.repaint();
    articulatorEditor.repaint();
    arpStepGrid.repaint();
    timeModEditor.repaint();
    presetBrowserView.repaint();
    repaint();
}

void PrismorAudioProcessorEditor::setBottomCenterMode(int modeIdx)
{
    bottomCenterMode = juce::jlimit(0, 1, modeIdx);

    modeArticulatorBtn.setColour(juce::TextButton::buttonColourId,
        bottomCenterMode == 0 ? Palette::energeticAmber.withAlpha(0.28f) : Palette::trackDark);
    modeArticulatorBtn.setColour(juce::TextButton::textColourOffId,
        bottomCenterMode == 0 ? Palette::energeticAmber : Palette::textSecondary);

    modeArpSeqBtn.setColour(juce::TextButton::buttonColourId,
        bottomCenterMode == 1 ? Palette::electricCyan.withAlpha(0.28f) : Palette::trackDark);
    modeArpSeqBtn.setColour(juce::TextButton::textColourOffId,
        bottomCenterMode == 1 ? Palette::electricCyan : Palette::textSecondary);

    const bool isArt = (bottomCenterMode == 0);
    const bool isArp = (bottomCenterMode == 1);

    articulatorEditor.setVisible(isArt);
    arpStepGrid.setVisible(isArp);
    arpModeBox.setVisible(isArp);
    arpRateBox.setVisible(isArp);

    for (auto& c : paramControls)
    {
        if (c->tabGroup == 3)
            c->slider->setVisible(isArp);
        else if (c->tabGroup == 5)
            c->slider->setVisible(isArt);
    }

    resized();
    repaint();
}

void PrismorAudioProcessorEditor::updateFxControlsVisibility()
{
    const bool isFxTab = (currentTab == 1);
    const auto activeChain = audioProcessor.getSynthEngine().getActiveFxChain();

    auto isModuleInActiveChain = [&](int modId) -> bool
    {
        for (int m : activeChain)
            if (m == modId) return true;
        return false;
    };

    for (auto& c : paramControls)
    {
        if (c->tabGroup == 1)
            c->slider->setVisible(isFxTab && isModuleInActiveChain(c->fxModuleId));
    }
    for (auto& t : toggleControls)
    {
        if (t->tabGroup == 1)
            t->button->setVisible(isFxTab && isModuleInActiveChain(t->fxModuleId) && t->button.get() != pluckEnableBtn);
    }

    const bool distVis = isFxTab && isModuleInActiveChain(2);
    distTypeBox.setVisible(distVis);
    distSubPresetBox.setVisible(distVis);

    const bool compVis = isFxTab && isModuleInActiveChain(8);
    compModeBox.setVisible(compVis);

    const bool ringVis = isFxTab && isModuleInActiveChain(9);
    ringModeBox.setVisible(ringVis);

    const bool crushVis = isFxTab && isModuleInActiveChain(10);
    crushModeBox.setVisible(crushVis);

    const bool flangerVis = isFxTab && isModuleInActiveChain(11);
    flangerModeBox.setVisible(flangerVis);

    tabFxBtn.setButtonText("FX RACK [" + juce::String(activeChain.size()) + "]");
}

void PrismorAudioProcessorEditor::setActiveTab(int tabIdx)
{
    currentTab = juce::jlimit(0, 3, tabIdx);

    tabSynthBtn.getProperties().set("tabActive", currentTab == 0);
    tabFxBtn.getProperties().set("tabActive", currentTab == 1);
    tabTimeModBtn.getProperties().set("tabActive", currentTab == 2);
    tabPresetsBtn.getProperties().set("tabActive", currentTab == 3);

    tabSynthBtn.setColour(juce::TextButton::buttonColourId,
        currentTab == 0 ? Palette::panelElevated : Palette::panelDark);
    tabSynthBtn.setColour(juce::TextButton::textColourOffId,
        currentTab == 0 ? Palette::textPrimary : Palette::textSecondary);

    tabFxBtn.setColour(juce::TextButton::buttonColourId,
        currentTab == 1 ? Palette::panelElevated : Palette::panelDark);
    tabFxBtn.setColour(juce::TextButton::textColourOffId,
        currentTab == 1 ? Palette::textPrimary : Palette::textSecondary);

    tabTimeModBtn.setColour(juce::TextButton::buttonColourId,
        currentTab == 2 ? Palette::panelElevated : Palette::panelDark);
    tabTimeModBtn.setColour(juce::TextButton::textColourOffId,
        currentTab == 2 ? Palette::textPrimary : Palette::textSecondary);

    tabPresetsBtn.setColour(juce::TextButton::buttonColourId,
        currentTab == 3 ? Palette::panelElevated : Palette::panelDark);
    tabPresetsBtn.setColour(juce::TextButton::textColourOffId,
        currentTab == 3 ? Palette::textPrimary : Palette::textSecondary);

    tabSynthBtn.repaint();
    tabFxBtn.repaint();
    tabTimeModBtn.repaint();
    tabPresetsBtn.repaint();

    const bool isSynth   = (currentTab == 0);
    const bool isTimeMod = (currentTab == 2);
    const bool isPresets = (currentTab == 3);

    for (auto& c : paramControls)
    {
        if (c->tabGroup == 0)
            c->slider->setVisible(isSynth);
        else if (c->tabGroup == 2)
            c->slider->setVisible(true);
        else if (c->tabGroup == 3)
            c->slider->setVisible(bottomCenterMode == 1);
        else if (c->tabGroup == 4)
            c->slider->setVisible(isTimeMod);
        else if (c->tabGroup == 5)
            c->slider->setVisible(bottomCenterMode == 0);
    }

    for (auto& t : toggleControls)
    {
        if (t->tabGroup == 0)
            t->button->setVisible(isSynth);
        else if (t->tabGroup == 2)
            t->button->setVisible(true);
        else if (t->tabGroup == 4)
            t->button->setVisible(isTimeMod);
    }

    oscAWaveBox.setVisible(isSynth);
    oscBWaveBox.setVisible(isSynth);
    warpModeBox.setVisible(isSynth);
    warpModeBBox.setVisible(isSynth);
    prismModeBox.setVisible(isSynth);
    voicingModeBox.setVisible(isSynth);
    unisonStackModeBox.setVisible(isSynth);
    filterSubPresetBox.setVisible(isSynth);
    filter1TypeBox.setVisible(isSynth);
    filter2TypeBox.setVisible(isSynth);
    noiseTypeBox.setVisible(isSynth);
    visualizer3D.setVisible(isSynth);

    timeModEditor.setVisible(isTimeMod);
    timeModModeBox.setVisible(isTimeMod);
    timeModRateBox.setVisible(isTimeMod);

    presetBrowserView.setVisible(isPresets);

    updateFxControlsVisibility();

    resized();
    repaint();
}

void PrismorAudioProcessorEditor::showAddFxMenu()
{
    static const char* fxMenuNames[12] = {
        "Spectral Blur & Pluck",
        "Harmonic Tremolo",
        "Distortion / Wavefolder",
        "Spectral Phaser",
        "Multi-Voice Chorus",
        "3-Band Parametric EQ",
        "Ping-Pong Delay",
        "Shimmer Hall Reverb",
        "Multiband OTT / VCA Compressor",
        "Zenology Modulation Pack (Ring Mod / Shifter)",
        "Zenology Lo-Fi Pack (Bitcrusher / SP-1200)",
        "Zenology Flanger Pack (Jet / Thru-Zero)"
    };
    static const char* fxEnableParamIds[12] = {
        "blurEnable", "tremEnable", "distEnable", "phaserEnable",
        "chorusEnable", "eqEnable", "delayEnable", "reverbEnable",
        "compEnable", "ringEnable", "crushEnable", "flangerEnable"
    };

    const auto activeChain = audioProcessor.getSynthEngine().getActiveFxChain();
    juce::PopupMenu menu;
    menu.addSectionHeader("ADD EFFECT TO DYNAMIC 12-SLOT FX CHAIN");

    for (int modId = 0; modId < 12; ++modId)
    {
        bool alreadyActive = false;
        for (int m : activeChain)
            if (m == modId) { alreadyActive = true; break; }

        menu.addItem(modId + 1,
                     juce::String(fxMenuNames[modId]) + (alreadyActive ? " [ACTIVE]" : ""),
                     !alreadyActive,
                     alreadyActive);
    }

    menu.addSeparator();
    menu.addItem(100, "Restore All 12 FX Modules (Default Chain)");

    menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result)
    {
        if (result == 100)
        {
            audioProcessor.getSynthEngine().resetFxChainOrder();
            updateFxControlsVisibility();
            resized();
            repaint();
        }
        else if (result >= 1 && result <= 12)
        {
            const int modId = result - 1;
            if (audioProcessor.getSynthEngine().addFxModule(modId))
            {
                if (auto* p = audioProcessor.getAPVTS().getParameter(fxEnableParamIds[modId]))
                    p->setValueNotifyingHost(1.0f);

                selectedFxSlot = std::max(0, static_cast<int>(audioProcessor.getSynthEngine().getActiveFxChain().size()) - 1);
                updateFxControlsVisibility();
                resized();
                repaint();
            }
        }
    });
}

juce::Slider& PrismorAudioProcessorEditor::addKnob(
    const juce::String& paramId,
    const juce::String& labelText,
    const juce::String& accentColor,
    bool isBipolar,
    int tabGroup,
    int fxModuleId)
{
    auto ctrl = std::make_unique<ParamControl>();
    ctrl->title = labelText;
    ctrl->tabGroup = tabGroup;
    ctrl->fxModuleId = fxModuleId;
    ctrl->slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    ctrl->slider->setName(paramId);
    ctrl->slider->getProperties().set("paramId", paramId);
    ctrl->slider->getProperties().set("accent", accentColor);
    ctrl->slider->getProperties().set("bipolar", isBipolar);

    auto* rawSlider = ctrl->slider.get();
    addAndMakeVisible(*ctrl->slider);
    ctrl->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), paramId, *ctrl->slider);

    paramControls.push_back(std::move(ctrl));
    return *rawSlider;
}

juce::Slider& PrismorAudioProcessorEditor::addPitchBox(
    const juce::String& paramId,
    const juce::String& boxTitle,
    const juce::String& boxSuffix,
    int decimals,
    const juce::String& accentColor,
    int tabGroup)
{
    auto ctrl = std::make_unique<ParamControl>();
    ctrl->title = {};
    ctrl->tabGroup = tabGroup;
    ctrl->slider = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox);
    ctrl->slider->getProperties().set("pitchBox", true);
    ctrl->slider->getProperties().set("boxTitle", boxTitle);
    ctrl->slider->getProperties().set("boxSuffix", boxSuffix);
    ctrl->slider->getProperties().set("boxDecimals", decimals);
    ctrl->slider->getProperties().set("accent", accentColor);

    auto* rawSlider = ctrl->slider.get();
    addAndMakeVisible(*ctrl->slider);
    ctrl->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), paramId, *ctrl->slider);

    paramControls.push_back(std::move(ctrl));
    return *rawSlider;
}

juce::Slider& PrismorAudioProcessorEditor::addVerticalFader(
    const juce::String& paramId,
    const juce::String& labelText,
    const juce::String& accentColor,
    int tabGroup)
{
    auto ctrl = std::make_unique<ParamControl>();
    ctrl->title = labelText;
    ctrl->tabGroup = tabGroup;
    ctrl->slider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::NoTextBox);
    ctrl->slider->getProperties().set("accent", accentColor);

    auto* rawSlider = ctrl->slider.get();
    addAndMakeVisible(*ctrl->slider);
    ctrl->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), paramId, *ctrl->slider);

    paramControls.push_back(std::move(ctrl));
    return *rawSlider;
}

juce::Slider& PrismorAudioProcessorEditor::addHorizontalFader(
    const juce::String& paramId,
    const juce::String& labelText,
    const juce::String& accentColor,
    bool isABCrossfader,
    int tabGroup)
{
    auto ctrl = std::make_unique<ParamControl>();
    ctrl->title = labelText;
    ctrl->tabGroup = tabGroup;
    ctrl->slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    ctrl->slider->getProperties().set("accent", accentColor);
    ctrl->slider->getProperties().set("abCrossfader", isABCrossfader);

    auto* rawSlider = ctrl->slider.get();
    addAndMakeVisible(*ctrl->slider);
    ctrl->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), paramId, *ctrl->slider);

    paramControls.push_back(std::move(ctrl));
    return *rawSlider;
}

juce::ToggleButton& PrismorAudioProcessorEditor::addBypassToggle(
    const juce::String& paramId,
    const juce::String& buttonText,
    const juce::String& accentColor,
    int tabGroup,
    int fxModuleId)
{
    auto tc = std::make_unique<ToggleControl>();
    tc->tabGroup = tabGroup;
    tc->fxModuleId = fxModuleId;
    tc->button = std::make_unique<juce::ToggleButton>(buttonText);
    tc->button->getProperties().set("accent", accentColor);

    auto* rawBtn = tc->button.get();
    addAndMakeVisible(*tc->button);
    tc->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), paramId, *tc->button);

    toggleControls.push_back(std::move(tc));
    return *rawBtn;
}

void PrismorAudioProcessorEditor::layoutControlWithLabel(juce::Slider& slider, juce::Rectangle<int> cell, int labelHeight)
{
    for (auto& c : paramControls)
    {
        if (c->slider.get() == &slider)
        {
            if (slider.getProperties()["pitchBox"])
            {
                c->labelBounds = {};
                slider.setBounds(cell);
                return;
            }
            c->labelBounds = cell.removeFromBottom(labelHeight);
            slider.setBounds(cell);
            return;
        }
    }
    slider.setBounds(cell);
}

void PrismorAudioProcessorEditor::timerCallback()
{
    noiseAnimPhase += 0.14f;
    auto snap = audioProcessor.getSynthEngine().getLatestVisualizerSnapshot();

    const juce::String exportPath      = juce::SystemStats::getEnvironmentVariable("PRISMOR_EXPORT_UI_PNG", {});
    const juce::String exportFxPath    = juce::SystemStats::getEnvironmentVariable("PRISMOR_EXPORT_FX_PNG", {});
    const juce::String exportTimePath  = juce::SystemStats::getEnvironmentVariable("PRISMOR_EXPORT_TIMEMOD_PNG", {});
    const juce::String exportPresetPath= juce::SystemStats::getEnvironmentVariable("PRISMOR_EXPORT_PRESETS_PNG", {});
    const juce::String exportTheme     = juce::SystemStats::getEnvironmentVariable("PRISMOR_EXPORT_THEME", {});

    if (exportPath.isNotEmpty() || exportFxPath.isNotEmpty() || exportTimePath.isNotEmpty() || exportPresetPath.isNotEmpty())
    {
        ++screenshotFrameCounter;
        if (screenshotFrameCounter == 1)
        {
            if (exportTheme.isNotEmpty())
                themeSelector.setSelectedItemIndex(exportTheme.getIntValue(), juce::sendNotificationSync);
            presetSelector.setSelectedItemIndex(4, juce::sendNotificationSync); // 04: Zenology Hard-Sync Lead
        }
        if (screenshotFrameCounter == 3)
        {
            audioProcessor.getKeyboardState().noteOn(1, 48, 0.92f);
            audioProcessor.getKeyboardState().noteOn(1, 55, 0.88f);
            audioProcessor.getKeyboardState().noteOn(1, 60, 0.85f);
        }
        if (!snap.voiceActive)
        {
            snap.voiceActive = true;
            snap.activePitchHz = 130.81f;
            snap.activeVoiceCount = 4;
            snap.peakLeft = 0.78f;
            snap.peakRight = 0.74f;
            snap.rmsLeft = 0.56f;
            snap.rmsRight = 0.53f;
            snap.limiterGainReductionDb = 1.8f;
            snap.envCursor = std::fmod(static_cast<float>(screenshotFrameCounter) * 0.03f, 0.85f);
            snap.timeModCursor = std::fmod(static_cast<float>(screenshotFrameCounter) * 0.025f, 1.0f);
            snap.timeModOffsetNorm = 0.35f;
            for (int t = 0; t < 6; ++t)
                snap.envCursors[static_cast<size_t>(t)] = snap.envCursor;
            for (int k = 0; k < PrismorDSP::VisualizerSnapshot::NUM_BINS; ++k)
            {
                const float norm = static_cast<float>(k) / static_cast<float>(PrismorDSP::VisualizerSnapshot::NUM_BINS - 1);
                snap.freqNormA[static_cast<size_t>(k)] = norm;
                snap.freqNormB[static_cast<size_t>(k)] = norm;
                const float harmonicPeak = 0.55f + 0.45f * std::sin(norm * 18.0f - static_cast<float>(screenshotFrameCounter) * 0.22f);
                const float rolloff = std::pow(1.0f - norm * 0.72f, 1.35f);
                snap.ampA[static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, rolloff * harmonicPeak * snap.filterCurve[static_cast<size_t>(k)] * 1.35f);
                snap.ampB[static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, rolloff * (1.0f - 0.5f * harmonicPeak) * snap.filterCurve[static_cast<size_t>(k)] * 1.15f);
            }
        }
        auto saveSnapshotToFile = [this](const juce::String& filePath)
        {
            if (filePath.isEmpty()) return;
            juce::Image img = createComponentSnapshot(getLocalBounds());
            juce::File outFile(filePath);
            outFile.deleteFile();
            juce::FileOutputStream stream(outFile);
            if (stream.openedOk())
            {
                juce::PNGImageFormat png;
                png.writeImageToStream(img, stream);
            }
        };

        if (screenshotFrameCounter == 18 && exportPath.isNotEmpty())
        {
            setActiveTab(0);
            setBottomCenterMode(0);
            visualizer3D.set2DViewMode(false);
            repaint();
        }
        if (screenshotFrameCounter == 22 && exportPath.isNotEmpty())
        {
            saveSnapshotToFile(exportPath);
            if (exportFxPath.isEmpty() && exportTimePath.isEmpty() && exportPresetPath.isEmpty())
                juce::JUCEApplicationBase::quit();
        }
        if (screenshotFrameCounter == 24 && exportFxPath.isNotEmpty())
        {
            setActiveTab(1);
            repaint();
        }
        if (screenshotFrameCounter == 28 && exportFxPath.isNotEmpty())
        {
            saveSnapshotToFile(exportFxPath);
            if (exportTimePath.isEmpty() && exportPresetPath.isEmpty())
                juce::JUCEApplicationBase::quit();
        }
        if (screenshotFrameCounter == 30 && exportTimePath.isNotEmpty())
        {
            presetSelector.setSelectedItemIndex(28, juce::sendNotificationSync); // 28: GrossBeat Halftime Pad
            setActiveTab(2);
            repaint();
        }
        if (screenshotFrameCounter == 34 && exportTimePath.isNotEmpty())
        {
            saveSnapshotToFile(exportTimePath);
            if (exportPresetPath.isEmpty())
                juce::JUCEApplicationBase::quit();
        }
        if (screenshotFrameCounter == 36 && exportPresetPath.isNotEmpty())
        {
            setActiveTab(3);
            repaint();
        }
        if (screenshotFrameCounter == 40 && exportPresetPath.isNotEmpty())
        {
            saveSnapshotToFile(exportPresetPath);
            juce::JUCEApplicationBase::quit();
        }
    }

    smoothedPeakL = (snap.peakLeft > smoothedPeakL)  ? snap.peakLeft  : (smoothedPeakL * 0.88f + snap.peakLeft * 0.12f);
    smoothedPeakR = (snap.peakRight > smoothedPeakR) ? snap.peakRight : (smoothedPeakR * 0.88f + snap.peakRight * 0.12f);
    smoothedRmsL  = smoothedRmsL * 0.85f + snap.rmsLeft * 0.15f;
    smoothedRmsR  = smoothedRmsR * 0.85f + snap.rmsRight * 0.15f;
    smoothedGrDb  = (snap.limiterGainReductionDb > smoothedGrDb)
        ? snap.limiterGainReductionDb
        : (smoothedGrDb * 0.86f + snap.limiterGainReductionDb * 0.14f);

    // Dynamic CPU & Active Voices Telemetry
    const float liveCpu = juce::jlimit(0.0f, 100.0f, audioProcessor.getCpuUsagePercent());
    if (liveCpu > smoothedCpu)
        smoothedCpu = smoothedCpu * 0.55f + liveCpu * 0.45f;
    else
        smoothedCpu = smoothedCpu * 0.90f + liveCpu * 0.10f;

    int liveVoices = audioProcessor.getSynthEngine().getActiveVoiceCount();

    if (exportPath.isNotEmpty() || exportFxPath.isNotEmpty() || exportTimePath.isNotEmpty() || exportPresetPath.isNotEmpty())
    {
        if (liveVoices == 0 && snap.activeVoiceCount > 0)
            liveVoices = snap.activeVoiceCount;
        if (smoothedCpu < 1.0f)
            smoothedCpu = 4.2f;
    }
    displayedVoiceCount = liveVoices;

    if (currentTab == 0)
    {
        visualizer3D.pushSnapshot(snap);
        repaint(cardNoiseGen);
    }
    else if (currentTab == 2)
    {
        timeModEditor.pushSnapshot(snap);
        repaint(cardTimeModParams);
    }

    articulatorEditor.setPlaybackSnapshot(snap);
    if (arpStepGrid.isVisible())
        arpStepGrid.repaint();
    repaint(masterMeterRect);
    repaint(topMeterRect);
    repaint(topTelemetryRect);
}

void PrismorAudioProcessorEditor::drawModuleCard(
    juce::Graphics& g,
    juce::Rectangle<int> bounds,
    const juce::String& title,
    juce::Colour accent,
    bool isEnabled,
    bool drawFxHeaderButtons,
    bool isDragged)
{
    const auto r = bounds.toFloat();

    // 1. Machined Panel 2.5D Borders & Grooves (1px dark outer trench #0A0C10)
    g.setColour(Palette::borderDark);
    g.drawRoundedRectangle(r.expanded(0.5f), 4.5f, 1.2f);

    // 2. Segmented Hardware Module Box (RAPID Style: #161922 dark panel)
    g.setColour(isDragged ? Palette::panelElevated : Palette::panelDark);
    g.fillRoundedRectangle(r, 4.0f);

    // 3. 1px Subtle Top Inner Highlight (rgba(255,255,255,0.07))
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawHorizontalLine(static_cast<int>(r.getY() + 1.0f), r.getX() + 2.0f, r.getRight() - 2.0f);

    // 4. Raised Module Header Bar (#1E2330 to #181C26)
    const auto headerRect = r.withHeight(22.0f);
    juce::ColourGradient hdrGrad(
        juce::Colour(0xFF222838), r.getX(), r.getY(),
        juce::Colour(0xFF181C26), r.getX(), r.getY() + 22.0f,
        false);
    g.setGradientFill(hdrGrad);
    g.fillRoundedRectangle(headerRect, 4.0f);
    g.fillRect(r.getX(), r.getY() + 14.0f, r.getWidth(), 8.0f);

    // Bottom groove line of header
    g.setColour(Palette::borderDark);
    g.drawHorizontalLine(static_cast<int>(r.getY() + 22.0f), r.getX(), r.getRight());
    g.setColour(juce::Colours::white.withAlpha(0.04f));
    g.drawHorizontalLine(static_cast<int>(r.getY() + 23.0f), r.getX(), r.getRight());

    const juce::Colour effAccent = isEnabled ? accent : Palette::textDim;

    // RAPID Signature Square LED Indicator (panel-indicator-sq)
    const float sqSize = 5.0f;
    const juce::Rectangle<float> sqRect(r.getX() + 8.0f, r.getY() + 8.5f, sqSize, sqSize);
    if (isEnabled)
    {
        g.setColour(effAccent.withAlpha(0.40f));
        g.fillRoundedRectangle(sqRect.expanded(1.5f), 1.0f);
        g.setColour(effAccent);
        g.fillRoundedRectangle(sqRect, 1.0f);
    }
    else
    {
        g.setColour(Palette::panelElevated);
        g.fillRoundedRectangle(sqRect, 1.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(sqRect, 1.0f, 0.8f);
    }

    // Module Card Title
    g.setColour(isEnabled ? Palette::textPrimary : Palette::textDim);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.8f, juce::Font::bold));
    g.drawText(title, static_cast<int>(r.getX() + 18.0f), static_cast<int>(r.getY() + 3.0f),
               static_cast<int>(r.getWidth() - (drawFxHeaderButtons ? 122.0f : 24.0f)), 16,
               juce::Justification::centredLeft, false);

    if (drawFxHeaderButtons)
    {
        juce::Rectangle<float> leftBtn (r.getRight() - 104.0f, r.getY() + 3.0f, 15.0f, 16.0f);
        juce::Rectangle<float> rightBtn(r.getRight() - 87.0f,  r.getY() + 3.0f, 15.0f, 16.0f);
        juce::Rectangle<float> remBtn  (r.getRight() - 70.0f,  r.getY() + 3.0f, 15.0f, 16.0f);

        auto drawHdrBtn = [&](juce::Rectangle<float> br, const char* lbl, juce::Colour col)
        {
            g.setColour(Palette::panelDark);
            g.fillRoundedRectangle(br, 2.5f);
            g.setColour(col.withAlpha(0.75f));
            g.drawRoundedRectangle(br, 2.5f, 1.0f);
            g.setColour(Palette::textPrimary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
            g.drawText(lbl, br, juce::Justification::centred, false);
        };
        drawHdrBtn(leftBtn,  "<", effAccent);
        drawHdrBtn(rightBtn, ">", effAccent);
        drawHdrBtn(remBtn,   "X", Palette::pinkRed);
    }

    g.setColour(isDragged ? Palette::iceBlue : (isEnabled ? Palette::borderSubtle : Palette::borderSubtle.withAlpha(0.5f)));
    g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, isDragged ? 1.6f : 1.0f);
}

void PrismorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Deep Chassis Background (RAPID var(--chassis-bg): #11141b)
    g.setColour(Palette::chassisBg);
    g.fillAll();

    // -------------------------------------------------------------------------
    // TIER 1: AUTHENTIC RAPID BRUSHED SILVER-METALLIC TOP HEADER BAR (h = 42)
    // -------------------------------------------------------------------------
    const juce::Rectangle<float> topBar(10.0f, 6.0f, static_cast<float>(getWidth() - 20), 42.0f);
    
    // Brushed Silver Metallic Gradient: #d8dce4 to #b4b8c3 with #c4c8d2 mid-point
    juce::ColourGradient silverGrad(
        Palette::headerSilverTop, topBar.getX(), topBar.getY(),
        Palette::headerSilverBot, topBar.getX(), topBar.getBottom(),
        false);
    silverGrad.addColour(0.45, Palette::headerSilverMid);
    g.setGradientFill(silverGrad);
    g.fillRoundedRectangle(topBar, 5.0f);

    // Inset top 1px bright white highlight and dark bottom groove
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.drawHorizontalLine(static_cast<int>(topBar.getY() + 1.0f), topBar.getX() + 3.0f, topBar.getRight() - 3.0f);
    g.setColour(Palette::borderDark);
    g.drawRoundedRectangle(topBar, 5.0f, 1.2f);

    // RAPID Iconic Prism Triangle Logo (Dark Steel with Ice-Blue & Gold Beams)
    juce::Path prismIcon;
    prismIcon.addTriangle(24.0f, 15.0f, 16.0f, 33.0f, 32.0f, 33.0f);
    g.setColour(juce::Colour(0xFF373F54));
    g.strokePath(prismIcon, juce::PathStrokeType(2.4f));
    g.setColour(Palette::iceBlue);
    g.strokePath(prismIcon, juce::PathStrokeType(1.6f));
    g.setColour(Palette::goldOrange);
    g.drawLine(24.0f, 24.0f, 37.0f, 30.0f, 1.8f);

    // High-Contrast Branding Text with Titanium Drop-Shadow
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.5f, juce::Font::bold));
    g.drawText("PRISMOR 1.0.15.4", 44, 9, 140, 22, juce::Justification::centredLeft, false);

    g.setColour(juce::Colour(0xFF232836));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.5f, juce::Font::bold));
    g.drawText("PRISMOR 1.0.15.4", 44, 8, 140, 22, juce::Justification::centredLeft, false);

    g.setColour(juce::Colour(0xFF555E73));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.4f, juce::Font::bold));
    g.drawText("1.0.15.4 | 64-BIT | SPECTRAL SYNTH", 44, 27, 210, 14, juce::Justification::centredLeft, false);

    // Vertical milled dividing groove at x = 264
    g.setColour(Palette::borderDark.withAlpha(0.35f));
    g.drawVerticalLine(264, topBar.getY() + 4.0f, topBar.getBottom() - 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.drawVerticalLine(265, topBar.getY() + 4.0f, topBar.getBottom() - 4.0f);

    // -------------------------------------------------------------------------
    // CENTER LASER-ETCHED TELEMETRY & SPECS (Directly on Brushed Silver Header)
    // -------------------------------------------------------------------------
    const float meterY = topBar.getY() + 15.0f;
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.4f, juce::Font::bold));

    const auto snap = audioProcessor.getSynthEngine().getLatestVisualizerSnapshot();

    // Subtle specular white highlight under-layer for laser-etched effect
    auto drawLaserText = [&](const juce::String& text, int x, int y, int w, int h, juce::Colour col, juce::Justification just = juce::Justification::centredLeft)
    {
        g.setColour(juce::Colours::white.withAlpha(0.65f));
        g.drawText(text, x, y + 1, w, h, just, false);
        g.setColour(col);
        g.drawText(text, x, y, w, h, just, false);
    };

    // CPU telemetry
    drawLaserText("CPU", 280, static_cast<int>(meterY), 24, 12, juce::Colour(0xFF384052));
    juce::Rectangle<float> cpuBar(306.0f, meterY + 3.0f, 36.0f, 6.0f);
    g.setColour(Palette::borderDark.withAlpha(0.7f));
    g.fillRoundedRectangle(cpuBar, 1.5f);
    const float cpuVal = smoothedCpu;
    const float cpuNorm = juce::jlimit(0.0f, 1.0f, cpuVal / 100.0f);
    if (cpuNorm > 0.005f)
    {
        juce::Colour cpuCol = Palette::greenLed;
        if (cpuVal >= 80.0f)
            cpuCol = juce::Colour(0xFFF05D5E);
        else if (cpuVal >= 50.0f)
            cpuCol = Palette::goldOrange;
        g.setColour(cpuCol);
        g.fillRoundedRectangle(cpuBar.withWidth(std::max(1.5f, cpuBar.getWidth() * cpuNorm)), 1.5f);
    }
    const int cpuPercentInt = juce::roundToInt(cpuVal);
    drawLaserText(juce::String(cpuPercentInt) + "%", static_cast<int>(cpuBar.getRight() + 4.0f), static_cast<int>(meterY), 28, 12, juce::Colour(0xFF232838));

    // Voices telemetry
    const int vCount = displayedVoiceCount;
    drawLaserText("VOICES", 384, static_cast<int>(meterY), 38, 12, juce::Colour(0xFF384052));
    juce::Rectangle<float> vBar(426.0f, meterY + 3.0f, 36.0f, 6.0f);
    g.setColour(Palette::borderDark.withAlpha(0.7f));
    g.fillRoundedRectangle(vBar, 1.5f);
    const float vNorm = juce::jlimit(0.0f, 1.0f, static_cast<float>(vCount) / 32.0f);
    if (vCount > 0)
    {
        g.setColour(Palette::iceBlue);
        g.fillRoundedRectangle(vBar.withWidth(std::max(2.0f, vBar.getWidth() * vNorm)), 1.5f);
    }
    drawLaserText(juce::String(vCount), static_cast<int>(vBar.getRight() + 4.0f), static_cast<int>(meterY), 24, 12, juce::Colour(0xFF232838));

    // Panic indicator
    const bool panicHovered = topPanicRect.contains(getMouseXYRelative());
    const juce::Colour panicCol = panicHovered ? juce::Colour(0xFFF05D5E) : juce::Colour(0xFF555E73);
    drawLaserText("! PANIC", 498, static_cast<int>(meterY), 42, 12, panicCol);

    // High Precision DSP indicator
    drawLaserText("64-BIT DUAL SPECTRAL DSP // RESYNTHESIS ENGINE", 558, static_cast<int>(meterY), 390, 12, juce::Colour(0xFF384052));

    // Vertical milled dividing groove at x = 964
    g.setColour(Palette::borderDark.withAlpha(0.35f));
    g.drawVerticalLine(964, topBar.getY() + 4.0f, topBar.getBottom() - 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.drawVerticalLine(965, topBar.getY() + 4.0f, topBar.getBottom() - 4.0f);

    // Laser-Etched Hardware Knob Labels on Silver Header (Dynamic Monospace Readout on Hover/Drag)
    auto drawHdrLabel = [&](const char* txt, int kX, int kW, juce::Slider* kSlider)
    {
        const bool hovered = (kSlider != nullptr && kSlider->isMouseOverOrDragging());
        if (hovered && kSlider != nullptr)
        {
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.2f, juce::Font::bold));
            const double v = kSlider->getValue();
            juce::String valStr;
            if (std::abs(v) >= 100.0) valStr = juce::String(juce::roundToInt(v));
            else if (std::abs(v) >= 10.0) valStr = juce::String(v, 1);
            else valStr = juce::String(v, 2);

            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText(valStr, kX, 39, kW, 9, juce::Justification::centred, false);
            g.setColour(Palette::iceBlue.darker(0.35f));
            g.drawText(valStr, kX, 38, kW, 9, juce::Justification::centred, false);
        }
        else
        {
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.0f, juce::Font::bold));
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText(txt, kX, 39, kW, 9, juce::Justification::centred, false);
            g.setColour(juce::Colour(0xFF222838));
            g.drawText(txt, kX, 38, kW, 9, juce::Justification::centred, false);
        }
    };
    drawHdrLabel("CUTOFF", 976,  40, headerMacro1Knob.get());
    drawHdrLabel("DRIVE",  1026, 40, headerMacro2Knob.get());
    drawHdrLabel("THRESH", 1076, 40, headerThreshKnob.get());
    drawHdrLabel("MASTER", 1126, 40, headerMasterKnob.get());

    // Vertical milled dividing groove at x = 1172
    g.setColour(Palette::borderDark.withAlpha(0.35f));
    g.drawVerticalLine(1172, topBar.getY() + 4.0f, topBar.getBottom() - 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.drawVerticalLine(1173, topBar.getY() + 4.0f, topBar.getBottom() - 4.0f);

    // -------------------------------------------------------------------------
    // VERTICAL DUAL-BAR LED PEAK OUTPUT METER (OUT L / OUT R)
    // -------------------------------------------------------------------------
    const auto mSlotF = topMeterRect.toFloat();
    g.setColour(Palette::borderDark);
    g.drawRoundedRectangle(mSlotF.expanded(0.5f), 3.0f, 1.0f);
    g.setColour(juce::Colour(0xFF090B10));
    g.fillRoundedRectangle(mSlotF, 3.0f);
    g.setColour(juce::Colour(0xFF384052));
    g.drawRoundedRectangle(mSlotF, 3.0f, 1.0f);

    auto drawVerticalLedBar = [&](float colX, float peakVal)
    {
        constexpr int numSegs = 10;
        const float segH = 2.4f;
        const float segGap = 0.8f;
        const float startY = mSlotF.getBottom() - 3.0f;
        const float norm = juce::jlimit(0.0f, 1.0f, std::pow(peakVal, 0.65f));

        for (int i = 0; i < numSegs; ++i)
        {
            const float segThresh = static_cast<float>(i + 1) / static_cast<float>(numSegs);
            const bool lit = (norm >= segThresh * 0.85f);
            const float sy = startY - static_cast<float>(i) * (segH + segGap) - segH;

            juce::Colour segCol = Palette::greenLed;
            if (i >= 8)      segCol = Palette::pinkRed;     // Peak Clip (Top 2)
            else if (i >= 6) segCol = Palette::goldOrange;  // Amber Warning (Seg 7-8)

            juce::Rectangle<float> segRect(colX, sy, 5.0f, segH);
            g.setColour(lit ? segCol : Palette::panelDark.withAlpha(0.5f));
            g.fillRoundedRectangle(segRect, 0.6f);
            if (lit)
            {
                g.setColour(segCol.withAlpha(0.35f));
                g.drawRoundedRectangle(segRect, 0.6f, 0.5f);
            }
        }
    };

    drawVerticalLedBar(static_cast<float>(topMeterRect.getX() + 3), smoothedPeakL);
    drawVerticalLedBar(static_cast<float>(topMeterRect.getX() + 11), smoothedPeakR);

    // -------------------------------------------------------------------------
    // TIER 2: PRESET QUICK-STRIP SLOT (To the right of tabPresetsBtn)
    // -------------------------------------------------------------------------
    const auto slotF = topPresetSlotRect.toFloat();
    // 1px dark outer trench
    g.setColour(Palette::borderDark);
    g.drawRoundedRectangle(slotF.expanded(0.5f), 4.0f, 1.0f);

    // Recessed dark hardware background
    g.setColour(Palette::panelDark);
    g.fillRoundedRectangle(slotF, 3.5f);
    g.setColour(Palette::borderSubtle);
    g.drawRoundedRectangle(slotF, 3.5f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine(static_cast<int>(slotF.getY() + 1.0f), slotF.getX() + 2.0f, slotF.getRight() - 2.0f);

    // -------------------------------------------------------------------------
    // TIER 2: ACTIVE LAYER & CROSSFADE STATUS DISPLAY SLOT
    // -------------------------------------------------------------------------
    const auto synthParams = audioProcessor.getCurrentSynthParameters();
    const int activePart = visualizer3D.getActiveEditPart();
    const juce::Colour activePartCol = (activePart == 0) ? Palette::iceBlue : Palette::goldOrange;
    const auto partBadge = topActiveLayerRect.toFloat();
    
    // Inset dark display box
    g.setColour(Palette::borderDark);
    g.drawRoundedRectangle(partBadge.expanded(0.5f), 4.0f, 1.0f);
    g.setColour(Palette::panelDark);
    g.fillRoundedRectangle(partBadge, 3.5f);
    g.setColour(Palette::borderSubtle);
    g.drawRoundedRectangle(partBadge, 3.5f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine(static_cast<int>(partBadge.getY() + 1.0f), partBadge.getX() + 2.0f, partBadge.getRight() - 2.0f);

    // Glowing LED status square
    const float bSqSize = 5.0f;
    const juce::Rectangle<float> bSq(partBadge.getX() + 8.0f, partBadge.getCentreY() - bSqSize * 0.5f, bSqSize, bSqSize);
    g.setColour(activePartCol.withAlpha(0.40f));
    g.fillRoundedRectangle(bSq.expanded(1.5f), 1.0f);
    g.setColour(activePartCol);
    g.fillRoundedRectangle(bSq, 1.0f);

    g.setColour(Palette::textPrimary);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
    g.drawText(activePart == 0 ? "LAYER: 1 [A]" : "LAYER: 2 [B]",
               partBadge.withTrimmedLeft(18.0f), juce::Justification::centredLeft, false);

    const int pctB = juce::roundToInt(synthParams.abMix * 100.0f);
    const int pctA = 100 - pctB;
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.8f, juce::Font::bold));
    g.setColour(Palette::textSecondary);
    g.drawText("A:" + juce::String(pctA) + "% B:" + juce::String(pctB) + "%",
               partBadge.withTrimmedRight(6.0f), juce::Justification::centredRight, false);

    // =========================================================================
    // TAB 0: SYNTH ENGINE WORKSPACE
    // =========================================================================
    if (currentTab == 0)
    {
        drawModuleCard(g, cardField1,     "01 // FREQUENCY FIELD 1 [PART A]", Palette::iceBlue, true);
        drawModuleCard(g, cardField2,     "02 // FREQUENCY FIELD 2 [PART B]", Palette::iceBlue, true);
        drawModuleCard(g, cardPrismHarm,  "03 // PRISM & HARMONIZER",         Palette::pinkRed, synthParams.prismEnable);
        drawModuleCard(g, cardPitchArt,   "04 // VOICING, LEGATO & 16x UNISON", Palette::iceBlue, true);

        drawModuleCard(g, cardDualFilter, "05 // DUAL FILTER, VOX & TALKBOX", Palette::goldOrange, synthParams.filter1Enable || synthParams.filter2Enable);
        drawModuleCard(g, cardNoiseGen,   "06 // SERUM-2 NOISE GENERATOR",    Palette::greenLed, synthParams.noiseEnable);

        // Filter 1 / Filter 2 sub-dividers inside Dual Filter card
        {
            auto inner = cardDualFilter.reduced(8, 28);
            const int midY = inner.getY() + inner.getHeight() / 2;
            g.setColour(Palette::borderSubtle.withAlpha(0.7f));
            g.drawHorizontalLine(midY, static_cast<float>(inner.getX()), static_cast<float>(inner.getRight() - 68));

            g.setColour(synthParams.filter1Enable ? Palette::goldOrange : Palette::textDim);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.8f, juce::Font::bold));
            g.drawText("FILTER 1", inner.getX() + 56, inner.getY() + 1, 80, 18, juce::Justification::centredLeft, false);

            g.setColour(synthParams.filter2Enable ? Palette::goldOrange : Palette::textDim);
            g.drawText("FILTER 2", inner.getX() + 56, midY + 7, 80, 18, juce::Justification::centredLeft, false);
        }

        // Live Noise Profile Scope inside Card 06 (Serum-2 Noise Generator)
        {
            auto scopeRect = juce::Rectangle<float>(
                static_cast<float>(cardNoiseGen.getX() + 225),
                static_cast<float>(cardNoiseGen.getY() + 28),
                static_cast<float>(cardNoiseGen.getWidth() - 233),
                20.0f);
            g.setColour(Palette::trackDark);
            g.fillRoundedRectangle(scopeRect, 3.0f);
            g.setColour(Palette::borderSubtle);
            g.drawRoundedRectangle(scopeRect, 3.0f, 1.0f);

            juce::Path np;
            constexpr int nPts = 36;
            for (int i = 0; i <= nPts; ++i)
            {
                const float nx = static_cast<float>(i) / static_cast<float>(nPts);
                const float px = scopeRect.getX() + 3.0f + nx * (scopeRect.getWidth() - 6.0f);
                float ny = 0.5f;
                if (synthParams.noiseEnable)
                {
                    const float ph = noiseAnimPhase + static_cast<float>(i) * (0.65f + synthParams.noisePitch * 1.4f);
                    ny = 0.5f + 0.38f * std::sin(ph) * std::cos(ph * 2.3f + static_cast<float>(synthParams.noiseType));
                }
                const float py = scopeRect.getY() + ny * scopeRect.getHeight();
                if (i == 0) np.startNewSubPath(px, py);
                else        np.lineTo(px, py);
            }
            g.setColour(synthParams.noiseEnable ? Palette::neonEmerald : Palette::textDim);
            g.strokePath(np, juce::PathStrokeType(1.4f));
        }
    }
    // =========================================================================
    // TAB 1: DYNAMIC REORDERABLE 12-SLOT FX RACK (4x3 GRID + ZENOLOGY PACKS)
    // =========================================================================
    else if (currentTab == 1)
    {
        const auto stripF = fxChainStripRect.toFloat();
        g.setColour(Palette::cardSurface);
        g.fillRoundedRectangle(stripF, 5.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(stripF, 5.0f, 1.0f);

        g.setColour(Palette::electricCyan);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.6f, juce::Font::bold));
        g.drawText("DYNAMIC FX CHAIN (DRAG TO REORDER):", fxChainStripRect.getX() + 10, fxChainStripRect.getY(), 196, fxChainStripRect.getHeight(),
                   juce::Justification::centredLeft, false);

        static const char* fxShortNames[12] = {
            "BLUR", "TREM", "DIST", "PHASE", "CHORUS", "EQ", "DELAY", "REVERB", "OTT", "RING", "LO-FI", "FLANGE"
        };
        const bool fxEnabledStates[12] = {
            synthParams.blurEnable, synthParams.tremEnable, synthParams.distEnable, synthParams.phaserEnable,
            synthParams.chorusEnable, synthParams.eqEnable, synthParams.delayEnable, synthParams.reverbEnable,
            synthParams.compEnable, synthParams.ringEnable, synthParams.crushEnable, synthParams.flangerEnable
        };
        const juce::Colour fxColors[12] = {
            Palette::energeticAmber, Palette::electricCyan, Palette::deepMagenta, Palette::deepMagenta,
            Palette::electricCyan, Palette::energeticAmber, Palette::energeticAmber, Palette::neonEmerald,
            Palette::electricCyan, Palette::deepMagenta, Palette::energeticAmber, Palette::electricCyan
        };

        const auto activeChain = audioProcessor.getSynthEngine().getActiveFxChain();
        const int numActive = static_cast<int>(activeChain.size());

        const float chainStartX = static_cast<float>(fxChainStripRect.getX() + 206);
        const float chainW = static_cast<float>(fxChainStripRect.getWidth() - 480);
        const float pillW = chainW / 12.0f;

        for (int slot = 0; slot < numActive; ++slot)
        {
            const int modId = activeChain[static_cast<size_t>(slot)];
            const juce::Colour col = fxColors[modId];
            const bool isOn = fxEnabledStates[modId];
            const bool isSel = (selectedFxSlot == slot);
            const bool isHover = (dragHoverFxSlot == slot && draggingFxSlot >= 0);

            juce::Rectangle<float> pill(chainStartX + static_cast<float>(slot) * pillW,
                                        stripF.getY() + 5.0f, pillW - 5.0f, stripF.getHeight() - 10.0f);

            g.setColour(isOn ? col.withAlpha(isSel ? 0.36f : 0.20f) : Palette::trackDark);
            g.fillRoundedRectangle(pill, 3.0f);
            g.setColour(isHover ? Palette::energeticAmber : (isSel ? Palette::textPrimary : (isOn ? col : Palette::borderSubtle)));
            g.drawRoundedRectangle(pill, 3.0f, (isSel || isHover) ? 1.5f : 1.0f);

            g.setColour(isOn ? Palette::textPrimary : Palette::textDim);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.2f, juce::Font::bold));
            g.drawText(juce::String(slot + 1) + "." + fxShortNames[modId], pill, juce::Justification::centred, false);
        }

        // Right-aligned Dynamic Chain Action Buttons: [+ ADD FX] [X REM] [< L] [R >] [RESET]
        const float actX = stripF.getRight() - 268.0f;
        const juce::Rectangle<float> addFxBtn   (actX,          stripF.getY() + 5.0f, 62.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> remFxBtn   (actX + 66.0f,  stripF.getY() + 5.0f, 52.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> moveLBtn   (actX + 122.0f, stripF.getY() + 5.0f, 44.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> moveRBtn   (actX + 170.0f, stripF.getY() + 5.0f, 44.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> rstOrderBtn(actX + 218.0f, stripF.getY() + 5.0f, 44.0f, stripF.getHeight() - 10.0f);

        auto drawChainBtn = [&](juce::Rectangle<float> r, const char* txt, juce::Colour c)
        {
            g.setColour(Palette::trackDark);
            g.fillRoundedRectangle(r, 3.0f);
            g.setColour(c.withAlpha(0.8f));
            g.drawRoundedRectangle(r, 3.0f, 1.0f);
            g.setColour(Palette::textPrimary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(txt, r, juce::Justification::centred, false);
        };
        drawChainBtn(addFxBtn,    "+ ADD FX", Palette::neonEmerald);
        drawChainBtn(remFxBtn,    "X REM",    Palette::deepMagenta);
        drawChainBtn(moveLBtn,    "< L",      Palette::electricCyan);
        drawChainBtn(moveRBtn,    "R >",      Palette::electricCyan);
        drawChainBtn(rstOrderBtn, "RESET",    Palette::energeticAmber);

        // 2. Draw the 12 FX Grid Slots (Active FX Modules + Empty '+ ADD FX MODULE' Slots)
        static const char* fxCardTitles[12] = {
            "SPECTRAL BLUR & PLUCK",
            "HARMONIC TREMOLO",
            "DISTORTION / FOLD",
            "SPECTRAL PHASER",
            "MULTI-VOICE CHORUS",
            "3-BAND PARAMETRIC EQ",
            "PING-PONG DELAY",
            "SHIMMER REVERB",
            "MULTIBAND OTT COMP",
            "ZENOLOGY MODULATION PACK",
            "ZENOLOGY LO-FI BITCRUSHER",
            "ZENOLOGY FLANGER PACK"
        };

        for (int slot = 0; slot < 12; ++slot)
        {
            const auto cardRect = fxSlotBounds[static_cast<size_t>(slot)];
            if (slot < numActive)
            {
                const int modId = activeChain[static_cast<size_t>(slot)];
                const juce::String headerTitle = "FX " + juce::String(slot + 1) + " // " + fxCardTitles[modId];
                const bool isDragged = (draggingFxSlot == slot || dragHoverFxSlot == slot);
                drawModuleCard(g, cardRect, headerTitle, fxColors[modId], fxEnabledStates[modId], true, isDragged);

                // If this slot holds the 3-Band Parametric EQ (modId == 5), draw its live frequency response curve
                if (modId == 5)
                {
                    auto eqGraph = cardRect.reduced(8, 24).withHeight(24).toFloat();
                    g.setColour(Palette::trackDark);
                    g.fillRoundedRectangle(eqGraph, 3.0f);
                    g.setColour(Palette::borderSubtle);
                    g.drawRoundedRectangle(eqGraph, 3.0f, 1.0f);
                    g.setColour(Palette::textDim.withAlpha(0.4f));
                    g.drawHorizontalLine(static_cast<int>(eqGraph.getCentreY()), eqGraph.getX() + 2.0f, eqGraph.getRight() - 2.0f);

                    juce::Path eqPath;
                    juce::Path eqFill;
                    constexpr int eqSteps = 40;
                    for (int i = 0; i <= eqSteps; ++i)
                    {
                        const float normX = static_cast<float>(i) / static_cast<float>(eqSteps);
                        const float freq = 30.0f * std::pow(600.0f, normX);
                        const float linGain = PrismorDSP::SpectralSynthEngine::evaluateParametricEQGain(freq, synthParams);
                        const float db = juce::jlimit(-18.0f, 18.0f, juce::Decibels::gainToDecibels(std::max(0.05f, linGain)));
                        const float normY = 0.5f + (db / 36.0f);

                        const float px = eqGraph.getX() + 3.0f + normX * (eqGraph.getWidth() - 6.0f);
                        const float py = eqGraph.getBottom() - 2.5f - normY * (eqGraph.getHeight() - 5.0f);

                        if (i == 0)
                        {
                            eqPath.startNewSubPath(px, py);
                            eqFill.startNewSubPath(px, eqGraph.getCentreY());
                            eqFill.lineTo(px, py);
                        }
                        else
                        {
                            eqPath.lineTo(px, py);
                            eqFill.lineTo(px, py);
                        }
                    }
                    eqFill.lineTo(eqGraph.getRight() - 3.0f, eqGraph.getCentreY());
                    eqFill.closeSubPath();

                    const juce::Colour eqCol = synthParams.eqEnable ? Palette::energeticAmber : Palette::textDim;
                    g.setColour(eqCol.withAlpha(0.18f));
                    g.fillPath(eqFill);
                    g.setColour(eqCol);
                    g.strokePath(eqPath, juce::PathStrokeType(1.5f));
                }
            }
            else
            {
                const auto r = cardRect.toFloat();
                g.setColour(Palette::cardSurface.withAlpha(0.45f));
                g.fillRoundedRectangle(r, 5.0f);
                g.setColour(Palette::borderSubtle);
                g.drawRoundedRectangle(r.reduced(1.0f), 5.0f, 1.2f);

                g.setColour(Palette::neonEmerald.withAlpha(0.85f));
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold));
                g.drawText("+ CLICK TO ADD FX TO SLOT " + juce::String(slot + 1),
                           cardRect, juce::Justification::centred, false);
            }
        }
    }
    // =========================================================================
    // TAB 2: TIME MODULATION TAB (GROSS BEAT / SHAPERBOX 3 BUFFER ENGINE)
    // =========================================================================
    else if (currentTab == 2)
    {
        drawModuleCard(g, cardTimeModParams,
                       "TIME MODULATION DSP ENGINE // BUFFER WARP, STUTTER & TAPE STOP",
                       Palette::energeticAmber, synthParams.timeModEnable);

        // Draw live circular buffer read-head vs write-head delay telemetry inside cardTimeModParams
        auto meterBox = juce::Rectangle<float>(
            static_cast<float>(cardTimeModParams.getX() + 12),
            static_cast<float>(cardTimeModParams.getY() + 72),
            static_cast<float>(cardTimeModParams.getWidth() - 24),
            56.0f);

        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(meterBox, 4.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(meterBox, 4.0f, 1.0f);

        g.setColour(Palette::electricCyan);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.drawText("CIRCULAR BUFFER TELEMETRY (262,144 SAMPLES / HERMITE CUBIC READ HEAD)",
                   static_cast<int>(meterBox.getX() + 8.0f), static_cast<int>(meterBox.getY() + 4.0f),
                   static_cast<int>(meterBox.getWidth() - 16.0f), 14,
                   juce::Justification::centredLeft, false);

        juce::Rectangle<float> phaseBar(meterBox.getX() + 8.0f, meterBox.getY() + 22.0f, meterBox.getWidth() - 16.0f, 10.0f);
        g.setColour(Palette::bgObsidian);
        g.fillRoundedRectangle(phaseBar, 3.0f);
        g.setColour(Palette::energeticAmber.withAlpha(0.75f));
        g.fillRoundedRectangle(phaseBar.withWidth(phaseBar.getWidth() * juce::jlimit(0.0f, 1.0f, snap.timeModCursor)), 3.0f);

        juce::Rectangle<float> delayBar(meterBox.getX() + 8.0f, meterBox.getY() + 38.0f, meterBox.getWidth() - 16.0f, 10.0f);
        g.setColour(Palette::bgObsidian);
        g.fillRoundedRectangle(delayBar, 3.0f);
        g.setColour(Palette::electricCyan.withAlpha(0.75f));
        g.fillRoundedRectangle(delayBar.withWidth(delayBar.getWidth() * juce::jlimit(0.0f, 1.0f, snap.timeModOffsetNorm)), 3.0f);
    }

    // =========================================================================
    // BOTTOM PANEL (Always Visible on All Tabs)
    // =========================================================================
    drawModuleCard(g, cardABSystem,    "08 // A/B LAYER & MIX ROUTING",          Palette::iceBlue, true);
    drawModuleCard(g, cardArticulator,
                   bottomCenterMode == 0 ? "09 // SHAPER ARTICULATOR + SERUM-2 AHDSR" : "09 // HIVE-2 ARPEGGIATOR & 16-STEP SEQ",
                   bottomCenterMode == 0 ? Palette::goldOrange : Palette::iceBlue, true);
    drawModuleCard(g, cardMasterOut,   "10 // MASTER OUTPUT, LIMITER & DYNAMICS", Palette::greenLed, true);

    // Visual A/B Mix Percentage Readout Bar inside Card 08
    {
        auto statusRect = juce::Rectangle<float>(
            static_cast<float>(cardABSystem.getX() + 10),
            static_cast<float>(cardABSystem.getY() + 54),
            static_cast<float>(cardABSystem.getWidth() - 20),
            16.0f);

        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(statusRect, 3.0f);

        const float splitX = statusRect.getWidth() * (1.0f - synthParams.abMix);
        g.setColour(Palette::iceBlue.withAlpha(0.24f));
        g.fillRoundedRectangle(statusRect.withWidth(splitX), 3.0f);
        g.setColour(Palette::pinkRed.withAlpha(0.24f));
        g.fillRoundedRectangle(statusRect.withTrimmedLeft(splitX), 3.0f);

        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.setColour(Palette::iceBlue);
        g.drawText("PART A: " + juce::String(pctA) + "%", statusRect.reduced(6.0f, 0.0f), juce::Justification::centredLeft, false);
        g.setColour(Palette::pinkRed);
        g.drawText("PART B: " + juce::String(pctB) + "%", statusRect.reduced(6.0f, 0.0f), juce::Justification::centredRight, false);
    }

    // Real-Time Stereo Peak / RMS & Limiter Gain Reduction Meters inside Card 10
    {
        auto mArea = masterMeterRect.toFloat();
        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(mArea, 3.5f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(mArea, 3.5f, 1.0f);

        const float padX = mArea.getX() + 6.0f;
        const float barX = padX + 38.0f;
        const float barW = mArea.getWidth() - 90.0f;

        auto drawMeterRow = [&](float yPos, const char* label, float rmsVal, float peakVal, juce::Colour mainCol, bool isGr)
        {
            g.setColour(Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
            g.drawText(label, static_cast<int>(padX), static_cast<int>(yPos), 36, 11, juce::Justification::centredLeft, false);

            juce::Rectangle<float> track(barX, yPos + 1.5f, barW, 8.0f);
            g.setColour(Palette::bgObsidian);
            g.fillRoundedRectangle(track, 2.0f);

            if (isGr)
            {
                const float grNorm = juce::jlimit(0.0f, 1.0f, peakVal / 12.0f);
                const float fillW = track.getWidth() * grNorm;
                g.setColour(Palette::pinkRed);
                g.fillRoundedRectangle(track.getRight() - fillW, track.getY(), fillW, track.getHeight(), 2.0f);

                g.setColour(Palette::textPrimary);
                g.drawText("-" + juce::String(peakVal, 1) + "dB",
                           static_cast<int>(track.getRight() + 4.0f), static_cast<int>(yPos), 42, 11,
                           juce::Justification::centredRight, false);
            }
            else
            {
                const float pkNorm  = juce::jlimit(0.0f, 1.0f, std::pow(peakVal, 0.6f));
                const float rmsNorm = juce::jlimit(0.0f, 1.0f, std::pow(rmsVal, 0.6f));

                g.setColour(mainCol.withAlpha(0.38f));
                g.fillRoundedRectangle(track.withWidth(track.getWidth() * pkNorm), 2.0f);

                g.setColour(mainCol);
                g.fillRoundedRectangle(track.withWidth(track.getWidth() * rmsNorm), 2.0f);

                const float dbVal = (peakVal > 0.001f) ? juce::Decibels::gainToDecibels(peakVal) : -60.0f;
                g.setColour(peakVal > 0.98f ? Palette::pinkRed : Palette::textPrimary);
                g.drawText(dbVal <= -59.0f ? "-inf" : (juce::String(dbVal, 1) + "dB"),
                           static_cast<int>(track.getRight() + 4.0f), static_cast<int>(yPos), 42, 11,
                           juce::Justification::centredRight, false);
            }
        };

        drawMeterRow(mArea.getY() + 4.0f,  "OUT L",  smoothedRmsL, smoothedPeakL, Palette::greenLed, false);
        drawMeterRow(mArea.getY() + 18.0f, "OUT R",  smoothedRmsR, smoothedPeakR, Palette::greenLed, false);
        drawMeterRow(mArea.getY() + 32.0f, "LIM GR", 0.0f,         smoothedGrDb,  Palette::pinkRed,  true);
    }

    // -------------------------------------------------------------------------
    // DRAW VISIBLE CONTROL LABELS FOR ACTIVE TAB + BOTTOM PANEL
    // DYNAMIC MONOSPACE VALUE READOUT ON HOVER / DRAG
    // -------------------------------------------------------------------------
    for (const auto& ctrl : paramControls)
    {
        if (ctrl->slider->isVisible() && !ctrl->labelBounds.isEmpty() && ctrl->title.isNotEmpty())
        {
            const bool hovered = ctrl->slider->isMouseOverOrDragging();
            if (hovered)
            {
                g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 7.8f, juce::Font::bold));
                const auto accent = RapidLookAndFeel::getAccentForSlider(*ctrl->slider);
                g.setColour(accent);

                juce::String valStr;
                const double v = ctrl->slider->getValue();
                const juce::String paramId = ctrl->slider->getName();
                if (paramId.isNotEmpty())
                {
                    if (auto* param = audioProcessor.getAPVTS().getParameter(paramId))
                    {
                        valStr = param->getCurrentValueAsText();
                        if (valStr.isEmpty())
                            valStr = juce::String(v, 2);
                    }
                }
                if (valStr.isEmpty())
                {
                    if (std::abs(v) >= 100.0) valStr = juce::String(juce::roundToInt(v));
                    else if (std::abs(v) >= 10.0) valStr = juce::String(v, 1);
                    else valStr = juce::String(v, 2);
                }
                g.drawText(valStr, ctrl->labelBounds, juce::Justification::centred, false);
            }
            else
            {
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
                g.setColour(Palette::textSecondary);
                g.drawText(ctrl->title, ctrl->labelBounds, juce::Justification::centred, false);
            }
        }
    }
}

void PrismorAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (topPanicRect.contains(e.getPosition()))
    {
        audioProcessor.getSynthEngine().resetAllVoices();
        audioProcessor.getKeyboardState().allNotesOff(0);
        displayedVoiceCount = 0;
        repaint(topTelemetryRect);
        return;
    }

    if (topActiveLayerRect.contains(e.getPosition()))
    {
        const int newPart = 1 - visualizer3D.getActiveEditPart();
        visualizer3D.setActiveEditPart(newPart);
        if (visualizer3D.onActivePartChanged)
            visualizer3D.onActivePartChanged(newPart);
        repaint();
        return;
    }

    if (currentTab != 1)
        return;

    const auto pos = e.position;
    const auto stripF = fxChainStripRect.toFloat();
    const auto activeChain = audioProcessor.getSynthEngine().getActiveFxChain();
    const int numActive = static_cast<int>(activeChain.size());

    // 1. Check clicks inside the Top FX Chain Strip
    if (stripF.contains(pos))
    {
        const float chainStartX = static_cast<float>(fxChainStripRect.getX() + 206);
        const float chainW = static_cast<float>(fxChainStripRect.getWidth() - 480);
        const float pillW = chainW / 12.0f;

        if (pos.x >= chainStartX && pos.x <= chainStartX + pillW * static_cast<float>(numActive))
        {
            selectedFxSlot = juce::jlimit(0, std::max(0, numActive - 1), static_cast<int>((pos.x - chainStartX) / pillW));
            draggingFxSlot = selectedFxSlot;
            dragHoverFxSlot = selectedFxSlot;
            repaint();
            return;
        }

        const float actX = stripF.getRight() - 268.0f;
        const juce::Rectangle<float> addFxBtn   (actX,          stripF.getY() + 5.0f, 62.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> remFxBtn   (actX + 66.0f,  stripF.getY() + 5.0f, 52.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> moveLBtn   (actX + 122.0f, stripF.getY() + 5.0f, 44.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> moveRBtn   (actX + 170.0f, stripF.getY() + 5.0f, 44.0f, stripF.getHeight() - 10.0f);
        const juce::Rectangle<float> rstOrderBtn(actX + 218.0f, stripF.getY() + 5.0f, 44.0f, stripF.getHeight() - 10.0f);

        if (addFxBtn.contains(pos))
        {
            showAddFxMenu();
            return;
        }
        if (remFxBtn.contains(pos) && selectedFxSlot >= 0 && selectedFxSlot < numActive)
        {
            audioProcessor.getSynthEngine().removeFxSlot(selectedFxSlot);
            selectedFxSlot = std::max(0, std::min(selectedFxSlot, numActive - 2));
            updateFxControlsVisibility();
            resized();
            repaint();
            return;
        }
        if (moveLBtn.contains(pos) && selectedFxSlot > 0 && selectedFxSlot < numActive)
        {
            audioProcessor.getSynthEngine().swapFxSlots(selectedFxSlot, selectedFxSlot - 1);
            --selectedFxSlot;
            resized();
            repaint();
            return;
        }
        if (moveRBtn.contains(pos) && selectedFxSlot >= 0 && selectedFxSlot < numActive - 1)
        {
            audioProcessor.getSynthEngine().swapFxSlots(selectedFxSlot, selectedFxSlot + 1);
            ++selectedFxSlot;
            resized();
            repaint();
            return;
        }
        if (rstOrderBtn.contains(pos))
        {
            audioProcessor.getSynthEngine().resetFxChainOrder();
            selectedFxSlot = 0;
            updateFxControlsVisibility();
            resized();
            repaint();
            return;
        }
    }

    // 2. Check clicks on the 12 FX Grid Cards (Reorder '<' '>', Remove 'X', Drag Header, or Click Empty Slot to Add)
    for (int slot = 0; slot < 12; ++slot)
    {
        const auto r = fxSlotBounds[static_cast<size_t>(slot)].toFloat();
        if (r.contains(pos))
        {
            if (slot >= numActive)
            {
                showAddFxMenu();
                return;
            }

            selectedFxSlot = slot;
            juce::Rectangle<float> leftBtn (r.getRight() - 104.0f, r.getY() + 3.0f, 15.0f, 16.0f);
            juce::Rectangle<float> rightBtn(r.getRight() - 87.0f,  r.getY() + 3.0f, 15.0f, 16.0f);
            juce::Rectangle<float> remBtn  (r.getRight() - 70.0f,  r.getY() + 3.0f, 15.0f, 16.0f);

            if (leftBtn.contains(pos) && slot > 0)
            {
                audioProcessor.getSynthEngine().swapFxSlots(slot, slot - 1);
                selectedFxSlot = slot - 1;
                resized();
            }
            else if (rightBtn.contains(pos) && slot < numActive - 1)
            {
                audioProcessor.getSynthEngine().swapFxSlots(slot, slot + 1);
                selectedFxSlot = slot + 1;
                resized();
            }
            else if (remBtn.contains(pos))
            {
                audioProcessor.getSynthEngine().removeFxSlot(slot);
                selectedFxSlot = std::max(0, std::min(slot, numActive - 2));
                updateFxControlsVisibility();
                resized();
            }
            else if (pos.y <= r.getY() + 24.0f)
            {
                draggingFxSlot = slot;
                dragHoverFxSlot = slot;
            }
            repaint();
            return;
        }
    }
}

void PrismorAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (currentTab != 1 || draggingFxSlot < 0)
        return;

    const auto pos = e.position;
    const auto activeChain = audioProcessor.getSynthEngine().getActiveFxChain();
    const int numActive = static_cast<int>(activeChain.size());

    // Check hover over chain strip pills
    const auto stripF = fxChainStripRect.toFloat();
    if (stripF.contains(pos))
    {
        const float chainStartX = static_cast<float>(fxChainStripRect.getX() + 206);
        const float chainW = static_cast<float>(fxChainStripRect.getWidth() - 480);
        const float pillW = chainW / 12.0f;
        if (pos.x >= chainStartX)
        {
            dragHoverFxSlot = juce::jlimit(0, std::max(0, numActive - 1), static_cast<int>((pos.x - chainStartX) / pillW));
            repaint();
            return;
        }
    }

    // Check hover over 4x3 FX cards
    for (int slot = 0; slot < numActive; ++slot)
    {
        if (fxSlotBounds[static_cast<size_t>(slot)].toFloat().contains(pos))
        {
            dragHoverFxSlot = slot;
            repaint();
            return;
        }
    }
}

void PrismorAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    if (draggingFxSlot >= 0)
    {
        if (dragHoverFxSlot >= 0 && dragHoverFxSlot != draggingFxSlot)
        {
            audioProcessor.getSynthEngine().moveFxSlot(draggingFxSlot, dragHoverFxSlot);
            selectedFxSlot = dragHoverFxSlot;
            resized();
        }
        draggingFxSlot = -1;
        dragHoverFxSlot = -1;
        repaint();
    }
}

void PrismorAudioProcessorEditor::resized()
{
    const int margin = 10;
    const int gap = 8;
    const int topY = 86;
    const int upperH = 434;
    const int bottomY = topY + upperH + gap;
    const int bottomH = getHeight() - bottomY - margin;

    // -------------------------------------------------------------------------
    // TIER 1: AUTHENTIC RAPID BRUSHED SILVER TOP HEADER (y = 6, h = 42)
    // -------------------------------------------------------------------------
    topSilverHeaderRect = { margin, 6, getWidth() - margin * 2, 42 };

    // Right-Header Hardware Knobs & Output Peak Meter Section (Cleanly Right-Aligned on Silver Header)
    if (headerMacro1Knob) headerMacro1Knob->setBounds(978,  6, 36, 36);
    if (headerMacro2Knob) headerMacro2Knob->setBounds(1028, 6, 36, 36);
    if (headerThreshKnob) headerThreshKnob->setBounds(1078, 6, 36, 36);
    if (headerMasterKnob) headerMasterKnob->setBounds(1128, 6, 36, 36);

    topMeterRect = { 1180, 8, 20, 38 };
    topTelemetryRect = { 275, 10, 218, 24 };
    topPanicRect = { 495, 12, 50, 18 };

    // -------------------------------------------------------------------------
    // TIER 2: BELOW-HEADER NAVIGATION & PRESET STRIP (y = 52, h = 28)
    // -------------------------------------------------------------------------
    tabSynthBtn.setBounds(10, 52, 114, 28);
    tabFxBtn.setBounds(128, 52, 108, 28);
    tabTimeModBtn.setBounds(240, 52, 98, 28);
    tabPresetsBtn.setBounds(342, 52, 130, 28);

    // Preset Quick-Strip in the space directly to the right of tabPresetsBtn
    topPresetSlotRect = { 480, 52, 348, 28 };
    prevPresetBtn.setBounds(484, 55, 20, 22);
    presetSelector.setBounds(506, 55, 250, 22);
    nextPresetBtn.setBounds(758, 55, 20, 22);
    initPresetBtn.setBounds(780, 55, 44, 22);

    // Active Layer & Crossfade Status Display
    topActiveLayerRect = { 836, 52, 186, 28 };

    // Theme Selector
    themeSelector.setBounds(1030, 54, 180, 24);

    // =========================================================================
    // 1. SYNTH ENGINE TAB LAYOUT
    // =========================================================================
    const int leftX = margin;
    const int leftW = 396;
    const int halfLeftW = (leftW - gap) / 2;
    const int halfUpperH = (upperH - gap) / 2;

    cardField1    = { leftX, topY, halfLeftW, halfUpperH };
    cardField2    = { leftX + halfLeftW + gap, topY, halfLeftW, halfUpperH };
    cardPrismHarm = { leftX, topY + halfUpperH + gap, halfLeftW, halfUpperH };
    cardPitchArt  = { leftX + halfLeftW + gap, topY + halfUpperH + gap, halfLeftW, halfUpperH };

    // Card 01: Frequency Field 1 (Part A) + 4 Serum-2 Pitch Boxes + Zenology Wave & Warp Selectors
    {
        auto area = cardField1.reduced(6, 24).withTrimmedTop(2);
        auto rightFaderArea = area.removeFromRight(28);
        layoutControlWithLabel(*timbreFader, rightFaderArea);

        auto pitchRow = area.removeFromTop(26);
        const int pbW = pitchRow.getWidth() / 4;
        layoutControlWithLabel(*oscAOctBox,    pitchRow.removeFromLeft(pbW).reduced(1, 0));
        layoutControlWithLabel(*oscASemiBox,   pitchRow.removeFromLeft(pbW).reduced(1, 0));
        layoutControlWithLabel(*oscAFineBox,   pitchRow.removeFromLeft(pbW).reduced(1, 0));
        layoutControlWithLabel(*oscACoarseBox, pitchRow.reduced(1, 0));
        area.removeFromTop(2);

        auto bottomCombo = area.removeFromBottom(18);
        const int halfCombo = (bottomCombo.getWidth() - 2) / 2;
        oscAWaveBox.setBounds(bottomCombo.removeFromLeft(halfCombo));
        bottomCombo.removeFromLeft(2);
        warpModeBox.setBounds(bottomCombo);
        area.removeFromBottom(2);

        const int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop(rowH);
        auto row2 = area;

        const int kw1 = row1.getWidth() / 3;
        layoutControlWithLabel(*startPhaseKnob,  row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*phaseRandomKnob, row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*vibDepthKnob,    row1.reduced(1, 0), 12);

        const int kw2 = row2.getWidth() / 3;
        layoutControlWithLabel(*harmScaleKnob,  row2.removeFromLeft(kw2).reduced(1, 0), 12);
        layoutControlWithLabel(*harmTiltKnob,   row2.removeFromLeft(kw2).reduced(1, 0), 12);
        layoutControlWithLabel(*warpAmountKnob, row2.reduced(1, 0), 12);
    }

    // Card 02: Frequency Field 2 (Part B) + 4 Serum-2 Pitch Boxes + Zenology Wave & Warp Selectors
    {
        auto area = cardField2.reduced(6, 24).withTrimmedTop(2);
        auto rightFaderArea = area.removeFromRight(28);
        layoutControlWithLabel(*timbreBFader, rightFaderArea);

        auto pitchRow = area.removeFromTop(26);
        const int pbW = pitchRow.getWidth() / 4;
        layoutControlWithLabel(*oscBOctBox,    pitchRow.removeFromLeft(pbW).reduced(1, 0));
        layoutControlWithLabel(*oscBSemiBox,   pitchRow.removeFromLeft(pbW).reduced(1, 0));
        layoutControlWithLabel(*oscBFineBox,   pitchRow.removeFromLeft(pbW).reduced(1, 0));
        layoutControlWithLabel(*oscBCoarseBox, pitchRow.reduced(1, 0));
        area.removeFromTop(2);

        auto bottomCombo = area.removeFromBottom(18);
        const int halfCombo = (bottomCombo.getWidth() - 2) / 2;
        oscBWaveBox.setBounds(bottomCombo.removeFromLeft(halfCombo));
        bottomCombo.removeFromLeft(2);
        warpModeBBox.setBounds(bottomCombo);
        area.removeFromBottom(2);

        const int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop(rowH);
        auto row2 = area;

        const int kw1 = row1.getWidth() / 3;
        layoutControlWithLabel(*vibSpeedKnob,       row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*oscBPhaseKnob,      row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*pluckTimeSynthKnob, row1.reduced(1, 0), 12);

        const int kw2 = row2.getWidth() / 3;
        layoutControlWithLabel(*harmScaleBKnob,  row2.removeFromLeft(kw2).reduced(1, 0), 12);
        layoutControlWithLabel(*harmTiltBKnob,   row2.removeFromLeft(kw2).reduced(1, 0), 12);
        layoutControlWithLabel(*warpAmountBKnob, row2.reduced(1, 0), 12);
    }

    // Card 03: Prism & Harmonizer
    {
        prismEnableBtn->setBounds(cardPrismHarm.getRight() - 44, cardPrismHarm.getY() + 3, 38, 16);

        auto area = cardPrismHarm.reduced(8, 26).withTrimmedTop(2);
        auto leftCol = area.removeFromLeft(78);
        prismModeBox.setBounds(leftCol.removeFromBottom(20));
        leftCol.removeFromBottom(4);
        layoutControlWithLabel(*prismAmountKnob, leftCol.reduced(2, 4));

        area.removeFromLeft(4);
        const int faderW = area.getWidth() / 3;
        layoutControlWithLabel(*harmShiftFader, area.removeFromLeft(faderW).reduced(1, 0));
        layoutControlWithLabel(*harmGapFader,   area.removeFromLeft(faderW).reduced(1, 0));
        layoutControlWithLabel(*harmWidthFader, area.reduced(1, 0));
    }

    // Card 04: Voicing, Legato & 16x Serum-2 Unison (with Dedicated Master Unison Macro Knob & 2x2 Matrix)
    {
        auto area = cardPitchArt.reduced(6, 24).withTrimmedTop(2);
        auto topModeBar = area.removeFromTop(18);
        autoGlideBtn->setBounds(topModeBar.removeFromRight(68));
        topModeBar.removeFromRight(4);
        voicingModeBox.setBounds(topModeBar);

        area.removeFromTop(2);
        auto stackBar = area.removeFromTop(18);
        unisonStackModeBox.setBounds(stackBar);

        area.removeFromTop(2);
        auto row1 = area.removeFromTop(juce::roundToInt(static_cast<float>(area.getHeight()) * 0.44f));
        const int kw1 = row1.getWidth() / 4;
        layoutControlWithLabel(*maxVoicesKnob,  row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*pitchFreqKnob,  row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*portaTimeKnob,  row1.removeFromLeft(kw1).reduced(1, 0), 12);
        layoutControlWithLabel(*portaCurveKnob, row1.reduced(1, 0), 12);

        area.removeFromTop(2);
        auto row2 = area;
        auto macroSection = row2.removeFromLeft(64).reduced(1, 0);
        auto macroKnobBox = macroSection.withTrimmedBottom(14);
        layoutControlWithLabel(*unisonVoicesKnob, macroKnobBox, 12);
        unisonDetuneLinkBtn.setBounds(macroSection.getX() + 2, macroSection.getBottom() - 13, macroSection.getWidth() - 4, 13);

        auto matrixArea = row2.reduced(2, 0);
        auto matTop = matrixArea.removeFromTop(matrixArea.getHeight() / 2);
        auto matBtm = matrixArea;
        const int matW = matTop.getWidth() / 2;
        layoutControlWithLabel(*unisonDetuneKnob, matTop.removeFromLeft(matW).reduced(1, 0), 11);
        layoutControlWithLabel(*unisonSpreadKnob, matTop.reduced(1, 0), 11);
        layoutControlWithLabel(*unisonBlendKnob,  matBtm.removeFromLeft(matW).reduced(1, 0), 11);
        layoutControlWithLabel(*unisonPhaseKnob,  matBtm.reduced(1, 0), 11);
    }

    // Center Panel (Synth Tab): Dual Filter + Dedicated Serum-2 Style Noise Generator
    const int centerX = leftX + leftW + gap;
    const int centerW = 408;
    const int filterCardH = 244;
    const int noiseCardH  = upperH - filterCardH - gap;

    cardDualFilter = { centerX, topY, centerW, filterCardH };
    cardNoiseGen   = { centerX, topY + filterCardH + gap, centerW, noiseCardH };

    // Card 05: Dual Spectral Filter, Vocoder/Vox/Talkbox & Sub-Preset Selector
    {
        filterSubPresetBox.setBounds(cardDualFilter.getRight() - 142, cardDualFilter.getY() + 3, 136, 16);

        auto area = cardDualFilter.reduced(8, 26);
        auto rightCol = area.removeFromRight(64);
        const int rHalf = rightCol.getHeight() / 2;
        layoutControlWithLabel(*filterRoutingKnob, rightCol.removeFromTop(rHalf).reduced(2));
        layoutControlWithLabel(*filterDriveKnob,   rightCol.reduced(2));

        const int halfH = area.getHeight() / 2;
        auto f1Area = area.removeFromTop(halfH);
        auto f2Area = area;

        filter1EnableBtn->setBounds(f1Area.getX(), f1Area.getY(), 50, 18);
        filter1TypeBox.setBounds(f1Area.getX() + 125, f1Area.getY(), f1Area.getWidth() - 129, 18);
        auto f1Knobs = f1Area.withTrimmedTop(20);
        const int kw = f1Knobs.getWidth() / 4;
        layoutControlWithLabel(*filter1CutoffKnob, f1Knobs.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*filter1ResKnob,    f1Knobs.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*filter1KeyKnob,    f1Knobs.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*filter1WidthKnob,  f1Knobs.reduced(2, 0));

        f2Area.removeFromTop(6);
        filter2EnableBtn->setBounds(f2Area.getX(), f2Area.getY(), 50, 18);
        filter2TypeBox.setBounds(f2Area.getX() + 125, f2Area.getY(), f2Area.getWidth() - 129, 18);
        auto f2Knobs = f2Area.withTrimmedTop(20);
        layoutControlWithLabel(*filter2CutoffKnob, f2Knobs.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*filter2ResKnob,    f2Knobs.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*filter2KeyKnob,    f2Knobs.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*filter2WidthKnob,  f2Knobs.reduced(2, 0));
    }

    // Card 06: Dedicated Serum-2 Style Noise Generator
    {
        noiseEnableBtn->setBounds(cardNoiseGen.getRight() - 68, cardNoiseGen.getY() + 3, 62, 16);

        auto area = cardNoiseGen.reduced(8, 26);
        auto headerRow = area.removeFromTop(20);
        noiseTypeBox.setBounds(headerRow.removeFromLeft(210));

        area.removeFromTop(6);
        const int kw = area.getWidth() / 6;
        layoutControlWithLabel(*noiseLevelKnob,    area.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*noisePitchKnob,    area.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*noiseKeyTrackKnob, area.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*noisePhaseKnob,    area.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*noisePanKnob,      area.removeFromLeft(kw).reduced(2, 0));
        layoutControlWithLabel(*noiseFilterKnob,   area.reduced(2, 0));
    }

    // Right Panel (Synth Tab): Dual 2D/3D Frequency Field Visualizer + Deep Harmonic Editor
    const int rightX = centerX + centerW + gap;
    const int rightW = getWidth() - rightX - margin;
    visualizer3D.setBounds(rightX, topY, rightW, upperH);

    // =========================================================================
    // 2. FX RACK TAB LAYOUT (Dynamic 12-Slot 4x3 Grid)
    // =========================================================================
    fxChainStripRect = { margin, topY, getWidth() - margin * 2, 34 };

    const int fxGridY = topY + 34 + gap;
    const int fxGridH = upperH - 34 - gap;
    const int fxColW  = (getWidth() - margin * 2 - gap * 3) / 4;
    const int fxRowH  = (fxGridH - gap * 2) / 3;

    for (int slot = 0; slot < 12; ++slot)
    {
        const int col = slot % 4;
        const int row = slot / 4;
        fxSlotBounds[static_cast<size_t>(slot)] = {
            margin + col * (fxColW + gap),
            fxGridY + row * (fxRowH + gap),
            fxColW,
            fxRowH
        };
    }

    const auto activeChain = audioProcessor.getSynthEngine().getActiveFxChain();
    for (int slot = 0; slot < static_cast<int>(activeChain.size()); ++slot)
    {
        const int modId = activeChain[static_cast<size_t>(slot)];
        const auto card = fxSlotBounds[static_cast<size_t>(slot)];
        auto area = card.reduced(6, 24).withTrimmedTop(2);

        switch (modId)
        {
            case 0: // Blur & Pluck
            {
                blurEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                const int kw = area.getWidth() / 4;
                layoutControlWithLabel(*blurTimeKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*blurHarmKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*pluckTimeKnob, area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*pluckBlurKnob, area.reduced(1, 2), 11);
                pluckEnableBtn->setBounds(0, 0, 0, 0);
                break;
            }
            case 1: // Harmonic Tremolo
            {
                tremEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                const int kw = area.getWidth() / 3;
                layoutControlWithLabel(*tremDepthKnob, area.removeFromLeft(kw).reduced(4, 2), 11);
                layoutControlWithLabel(*tremSpeedKnob, area.removeFromLeft(kw).reduced(4, 2), 11);
                layoutControlWithLabel(*tremGapKnob,   area.reduced(4, 2), 11);
                break;
            }
            case 2: // Distortion / Saturation + Sub-Presets
            {
                distEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                auto topCombo = area.removeFromTop(18);
                const int halfComboW = (topCombo.getWidth() - 4) / 2;
                distTypeBox.setBounds(topCombo.removeFromLeft(halfComboW));
                topCombo.removeFromLeft(4);
                distSubPresetBox.setBounds(topCombo);
                area.removeFromTop(2);
                const int kw = area.getWidth() / 3;
                layoutControlWithLabel(*distDriveKnob, area.removeFromLeft(kw).reduced(2, 0), 11);
                layoutControlWithLabel(*distToneKnob,  area.removeFromLeft(kw).reduced(2, 0), 11);
                layoutControlWithLabel(*distMixKnob,   area.reduced(2, 0), 11);
                break;
            }
            case 3: // Spectral Phaser
            {
                phaserEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                const int kw = area.getWidth() / 4;
                layoutControlWithLabel(*phaserMixKnob,    area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*phaserSpeedKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*phaserWidthKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*phaserOffsetKnob, area.reduced(1, 2), 11);
                break;
            }
            case 4: // Multi-Voice Chorus
            {
                chorusEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                const int kw = area.getWidth() / 5;
                layoutControlWithLabel(*chorusVoicesKnob, area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*chorusRateKnob,   area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*chorusDepthKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*chorusSpreadKnob, area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*chorusMixKnob,    area.reduced(1, 2), 11);
                break;
            }
            case 5: // 3-Band Parametric EQ
            {
                eqEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                auto eqArea = area.withTrimmedTop(26);
                const int kw = eqArea.getWidth() / 7;
                layoutControlWithLabel(*eqLowGainKnob,  eqArea.removeFromLeft(kw).reduced(1, 0), 10);
                layoutControlWithLabel(*eqLowFreqKnob,  eqArea.removeFromLeft(kw).reduced(1, 0), 10);
                layoutControlWithLabel(*eqMidGainKnob,  eqArea.removeFromLeft(kw).reduced(1, 0), 10);
                layoutControlWithLabel(*eqMidFreqKnob,  eqArea.removeFromLeft(kw).reduced(1, 0), 10);
                layoutControlWithLabel(*eqMidQKnob,     eqArea.removeFromLeft(kw).reduced(1, 0), 10);
                layoutControlWithLabel(*eqHighGainKnob, eqArea.removeFromLeft(kw).reduced(1, 0), 10);
                layoutControlWithLabel(*eqHighFreqKnob, eqArea.reduced(1, 0), 10);
                break;
            }
            case 6: // Ping-Pong Delay
            {
                delayEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                const int kw = area.getWidth() / 5;
                layoutControlWithLabel(*delayTimeKnob,     area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*delayFeedbackKnob, area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*delayPingPongKnob, area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*delayFilterKnob,   area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*delayMixKnob,      area.reduced(1, 2), 11);
                break;
            }
            case 7: // Shimmer Reverb
            {
                reverbEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                const int kw = area.getWidth() / 6;
                layoutControlWithLabel(*reverbSizeKnob,     area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*reverbDecayKnob,    area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*reverbPreDelayKnob, area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*reverbDampingKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*reverbShimmerKnob,  area.removeFromLeft(kw).reduced(1, 2), 11);
                layoutControlWithLabel(*reverbMixKnob,      area.reduced(1, 2), 11);
                break;
            }
            case 8: // Multiband OTT / VCA Compressor
            {
                compEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                auto topRow = area.removeFromTop(18);
                compModeBox.setBounds(topRow.reduced(12, 0));
                area.removeFromTop(2);
                const int kw = area.getWidth() / 6;
                layoutControlWithLabel(*compThresholdKnob, area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*compRatioKnob,     area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*compAttackKnob,    area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*compReleaseKnob,   area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*compMakeupKnob,    area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*compMixKnob,       area.reduced(1, 0), 11);
                break;
            }
            case 9: // Zenology Modulation Pack (Ring Mod / Freq Shifter / Auto-Pan / Rotary)
            {
                ringEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                auto topRow = area.removeFromTop(18);
                ringModeBox.setBounds(topRow.reduced(12, 0));
                area.removeFromTop(2);
                const int kw = area.getWidth() / 5;
                layoutControlWithLabel(*ringFreqKnob,   area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*ringDepthKnob,  area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*ringRateKnob,   area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*ringSpreadKnob, area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*ringMixKnob,    area.reduced(1, 0), 11);
                break;
            }
            case 10: // Zenology Lo-Fi Pack (Bitcrusher / Sample Decimator / SP-1200 / Radio)
            {
                crushEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                auto topRow = area.removeFromTop(18);
                crushModeBox.setBounds(topRow.reduced(12, 0));
                area.removeFromTop(2);
                const int kw = area.getWidth() / 5;
                layoutControlWithLabel(*crushBitsKnob,       area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*crushDownsampleKnob, area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*crushJitterKnob,     area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*crushFilterKnob,     area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*crushMixKnob,        area.reduced(1, 0), 11);
                break;
            }
            case 11: // Zenology Flanger Pack (Jet / Through-Zero / Barberpole / Step Flanger)
            default:
            {
                flangerEnableBtn->setBounds(card.getRight() - 48, card.getY() + 3, 42, 16);
                auto topRow = area.removeFromTop(18);
                flangerModeBox.setBounds(topRow.reduced(12, 0));
                area.removeFromTop(2);
                const int kw = area.getWidth() / 5;
                layoutControlWithLabel(*flangerRateKnob,     area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*flangerDepthKnob,    area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*flangerFeedbackKnob, area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*flangerSpreadKnob,   area.removeFromLeft(kw).reduced(1, 0), 11);
                layoutControlWithLabel(*flangerMixKnob,      area.reduced(1, 0), 11);
                break;
            }
        }
    }

    // =========================================================================
    // 3. TIME MODULATION TAB LAYOUT (currentTab == 2)
    // =========================================================================
    {
        const int tmEditorW = 780;
        timeModEditor.setBounds(margin, topY, tmEditorW, upperH);
        cardTimeModParams = { margin + tmEditorW + gap, topY, getWidth() - (margin + tmEditorW + gap) - margin, upperH };

        timeModEnableBtn->setBounds(cardTimeModParams.getRight() - 88, cardTimeModParams.getY() + 3, 82, 16);

        auto tmArea = cardTimeModParams.reduced(12, 30);
        auto comboRow = tmArea.removeFromTop(24);
        const int halfC = (comboRow.getWidth() - 8) / 2;
        timeModModeBox.setBounds(comboRow.removeFromLeft(halfC));
        comboRow.removeFromLeft(8);
        timeModRateBox.setBounds(comboRow);

        // Skip space reserved for Circular Buffer Telemetry box
        tmArea.removeFromTop(84);

        const int rowH = tmArea.getHeight() / 2;
        auto r1 = tmArea.removeFromTop(rowH);
        auto r2 = tmArea;

        const int kw = r1.getWidth() / 3;
        layoutControlWithLabel(*timeModDepthKnob,   r1.removeFromLeft(kw).reduced(6, 4), 14);
        layoutControlWithLabel(*timeModStutterKnob, r1.removeFromLeft(kw).reduced(6, 4), 14);
        layoutControlWithLabel(*timeModSpeedKnob,   r1.reduced(6, 4), 14);

        layoutControlWithLabel(*timeModSmoothKnob,  r2.removeFromLeft(kw).reduced(6, 4), 14);
        layoutControlWithLabel(*timeModFilterKnob,  r2.removeFromLeft(kw).reduced(6, 4), 14);
        layoutControlWithLabel(*timeModMixKnob,     r2.reduced(6, 4), 14);
    }

    // =========================================================================
    // 4. DEDICATED PRESET BROWSER TAB LAYOUT (currentTab == 3)
    // =========================================================================
    presetBrowserView.setBounds(margin, topY, getWidth() - margin * 2, upperH);

    // =========================================================================
    // 5. BOTTOM PANEL: A/B SYSTEM, ARTICULATOR + AHDSR / ARP, MASTER & PRESETS
    // =========================================================================
    const int abW = 236;
    const int masterW = 296;
    const int artW = getWidth() - margin * 2 - abW - masterW - gap * 2;

    cardABSystem    = { margin, bottomY, abW, bottomH };
    cardArticulator = { margin + abW + gap, bottomY, artW, bottomH };
    cardMasterOut   = { margin + abW + gap + artW + gap, bottomY, masterW, bottomH };

    // Card 08: Redesigned A/B Part System
    {
        auto area = cardABSystem.reduced(10, 26);

        auto tabRow = area.removeFromTop(22);
        const int btnW = (tabRow.getWidth() - 44) / 2;
        partAButton.setBounds(tabRow.removeFromLeft(btnW));
        tabRow.removeFromLeft(4);
        partBButton.setBounds(tabRow.removeFromLeft(btnW));
        tabRow.removeFromLeft(4);
        copyABButton.setBounds(tabRow);

        area.removeFromTop(24);
        layoutControlWithLabel(*abMixFader, area.removeFromTop(34), 12);

        area.removeFromTop(2);
        const int kw = area.getWidth() / 2;
        layoutControlWithLabel(*velLinkKnob, area.removeFromLeft(kw).reduced(4, 0));
        layoutControlWithLabel(*partPanKnob, area.reduced(4, 0));
    }

    // Card 09: Switchable ShaperBox Articulator (+ Serum-2 Physical AHDSR Knobs) vs Hive-2 Arpeggiator
    {
        arpEnableBtn->setBounds(cardArticulator.getRight() - 66, cardArticulator.getY() + 3, 60, 16);
        modeArpSeqBtn.setBounds(cardArticulator.getRight() - 196, cardArticulator.getY() + 3, 126, 16);
        modeArticulatorBtn.setBounds(cardArticulator.getRight() - 326, cardArticulator.getY() + 3, 126, 16);

        auto inner = cardArticulator.reduced(8, 25).withTrimmedTop(2);

        if (bottomCenterMode == 0)
        {
            // Left Strip: 5 Dedicated Physical AHDSR Knobs (Serum 2 Envelope Layout)
            auto ahdsrStrip = inner.removeFromBottom(58);
            inner.removeFromBottom(2);
            articulatorEditor.setBounds(inner);

            const int kw = ahdsrStrip.getWidth() / 5;
            layoutControlWithLabel(*envAttackKnob,  ahdsrStrip.removeFromLeft(kw).reduced(6, 0), 11);
            layoutControlWithLabel(*envHoldKnob,    ahdsrStrip.removeFromLeft(kw).reduced(6, 0), 11);
            layoutControlWithLabel(*envDecayKnob,   ahdsrStrip.removeFromLeft(kw).reduced(6, 0), 11);
            layoutControlWithLabel(*envSustainKnob, ahdsrStrip.removeFromLeft(kw).reduced(6, 0), 11);
            layoutControlWithLabel(*envReleaseKnob, ahdsrStrip.reduced(6, 0), 11);
        }
        else
        {
            auto leftPanel = inner.removeFromLeft(162);
            inner.removeFromLeft(6);

            auto comboRow1 = leftPanel.removeFromTop(20);
            arpModeBox.setBounds(comboRow1.removeFromLeft(94));
            comboRow1.removeFromLeft(4);
            arpRateBox.setBounds(comboRow1);

            leftPanel.removeFromTop(4);
            const int kw = leftPanel.getWidth() / 3;
            layoutControlWithLabel(*arpOctavesKnob, leftPanel.removeFromLeft(kw).reduced(1, 2), 12);
            layoutControlWithLabel(*arpGateKnob,    leftPanel.removeFromLeft(kw).reduced(1, 2), 12);
            layoutControlWithLabel(*arpSwingKnob,   leftPanel.reduced(1, 2), 12);

            arpStepGrid.setBounds(inner);
        }
    }

    // Card 10: Master Output, Limiter & Dynamics
    {
        limiterEnableBtn->setBounds(cardMasterOut.getRight() - 58, cardMasterOut.getY() + 3, 52, 16);

        auto area = cardMasterOut.reduced(8, 26).withTrimmedTop(6);
        masterMeterRect = area.removeFromTop(56);
        area.removeFromTop(16);

        const int kw = area.getWidth() / 4;
        layoutControlWithLabel(*strumTimeKnob,     area.removeFromLeft(kw).reduced(2, 0), 12);
        layoutControlWithLabel(*strumTensionKnob,  area.removeFromLeft(kw).reduced(2, 0), 12);
        layoutControlWithLabel(*masterGainKnob,    area.removeFromLeft(kw).reduced(2, 0), 12);
        layoutControlWithLabel(*limiterThreshKnob, area.reduced(2, 0), 12);
    }
}
