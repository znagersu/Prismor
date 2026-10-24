#pragma once
#include <JuceHeader.h>
#include "DSP/SpectralSynthEngine.h"

class PrismorAudioProcessor : public juce::AudioProcessor
{
public:
    PrismorAudioProcessor();
    ~PrismorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    PrismorDSP::SpectralSynthEngine& getSynthEngine() noexcept { return synthEngine; }
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

    float getCpuUsagePercent() const noexcept
    {
        return static_cast<float>(loadMeasurer.getLoadAsPercentage());
    }

    PrismorDSP::SynthParameters getCurrentSynthParameters() const noexcept { return readParametersFromAPVTS(); }

    struct FactoryPresetMetadata
    {
        int index = 0;
        juce::String name;
        juce::String category;
        juce::String author;
        juce::String wavesUsed;
        juce::String fxUsed;
        juce::String description;
        bool isUserPreset = false;
        juce::File userFile;
    };

    void loadFactoryPreset(int index);
    int getCurrentPresetIndex() const noexcept { return currentPreset; }
    juce::StringArray getFactoryPresetNames() const;
    std::vector<FactoryPresetMetadata> getFactoryPresetBank() const;

    juce::File getUserPresetDirectory() const;
    bool saveUserPreset(const juce::String& name, const juce::String& category, const juce::String& author, const juce::String& description);
    bool exportPresetToFile(const juce::File& file, const juce::String& name, const juce::String& category, const juce::String& author, const juce::String& description);
    bool importPresetFromFile(const juce::File& file);
    bool loadPresetFromXml(const juce::XmlElement& xml);

