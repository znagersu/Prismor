#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Updater.h"
#include <functional>
#include <optional>
#include <vector>

class PluginBrowser : public juce::WebBrowserComponent
{
public:
    using juce::WebBrowserComponent::WebBrowserComponent;

    std::function<bool(const juce::String&)> onNavigate;

    bool pageAboutToLoad(const juce::String& newURL) override
    {
        if (onNavigate && onNavigate(newURL))
            return false;

        const auto root = getResourceProviderRoot();
        return newURL == root || newURL == root.trimCharactersAtEnd("/");
    }
};

class WebViewEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit WebViewEditor(PrismorAudioProcessor&);
    ~WebViewEditor() override;

    void resized() override;

private:
    void timerCallback() override;
    void sendInitDataToJS();
    void sendStateToJS(bool includeCurvesAndBars);
    void sendTelemetryToJS();
    void sendPresetListToJS();
    void setParamFromJS(const juce::String& paramId, float plainValue);
    void triggerUpdateCheckIfNeeded();
    void triggerSelfUpdate();
    void applyRestartOrRelaunch();

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    bool handleJSAction(const juce::String& url);

    PrismorAudioProcessor& m_processor;
    PluginBrowser m_webView;

    bool m_uiReady = false;
    bool m_forceFullSync = true;
    bool m_curvesDirty = true;
    int  m_tickCounter = 0;
    bool m_updateCheckStarted = false;

    std::vector<float> m_lastSentParams;
    int m_lastWaveA = -1;
    int m_lastWaveB = -1;
    int m_lastPresetIdx = -1;
    std::vector<int> m_lastFxChain;

    PrismorUpdate::UpdateState m_updateState { PrismorUpdate::UpdateState::Hidden };
    juce::String m_updateVersion;
    float m_updateProgress = 0.0f;
    juce::String m_updateStatusMsg;
    std::unique_ptr<PrismorUpdate::UpdateCheckerThread> m_updateChecker;
    std::unique_ptr<PrismorUpdate::SelfUpdateWorkerThread> m_selfUpdater;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewEditor)
};
