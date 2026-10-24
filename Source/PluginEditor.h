#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/RapidLookAndFeel.h"
#include "UI/ArticulatorEditor.h"
#include "UI/Visualizer3D.h"
#include <vector>
#include <memory>

class PrismorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit PrismorAudioProcessorEditor(PrismorAudioProcessor&);
    ~PrismorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void setActiveTab(int tabIdx);
    void setBottomCenterMode(int modeIdx); // 0 = Shaper Articulator + AHDSR, 1 = Arpeggiator & 16-Step Seq
    void applyValhallaTheme(int themeIdx);
    void showAddFxMenu();
    void updateFxControlsVisibility();
    void syncWaveSelectorsFromEngine();

    struct ParamControl
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String title;
        juce::Rectangle<int> labelBounds;
        int tabGroup = 0; // 0 = Synth Tab, 1 = FX Tab, 2 = Always Visible (Bottom), 3 = Arp Panel, 4 = Time Mod Tab, 5 = Articulator AHDSR
        int fxModuleId = -1; // 0..11 for FX module ownership
    };

    struct ToggleControl
    {
        std::unique_ptr<juce::ToggleButton> button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
        int tabGroup = 0;
        int fxModuleId = -1;
    };

    juce::Slider& addKnob(const juce::String& paramId, const juce::String& labelText,
                          const juce::String& accentColor, bool isBipolar = false, int tabGroup = 0, int fxModuleId = -1);
    juce::Slider& addPitchBox(const juce::String& paramId, const juce::String& boxTitle,
                              const juce::String& boxSuffix, int decimals,
                              const juce::String& accentColor, int tabGroup = 0);
    juce::Slider& addVerticalFader(const juce::String& paramId, const juce::String& labelText,
                                   const juce::String& accentColor, int tabGroup = 0);
    juce::Slider& addHorizontalFader(const juce::String& paramId, const juce::String& labelText,
                                     const juce::String& accentColor, bool isABCrossfader = false, int tabGroup = 2);
    juce::ToggleButton& addBypassToggle(const juce::String& paramId, const juce::String& buttonText,
                                        const juce::String& accentColor, int tabGroup = 0, int fxModuleId = -1);

    void layoutControlWithLabel(juce::Slider& slider, juce::Rectangle<int> cell, int labelHeight = 14);
    void drawModuleCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title,
                        juce::Colour accent, bool isEnabled = true, bool drawFxHeaderButtons = false, bool isDragged = false);

    PrismorAudioProcessor& audioProcessor;
    PrismorUI::RapidLookAndFeel rapidLookAndFeel;

    std::vector<std::unique_ptr<ParamControl>> paramControls;
    std::vector<std::unique_ptr<ToggleControl>> toggleControls;

    // Top-Bar Navigation Tabs (0 = SYNTH ENGINE, 1 = FX RACK, 2 = TIME MOD, 3 = PRESET BROWSER)
    int currentTab { 0 };
    int bottomCenterMode { 0 }; // 0 = Shaper Articulator + AHDSR, 1 = Arpeggiator & Step Seq
    juce::TextButton tabSynthBtn   { "SYNTH ENGINE" };
    juce::TextButton tabFxBtn      { "FX RACK [12]" };
    juce::TextButton tabTimeModBtn { "TIME MOD" };
    juce::TextButton tabPresetsBtn { "PRESET BROWSER" };
    juce::ComboBox   themeSelector;

    // Top-Right Master Volume & Hardware Macro Knobs (Tier 1 Brushed Silver Bar)
    std::unique_ptr<juce::Slider> headerMacro1Knob;
    std::unique_ptr<juce::Slider> headerMacro2Knob;
    std::unique_ptr<juce::Slider> headerThreshKnob;
    std::unique_ptr<juce::Slider> headerMasterKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> headerMacro1Attach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> headerMacro2Attach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> headerThreshAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> headerMasterAttach;

    juce::Rectangle<int> topSilverHeaderRect;
    juce::Rectangle<int> topPresetSlotRect;
    juce::Rectangle<int> topMeterRect;
    juce::Rectangle<int> topActiveLayerRect;
    juce::Rectangle<int> topTelemetryRect;
    juce::Rectangle<int> topPanicRect;

    // Phase 7.2 In-App Automated Self-Updater
    enum class UpdateState
    {
        Hidden,
        UpdateAvailable,
        Downloading,
        Extracting,
        InstalledRestartReady,
        InstalledReloadDAW,
        ErrorFallback
    };

    class UpdateNotificationButton : public juce::Button
    {
    public:
        UpdateNotificationButton();
        void setUpdateState(UpdateState newState, const juce::String& ver = {}, float progressNorm = 0.0f);
        UpdateState getUpdateState() const { return currentState; }
        const juce::String& getVersionText() const { return versionStr; }
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    private:
        UpdateState currentState { UpdateState::Hidden };
        juce::String versionStr;
        float progress { 0.0f };
    };

    class UpdateCheckerThread : public juce::Thread
    {
    public:
        explicit UpdateCheckerThread(std::function<void(const juce::String&)> onNewVersionFound);
        ~UpdateCheckerThread() override;
        void run() override;
    private:
        std::function<void(const juce::String&)> callback;
        static bool isRemoteVersionNewer(juce::String remoteTag, juce::String currentVer);
    };

    class SelfUpdateWorkerThread : public juce::Thread
    {
    public:
        using ProgressCallback = std::function<void(float progressNorm, const juce::String& statusMsg)>;
        using CompletionCallback = std::function<void(bool success, bool isStandalone, const juce::String& errorMsg)>;

        SelfUpdateWorkerThread(bool isStandalone, ProgressCallback onProg, CompletionCallback onDone);
        ~SelfUpdateWorkerThread() override;
        void run() override;

    private:
        bool isStandaloneHost { false };
        ProgressCallback progressCb;
        CompletionCallback doneCb;
    };

    UpdateNotificationButton updateAvailableBtn;
    std::unique_ptr<UpdateCheckerThread> updateCheckerThread;
    std::unique_ptr<SelfUpdateWorkerThread> selfUpdateWorkerThread;

    void triggerSelfUpdate();
    void applyRestartOrRelaunch();

    // -------------------------------------------------------------------------
    // SYNTH TAB CONTROLS (tabGroup = 0)
    // -------------------------------------------------------------------------
    // Card 01: Frequency Field 1 (Part A) + Serum-2 Pitch Boxes + Zenology Wave Selector
    juce::Slider* oscAOctBox        = nullptr;
    juce::Slider* oscASemiBox       = nullptr;
    juce::Slider* oscAFineBox       = nullptr;
    juce::Slider* oscACoarseBox     = nullptr;
    juce::Slider* startPhaseKnob    = nullptr;
    juce::Slider* phaseRandomKnob   = nullptr;
    juce::Slider* vibDepthKnob      = nullptr;
    juce::Slider* harmScaleKnob     = nullptr;
    juce::Slider* harmTiltKnob      = nullptr;
    juce::Slider* warpAmountKnob    = nullptr;
    juce::ComboBox oscAWaveBox;
    juce::ComboBox warpModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> warpModeAttach;
    juce::Slider* timbreFader       = nullptr;

    // Card 02: Frequency Field 2 (Part B) + Serum-2 Pitch Boxes + Zenology Wave Selector
    juce::Slider* oscBOctBox        = nullptr;
    juce::Slider* oscBSemiBox       = nullptr;
    juce::Slider* oscBFineBox       = nullptr;
    juce::Slider* oscBCoarseBox     = nullptr;
    juce::Slider* vibSpeedKnob      = nullptr;
    juce::Slider* oscBPhaseKnob     = nullptr;
    juce::Slider* pluckTimeSynthKnob= nullptr;
    juce::Slider* harmScaleBKnob    = nullptr;
    juce::Slider* harmTiltBKnob     = nullptr;
    juce::Slider* warpAmountBKnob   = nullptr;
    juce::ComboBox oscBWaveBox;
    juce::ComboBox warpModeBBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> warpModeBAttach;
    juce::Slider* timbreBFader      = nullptr;

    // Card 03: Prism & Harmonizer
    juce::ToggleButton* prismEnableBtn = nullptr;
    juce::Slider* prismAmountKnob   = nullptr;
    juce::ComboBox prismModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> prismModeAttach;
    juce::Slider* harmShiftFader    = nullptr;
    juce::Slider* harmGapFader      = nullptr;
    juce::Slider* harmWidthFader    = nullptr;

    // Card 04: Voicing, True Legato & 16x Serum-2 Unison
    juce::ComboBox voicingModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voicingModeAttach;
    juce::ComboBox unisonStackModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> unisonStackModeAttach;
    juce::ToggleButton* autoGlideBtn = nullptr;
    juce::Slider* maxVoicesKnob     = nullptr;
    juce::Slider* pitchFreqKnob     = nullptr;
    juce::Slider* portaTimeKnob     = nullptr;
    juce::Slider* portaCurveKnob    = nullptr;
    juce::Slider* unisonVoicesKnob  = nullptr;
    juce::Slider* unisonDetuneKnob  = nullptr;
    juce::Slider* unisonSpreadKnob  = nullptr;
    juce::Slider* unisonBlendKnob   = nullptr;
    juce::Slider* unisonPhaseKnob   = nullptr;
    juce::TextButton unisonDetuneLinkBtn { "LINK" };
    bool unisonDetuneLinkActive { true };

    // Card 05: Dual Spectral Filter, Vocoder/Vox/Talkbox & Sub-Presets
    juce::ComboBox filterSubPresetBox;
    juce::ToggleButton* filter1EnableBtn = nullptr;
    juce::ComboBox filter1TypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filter1TypeAttach;
    juce::Slider* filter1CutoffKnob = nullptr;
    juce::Slider* filter1ResKnob    = nullptr;
    juce::Slider* filter1KeyKnob    = nullptr;
    juce::Slider* filter1WidthKnob  = nullptr;

    juce::ToggleButton* filter2EnableBtn = nullptr;
    juce::ComboBox filter2TypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filter2TypeAttach;
    juce::Slider* filter2CutoffKnob = nullptr;
    juce::Slider* filter2ResKnob    = nullptr;
    juce::Slider* filter2KeyKnob    = nullptr;
    juce::Slider* filter2WidthKnob  = nullptr;

    juce::Slider* filterRoutingKnob = nullptr;
    juce::Slider* filterDriveKnob   = nullptr;

    // Card 06: Dedicated Serum-2 Style Noise Generator
    juce::ToggleButton* noiseEnableBtn = nullptr;
    juce::ComboBox noiseTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> noiseTypeAttach;
    juce::Slider* noiseLevelKnob    = nullptr;
    juce::Slider* noisePitchKnob    = nullptr;
    juce::Slider* noiseKeyTrackKnob = nullptr;
    juce::Slider* noisePhaseKnob    = nullptr;
    juce::Slider* noisePanKnob      = nullptr;
    juce::Slider* noiseFilterKnob   = nullptr;

    // -------------------------------------------------------------------------
    // FX RACK TAB CONTROLS (Dynamic 12-Slot FX Rack with Add/Remove/Drag-Drop)
    // -------------------------------------------------------------------------
    // FX 0: Blur & Pluck
    juce::ToggleButton* blurEnableBtn = nullptr;
    juce::Slider* blurTimeKnob      = nullptr;
    juce::Slider* blurHarmKnob      = nullptr;
    juce::ToggleButton* pluckEnableBtn = nullptr;
    juce::Slider* pluckTimeKnob     = nullptr;
    juce::Slider* pluckBlurKnob     = nullptr;

    // FX 1: Tremolo
    juce::ToggleButton* tremEnableBtn = nullptr;
    juce::Slider* tremDepthKnob     = nullptr;
    juce::Slider* tremSpeedKnob     = nullptr;
    juce::Slider* tremGapKnob       = nullptr;

    // FX 2: Distortion / Saturation + Sub-Preset Selector
    juce::ToggleButton* distEnableBtn = nullptr;
    juce::ComboBox distTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distTypeAttach;
    juce::ComboBox distSubPresetBox;
    juce::Slider* distDriveKnob     = nullptr;
    juce::Slider* distToneKnob      = nullptr;
    juce::Slider* distMixKnob       = nullptr;

    // FX 3: Phaser
    juce::ToggleButton* phaserEnableBtn = nullptr;
    juce::Slider* phaserMixKnob     = nullptr;
    juce::Slider* phaserWidthKnob   = nullptr;
    juce::Slider* phaserOffsetKnob  = nullptr;
    juce::Slider* phaserSpeedKnob   = nullptr;

    // FX 4: Chorus
    juce::ToggleButton* chorusEnableBtn = nullptr;
    juce::Slider* chorusVoicesKnob  = nullptr;
    juce::Slider* chorusRateKnob    = nullptr;
    juce::Slider* chorusDepthKnob   = nullptr;
    juce::Slider* chorusSpreadKnob  = nullptr;
    juce::Slider* chorusMixKnob     = nullptr;

    // FX 5: 3-Band Parametric EQ
    juce::ToggleButton* eqEnableBtn = nullptr;
    juce::Slider* eqLowGainKnob     = nullptr;
    juce::Slider* eqMidGainKnob     = nullptr;
    juce::Slider* eqHighGainKnob    = nullptr;
    juce::Slider* eqLowFreqKnob     = nullptr;
    juce::Slider* eqMidFreqKnob     = nullptr;
    juce::Slider* eqMidQKnob        = nullptr;
    juce::Slider* eqHighFreqKnob    = nullptr;

    // FX 6: Stereo / Ping-Pong Delay
    juce::ToggleButton* delayEnableBtn = nullptr;
    juce::Slider* delayTimeKnob     = nullptr;
    juce::Slider* delayFeedbackKnob = nullptr;
    juce::Slider* delayPingPongKnob = nullptr;
    juce::Slider* delayFilterKnob   = nullptr;
    juce::Slider* delayMixKnob      = nullptr;

    // FX 7: Shimmer / Hall Reverb
    juce::ToggleButton* reverbEnableBtn = nullptr;
    juce::Slider* reverbSizeKnob    = nullptr;
    juce::Slider* reverbDecayKnob   = nullptr;
    juce::Slider* reverbPreDelayKnob= nullptr;
    juce::Slider* reverbDampingKnob = nullptr;
    juce::Slider* reverbShimmerKnob = nullptr;
    juce::Slider* reverbMixKnob     = nullptr;

    // FX 8: Multiband OTT / Single-Band Compressor
    juce::ToggleButton* compEnableBtn = nullptr;
    juce::ComboBox compModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> compModeAttach;
    juce::Slider* compThresholdKnob = nullptr;
    juce::Slider* compRatioKnob     = nullptr;
    juce::Slider* compAttackKnob    = nullptr;
    juce::Slider* compReleaseKnob   = nullptr;
    juce::Slider* compMakeupKnob    = nullptr;
    juce::Slider* compMixKnob       = nullptr;

    // FX 9: Zenology Modulation Pack (Ring Mod / Freq Shifter / Auto-Pan / Rotary)
    juce::ToggleButton* ringEnableBtn = nullptr;
    juce::ComboBox ringModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ringModeAttach;
    juce::Slider* ringFreqKnob      = nullptr;
    juce::Slider* ringDepthKnob     = nullptr;
    juce::Slider* ringRateKnob      = nullptr;
    juce::Slider* ringSpreadKnob    = nullptr;
    juce::Slider* ringMixKnob       = nullptr;

    // FX 10: Zenology Lo-Fi Pack (Bitcrusher / Sample Decimator / SP-1200 / Radio)
    juce::ToggleButton* crushEnableBtn = nullptr;
    juce::ComboBox crushModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> crushModeAttach;
    juce::Slider* crushBitsKnob       = nullptr;
    juce::Slider* crushDownsampleKnob = nullptr;
    juce::Slider* crushJitterKnob     = nullptr;
    juce::Slider* crushFilterKnob     = nullptr;
    juce::Slider* crushMixKnob        = nullptr;

    // FX 11: Zenology Flanger Pack (Jet / Through-Zero / Barberpole / Step Flanger)
    juce::ToggleButton* flangerEnableBtn = nullptr;
    juce::ComboBox flangerModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> flangerModeAttach;
    juce::Slider* flangerRateKnob     = nullptr;
    juce::Slider* flangerDepthKnob    = nullptr;
    juce::Slider* flangerFeedbackKnob = nullptr;
    juce::Slider* flangerSpreadKnob   = nullptr;
    juce::Slider* flangerMixKnob      = nullptr;

    // -------------------------------------------------------------------------
    // TIME MODULATION TAB CONTROLS (tabGroup = 4)
    // -------------------------------------------------------------------------
    PrismorUI::TimeModCurveEditor timeModEditor;
    juce::ToggleButton* timeModEnableBtn = nullptr;
    juce::ComboBox timeModModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> timeModModeAttach;
    juce::ComboBox timeModRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> timeModRateAttach;
    juce::Slider* timeModDepthKnob   = nullptr;
    juce::Slider* timeModStutterKnob = nullptr;
    juce::Slider* timeModSpeedKnob   = nullptr;
    juce::Slider* timeModSmoothKnob  = nullptr;
    juce::Slider* timeModFilterKnob  = nullptr;
    juce::Slider* timeModMixKnob     = nullptr;
    juce::Rectangle<int> cardTimeModParams;

    // -------------------------------------------------------------------------
    // DEDICATED PRESET BROWSER TAB (currentTab == 3)
    // -------------------------------------------------------------------------
    PrismorUI::PresetBrowserView presetBrowserView;

    // -------------------------------------------------------------------------
    // BOTTOM PANEL (Always Visible: A/B System, Articulator/Arp, Master & Presets)
    // -------------------------------------------------------------------------
    juce::TextButton partAButton  { "EDIT PART A" };
    juce::TextButton partBButton  { "EDIT PART B" };
    juce::TextButton copyABButton { "A > B" };

    juce::Slider* abMixFader        = nullptr;
    juce::Slider* velLinkKnob       = nullptr;
    juce::Slider* partPanKnob       = nullptr;
    juce::Slider* strumTimeKnob     = nullptr;
    juce::Slider* strumTensionKnob  = nullptr;
    juce::Slider* masterGainKnob    = nullptr;
    juce::Slider* limiterThreshKnob = nullptr;
    juce::ToggleButton* limiterEnableBtn = nullptr;

    juce::TextButton prevPresetBtn { "<" };
    juce::TextButton nextPresetBtn { ">" };
    juce::TextButton initPresetBtn { "INIT" };
    juce::ComboBox   presetSelector;

    // Card 09 Mode Switcher: ShaperBox Articulator vs Hive-2 Arpeggiator & Step Seq
    juce::TextButton modeArticulatorBtn { "SHAPER ARTICULATOR" };
    juce::TextButton modeArpSeqBtn      { "ARP & 16-STEP SEQ" };

    PrismorUI::ArticulatorEditor articulatorEditor;
    PrismorUI::ArpeggiatorStepGrid arpStepGrid;

    // Serum-2 Style Physical AHDSR Envelope Knobs inside Card 09 (tabGroup = 5)
    juce::Slider* envAttackKnob  = nullptr;
    juce::Slider* envHoldKnob    = nullptr;
    juce::Slider* envDecayKnob   = nullptr;
    juce::Slider* envSustainKnob = nullptr;
    juce::Slider* envReleaseKnob = nullptr;

    // Arpeggiator & Step Sequencer Header Controls (tabGroup = 3)
    juce::ToggleButton* arpEnableBtn = nullptr;
    juce::ComboBox arpModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpModeAttach;
    juce::ComboBox arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpRateAttach;
    juce::Slider* arpOctavesKnob = nullptr;
    juce::Slider* arpGateKnob    = nullptr;
    juce::Slider* arpSwingKnob   = nullptr;

    PrismorUI::Visualizer3D visualizer3D;

    // Layout Card Bounds (Synth Tab)
    juce::Rectangle<int> cardField1;
    juce::Rectangle<int> cardField2;
    juce::Rectangle<int> cardPrismHarm;
    juce::Rectangle<int> cardPitchArt;
    juce::Rectangle<int> cardDualFilter;
    juce::Rectangle<int> cardNoiseGen;

    // Layout Card Bounds (FX Tab - 12-Slot 4x3 Grid)
    juce::Rectangle<int> fxChainStripRect;
    std::array<juce::Rectangle<int>, 12> fxSlotBounds {}; // Bounds of the 12 visual slots (0..11)
    int selectedFxSlot { 0 };
    int draggingFxSlot { -1 };
    int dragHoverFxSlot { -1 };

    // Layout Card Bounds (Bottom Panel)
    juce::Rectangle<int> cardABSystem;
    juce::Rectangle<int> cardArticulator;
    juce::Rectangle<int> cardMasterOut;
    juce::Rectangle<int> masterMeterRect;

    // Smoothed meter values for UI
    float smoothedPeakL = 0.0f;
    float smoothedPeakR = 0.0f;
    float smoothedRmsL  = 0.0f;
    float smoothedRmsR  = 0.0f;
    float smoothedGrDb  = 0.0f;
    float smoothedCpu   = 0.0f;
    int   displayedVoiceCount = 0;
    float noiseAnimPhase = 0.0f;
    int screenshotFrameCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismorAudioProcessorEditor)
};