    void loadFilterSubPreset(int subPresetIndex);
    void loadDistortionSubPreset(int subPresetIndex);
    juce::StringArray getFilterSubPresetNames() const;
    juce::StringArray getDistortionSubPresetNames() const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct CachedParamPointers
    {
        std::atomic<float>* oscAOct = nullptr;
        std::atomic<float>* oscASemi = nullptr;
        std::atomic<float>* oscAFine = nullptr;
        std::atomic<float>* oscACoarse = nullptr;
        std::atomic<float>* startPhase = nullptr;
        std::atomic<float>* phaseRandom = nullptr;
        std::atomic<float>* timbre = nullptr;
        std::atomic<float>* warpMode = nullptr;
        std::atomic<float>* warpAmount = nullptr;
        std::atomic<float>* harmonicScale = nullptr;
        std::atomic<float>* harmonicTilt = nullptr;

        std::atomic<float>* oscBOct = nullptr;
        std::atomic<float>* oscBSemi = nullptr;
        std::atomic<float>* oscBFine = nullptr;
        std::atomic<float>* oscBPitch = nullptr;
        std::atomic<float>* oscBPhase = nullptr;
        std::atomic<float>* timbreB = nullptr;
        std::atomic<float>* warpModeB = nullptr;
        std::atomic<float>* warpAmountB = nullptr;
        std::atomic<float>* harmonicScaleB = nullptr;
        std::atomic<float>* harmonicTiltB = nullptr;

        std::atomic<float>* prismEnable = nullptr;
        std::atomic<float>* prismAmount = nullptr;
        std::atomic<float>* prismMode = nullptr;
        std::atomic<float>* harmShift = nullptr;
        std::atomic<float>* harmGap = nullptr;
        std::atomic<float>* harmWidth = nullptr;

        std::atomic<float>* pluckEnable = nullptr;
        std::atomic<float>* pluckTime = nullptr;
        std::atomic<float>* pluckBlur = nullptr;

        std::atomic<float>* voicingMode = nullptr;
        std::atomic<float>* maxVoices = nullptr;
        std::atomic<float>* pitchFreq = nullptr;
        std::atomic<float>* portaTime = nullptr;
        std::atomic<float>* portaCurve = nullptr;
        std::atomic<float>* autoGlide = nullptr;

        std::atomic<float>* unisonVoices = nullptr;
        std::atomic<float>* unisonDetune = nullptr;
        std::atomic<float>* unisonSpread = nullptr;
        std::atomic<float>* unisonBlend = nullptr;
        std::atomic<float>* unisonPhase = nullptr;
        std::atomic<float>* unisonStackMode = nullptr;

        std::atomic<float>* vibDepth = nullptr;
        std::atomic<float>* vibSpeed = nullptr;

        std::atomic<float>* filter1Enable = nullptr;
        std::atomic<float>* filter1Type = nullptr;
        std::atomic<float>* filter1Cutoff = nullptr;
        std::atomic<float>* filter1Res = nullptr;
        std::atomic<float>* filter1KeyTrack = nullptr;
        std::atomic<float>* filter1Width = nullptr;

        std::atomic<float>* filter2Enable = nullptr;
        std::atomic<float>* filter2Type = nullptr;
        std::atomic<float>* filter2Cutoff = nullptr;
        std::atomic<float>* filter2Res = nullptr;
        std::atomic<float>* filter2KeyTrack = nullptr;
        std::atomic<float>* filter2Width = nullptr;

        std::atomic<float>* filterRouting = nullptr;
        std::atomic<float>* filterDrive = nullptr;

        std::atomic<float>* noiseEnable = nullptr;
        std::atomic<float>* noiseType = nullptr;
        std::atomic<float>* noiseLevel = nullptr;
        std::atomic<float>* noisePitch = nullptr;
        std::atomic<float>* noiseKeyTrack = nullptr;
        std::atomic<float>* noisePhase = nullptr;
        std::atomic<float>* noisePan = nullptr;
        std::atomic<float>* noiseFilter = nullptr;

        std::atomic<float>* arpEnable = nullptr;
        std::atomic<float>* arpMode = nullptr;
        std::atomic<float>* arpOctaves = nullptr;
        std::atomic<float>* arpRate = nullptr;
        std::atomic<float>* arpGate = nullptr;
        std::atomic<float>* arpSwing = nullptr;

        std::atomic<float>* blurEnable = nullptr;
        std::atomic<float>* blurTime = nullptr;
        std::atomic<float>* blurHarmonic = nullptr;

        std::atomic<float>* tremEnable = nullptr;
        std::atomic<float>* tremDepth = nullptr;
        std::atomic<float>* tremSpeed = nullptr;
        std::atomic<float>* tremGap = nullptr;

        std::atomic<float>* distEnable = nullptr;
        std::atomic<float>* distType = nullptr;
        std::atomic<float>* distDrive = nullptr;
        std::atomic<float>* distTone = nullptr;
        std::atomic<float>* distMix = nullptr;

        std::atomic<float>* phaserEnable = nullptr;
        std::atomic<float>* phaserMix = nullptr;
        std::atomic<float>* phaserWidth = nullptr;
        std::atomic<float>* phaserOffset = nullptr;
        std::atomic<float>* phaserSpeed = nullptr;

        std::atomic<float>* chorusEnable = nullptr;
        std::atomic<float>* chorusVoices = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusSpread = nullptr;
        std::atomic<float>* chorusMix = nullptr;

        std::atomic<float>* eqEnable = nullptr;
        std::atomic<float>* eqLowFreq = nullptr;
        std::atomic<float>* eqLowGain = nullptr;
        std::atomic<float>* eqMidFreq = nullptr;
        std::atomic<float>* eqMidGain = nullptr;
        std::atomic<float>* eqMidQ = nullptr;
        std::atomic<float>* eqHighFreq = nullptr;
        std::atomic<float>* eqHighGain = nullptr;

        std::atomic<float>* delayEnable = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayPingPong = nullptr;
        std::atomic<float>* delayFilter = nullptr;
        std::atomic<float>* delayMix = nullptr;

        std::atomic<float>* reverbEnable = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDecay = nullptr;
        std::atomic<float>* reverbPreDelay = nullptr;
        std::atomic<float>* reverbDamping = nullptr;
        std::atomic<float>* reverbShimmer = nullptr;
        std::atomic<float>* reverbMix = nullptr;

        std::atomic<float>* compEnable = nullptr;
        std::atomic<float>* compMode = nullptr;
        std::atomic<float>* compThreshold = nullptr;
        std::atomic<float>* compRatio = nullptr;
        std::atomic<float>* compAttack = nullptr;
        std::atomic<float>* compRelease = nullptr;
        std::atomic<float>* compMakeup = nullptr;
        std::atomic<float>* compMix = nullptr;

        std::atomic<float>* ringEnable = nullptr;
        std::atomic<float>* ringMode = nullptr;
        std::atomic<float>* ringFreq = nullptr;
        std::atomic<float>* ringDepth = nullptr;
        std::atomic<float>* ringLfoRate = nullptr;
        std::atomic<float>* ringLfoDepth = nullptr;
        std::atomic<float>* ringMix = nullptr;

        std::atomic<float>* crushEnable = nullptr;
        std::atomic<float>* crushMode = nullptr;
        std::atomic<float>* crushBits = nullptr;
        std::atomic<float>* crushDownsample = nullptr;
        std::atomic<float>* crushDrive = nullptr;
        std::atomic<float>* crushTone = nullptr;
        std::atomic<float>* crushMix = nullptr;

        std::atomic<float>* flangerEnable = nullptr;
        std::atomic<float>* flangerMode = nullptr;
        std::atomic<float>* flangerRate = nullptr;
        std::atomic<float>* flangerDepth = nullptr;
        std::atomic<float>* flangerFeedback = nullptr;
        std::atomic<float>* flangerManual = nullptr;
        std::atomic<float>* flangerMix = nullptr;

        std::atomic<float>* bodeEnable = nullptr;
        std::atomic<float>* bodeShift = nullptr;
        std::atomic<float>* bodeFeedback = nullptr;
        std::atomic<float>* bodeMix = nullptr;

        std::atomic<float>* timeModEnable = nullptr;
        std::atomic<float>* timeModMode = nullptr;
        std::atomic<float>* timeModRate = nullptr;
        std::atomic<float>* timeModDepth = nullptr;
        std::atomic<float>* timeModStutter = nullptr;
        std::atomic<float>* timeModSpeed = nullptr;
        std::atomic<float>* timeModSmooth = nullptr;
        std::atomic<float>* timeModFilter = nullptr;
        std::atomic<float>* timeModMix = nullptr;

        std::atomic<float>* envAttack = nullptr;
        std::atomic<float>* envHold = nullptr;
        std::atomic<float>* envDecay = nullptr;
        std::atomic<float>* envSustain = nullptr;
        std::atomic<float>* envRelease = nullptr;

        std::atomic<float>* abMix = nullptr;
        std::atomic<float>* velLink = nullptr;
        std::atomic<float>* partPan = nullptr;
        std::atomic<float>* strumTime = nullptr;
        std::atomic<float>* strumTension = nullptr;

        std::atomic<float>* masterGainDb = nullptr;
        std::atomic<float>* limiterEnable = nullptr;
        std::atomic<float>* limiterThreshDb = nullptr;
    };

private:
    void initCachedParamPointers();
    CachedParamPointers cachedParams;
    PrismorDSP::SynthParameters readParametersFromAPVTS() const noexcept;

    juce::AudioProcessorValueTreeState apvts;
    PrismorDSP::SpectralSynthEngine synthEngine;
    juce::AudioProcessLoadMeasurer loadMeasurer;
    juce::MidiKeyboardState keyboardState;
    int currentPreset { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismorAudioProcessor)
};
