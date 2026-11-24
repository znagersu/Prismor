#include "WebViewEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
juce::StringPairArray parseQuery(const juce::String& url)
{
    juce::StringPairArray params;
    const auto query = url.fromFirstOccurrenceOf("?", false, false);

    for (const auto& part : juce::StringArray::fromTokens(query, "&", ""))
    {
        if (part.contains("="))
        {
            const auto key = part.upToFirstOccurrenceOf("=", false, false).trim();
            const auto val = part.fromFirstOccurrenceOf("=", false, false).trim();
            params.set(key, juce::URL::removeEscapeChars(val));
        }
    }
    return params;
}

juce::WebBrowserComponent::Options makeBrowserOptions(juce::WebBrowserComponent::ResourceProvider provider)
{
    auto options = juce::WebBrowserComponent::Options{}.withResourceProvider(std::move(provider));

   #if JUCE_WINDOWS
    const auto userDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                 .getChildFile("CyberSpectralAudio")
                                 .getChildFile("Prismor")
                                 .getChildFile("WebView2");
    userDataDir.createDirectory();

    options = options
                  .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                  .withWinWebView2Options(
                      juce::WebBrowserComponent::Options::WinWebView2{}
                          .withUserDataFolder(userDataDir)
                          .withBackgroundColour(juce::Colour(0xff0b0d14))
                          .withStatusBarDisabled()
                          .withBuiltInErrorPageDisabled());
   #endif

    return options;
}
} // namespace

WebViewEditor::WebViewEditor(PrismorAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      m_processor(p),
      m_webView(makeBrowserOptions([this](const juce::String& url) { return getResource(url); }))
{
    setSize(1240, 760);
    setResizable(true, false);
    setResizeLimits(960, 590, 1920, 1180);

    m_webView.onNavigate = [this](const juce::String& url) { return handleJSAction(url); };

    addAndMakeVisible(m_webView);
    m_webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    startTimerHz(30);
}

WebViewEditor::~WebViewEditor()
{
    stopTimer();
    m_selfUpdater.reset();
    m_updateChecker.reset();
}

void WebViewEditor::resized()
{
    m_webView.setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> WebViewEditor::getResource(const juce::String& url)
{
    const auto cleanUrl = url.upToFirstOccurrenceOf("?", false, false)
                             .upToFirstOccurrenceOf("#", false, false);
    const auto wanted = (cleanUrl.isEmpty() || cleanUrl == "/")
        ? juce::String("index.html")
        : cleanUrl.fromFirstOccurrenceOf("/", false, false);

    struct Entry { const char* name; const char* data; int size; const char* mime; };
    static const Entry entries[] = {
        { "index.html", Prismor_UI::index_html, Prismor_UI::index_htmlSize, "text/html" },
        { "style.css",  Prismor_UI::style_css,  Prismor_UI::style_cssSize,  "text/css" },
        { "app.js",     Prismor_UI::app_js,     Prismor_UI::app_jsSize,     "application/javascript" }
    };

    for (const auto& e : entries)
    {
        if (wanted != e.name)
            continue;
        const auto* bytes = reinterpret_cast<const std::byte*>(e.data);
        return juce::WebBrowserComponent::Resource{
            std::vector<std::byte>(bytes, bytes + static_cast<size_t>(juce::jmax(0, e.size))),
            juce::String(e.mime)
        };
    }
    return std::nullopt;
}

void WebViewEditor::timerCallback()
{
    ++m_tickCounter;
    if (! m_uiReady)
        return;

    if (! m_updateCheckStarted && m_tickCounter >= 60)
        triggerUpdateCheckIfNeeded();

    const auto& ids = PrismorAudioProcessor::getAllParameterIds();
    auto& apvts = m_processor.getAPVTS();
    if (m_lastSentParams.size() != static_cast<size_t>(ids.size()))
    {
        m_lastSentParams.assign(static_cast<size_t>(ids.size()), -99999.0f);
        m_forceFullSync = true;
    }

    bool changed = m_forceFullSync || m_curvesDirty;
    for (int i = 0; i < ids.size(); ++i)
    {
        if (auto* raw = apvts.getRawParameterValue(ids[i]))
        {
            const float v = raw->load(std::memory_order_relaxed);
            if (std::abs(v - m_lastSentParams[static_cast<size_t>(i)]) > 1e-4f)
            {
                m_lastSentParams[static_cast<size_t>(i)] = v;
                changed = true;
            }
        }
    }

    auto& engine = m_processor.getSynthEngine();
    const int waveA = engine.getResynthesisEngine().getActiveWaveType(0);
    const int waveB = engine.getResynthesisEngine().getActiveWaveType(1);
    const int presetIdx = m_processor.getCurrentPresetIndex();
    const auto fxChain = engine.getActiveFxChain();

    if (waveA != m_lastWaveA || waveB != m_lastWaveB || presetIdx != m_lastPresetIdx || fxChain != m_lastFxChain)
    {
        m_lastWaveA = waveA;
        m_lastWaveB = waveB;
        m_lastPresetIdx = presetIdx;
        m_lastFxChain = fxChain;
        changed = true;
    }

    if (changed)
    {
        const bool includeCurves = m_forceFullSync || m_curvesDirty;
        m_forceFullSync = false;
        m_curvesDirty = false;
        sendStateToJS(includeCurves);
    }

    sendTelemetryToJS();
}

void WebViewEditor::sendInitDataToJS()
{
    auto* root = new juce::DynamicObject();
    root->setProperty("version", juce::String(ProjectInfo::versionString));

    juce::Array<juce::var> waveNames;
    for (const auto& w : PrismorDSP::ResynthesisEngine::getWaveformNames())
        waveNames.add(w);
    root->setProperty("waveforms", waveNames);

    juce::Array<juce::var> syncDivs;
    for (const auto& d : PrismorDSP::SplineCurve::getSyncDivisionNames())
        syncDivs.add(d);
    root->setProperty("syncDivisions", syncDivs);

    auto makeSubArr = [](const juce::StringArray& arr) {
        juce::Array<juce::var> out;
        for (const auto& s : arr) out.add(s);
        return out;
    };
    root->setProperty("filterSubPresets", makeSubArr(m_processor.getFilterSubPresetNames()));
    root->setProperty("distSubPresets", makeSubArr(m_processor.getDistortionSubPresetNames()));

    const auto json = juce::JSON::toString(juce::var(root), true);
    m_webView.evaluateJavascript("window.initPrismor && window.initPrismor(" + json + ");");
    sendPresetListToJS();
}

void WebViewEditor::sendPresetListToJS()
{
    juce::Array<juce::var> arr;
    for (const auto& e : m_processor.getFactoryPresetBank())
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("index", e.index);
        o->setProperty("name", e.name);
        o->setProperty("category", e.category);
        o->setProperty("author", e.author);
        o->setProperty("waves", e.wavesUsed);
        o->setProperty("fx", e.fxUsed);
        o->setProperty("desc", e.description);
        o->setProperty("isUser", false);
        arr.add(juce::var(o));
    }
    for (const auto& u : m_processor.getUserPresetMetadata())
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("index", u.index);
        o->setProperty("name", u.name);
        o->setProperty("category", u.category);
        o->setProperty("author", u.author);
        o->setProperty("waves", u.wavesUsed);
        o->setProperty("fx", u.fxUsed);
        o->setProperty("desc", u.description);
        o->setProperty("isUser", true);
        o->setProperty("path", u.userFile.getFullPathName());
        arr.add(juce::var(o));
    }
    const auto json = juce::JSON::toString(juce::var(arr), true);
    m_webView.evaluateJavascript("window.updatePresets && window.updatePresets(" + json + ");");
}

void WebViewEditor::sendStateToJS(bool includeCurvesAndBars)
{
    auto* state = new juce::DynamicObject();
    auto& apvts = m_processor.getAPVTS();
    auto& engine = m_processor.getSynthEngine();

    auto* paramsObj = new juce::DynamicObject();
    for (const auto& id : PrismorAudioProcessor::getAllParameterIds())
    {
        if (auto* raw = apvts.getRawParameterValue(id))
            paramsObj->setProperty(id, static_cast<double>(raw->load(std::memory_order_relaxed)));
    }
    state->setProperty("params", juce::var(paramsObj));
    state->setProperty("waveA", engine.getResynthesisEngine().getActiveWaveType(0));
    state->setProperty("waveB", engine.getResynthesisEngine().getActiveWaveType(1));
    state->setProperty("currentPreset", m_processor.getCurrentPresetIndex());

    juce::Array<juce::var> chainArr;
    for (int id : engine.getActiveFxChain())
        chainArr.add(id);
    state->setProperty("fxChain", chainArr);

    auto* upd = new juce::DynamicObject();
    upd->setProperty("state", static_cast<int>(m_updateState));
    upd->setProperty("version", m_updateVersion);
    upd->setProperty("progress", static_cast<double>(m_updateProgress));
    upd->setProperty("msg", m_updateStatusMsg);
    state->setProperty("update", juce::var(upd));

    if (includeCurvesAndBars)
    {
        juce::Array<juce::var> arts;
        for (int i = 0; i < static_cast<int>(PrismorDSP::ArticulatorTarget::Count); ++i)
            arts.add(engine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(i)).toJsonVar());
        state->setProperty("articulators", arts);
        state->setProperty("timeModCurve", engine.getTimeModCurve().toJsonVar());

        juce::Array<juce::var> arpArr;
        for (int i = 0; i < PrismorDSP::SpectralSynthEngine::NUM_ARP_STEPS; ++i)
        {
            const auto st = engine.getArpStep(i);
            auto* so = new juce::DynamicObject();
            so->setProperty("enabled", st.enabled);
            so->setProperty("vel", static_cast<double>(st.velocity));
            so->setProperty("oct", st.octaveOffset);
            so->setProperty("semi", st.semitoneTranspose);
            arpArr.add(juce::var(so));
        }
        state->setProperty("arpSteps", arpArr);

        juce::Array<juce::var> barsA, barsB;
        for (int k = 0; k < 64; ++k)
        {
            barsA.add(static_cast<double>(engine.getResynthesisEngine().getHarmonicBarUI(0, k)));
            barsB.add(static_cast<double>(engine.getResynthesisEngine().getHarmonicBarUI(1, k)));
        }
        state->setProperty("barsA", barsA);
        state->setProperty("barsB", barsB);
    }

    const auto json = juce::JSON::toString(juce::var(state), true);
    m_webView.evaluateJavascript("window.updateState && window.updateState(" + json + ");");
}

void WebViewEditor::sendTelemetryToJS()
{
    const auto snap = m_processor.getSynthEngine().getLatestVisualizerSnapshot();
    auto* obj = new juce::DynamicObject();
    obj->setProperty("peakL", static_cast<double>(snap.peakLeft));
    obj->setProperty("peakR", static_cast<double>(snap.peakRight));
    obj->setProperty("rmsL", static_cast<double>(snap.rmsLeft));
    obj->setProperty("rmsR", static_cast<double>(snap.rmsRight));
    obj->setProperty("limGr", static_cast<double>(snap.limiterGainReductionDb));
    obj->setProperty("compGr", static_cast<double>(snap.compGainReductionDb));
    obj->setProperty("cpuPct", static_cast<double>(m_processor.getCpuUsagePercent()));
    obj->setProperty("voices", snap.activeVoiceCount);
    obj->setProperty("arpStep", snap.activeArpStep);
    obj->setProperty("voiceActive", snap.voiceActive);
    obj->setProperty("pitchHz", static_cast<double>(snap.activePitchHz));
    obj->setProperty("envCursor", static_cast<double>(snap.envCursor));
    obj->setProperty("tmCursor", static_cast<double>(snap.timeModCursor));
    obj->setProperty("tmOff", static_cast<double>(snap.timeModOffsetNorm));

    juce::Array<juce::var> sA, sB, flt, tmW;
    for (int i = 0; i < PrismorDSP::VisualizerSnapshot::NUM_BINS; i += 2)
    {
        const size_t k = static_cast<size_t>(i);
        sA.add(static_cast<double>(snap.ampA[k]));
        sB.add(static_cast<double>(snap.ampB[k]));
        flt.add(static_cast<double>(snap.filterCurve[k]));
    }
    for (size_t i = 0; i < snap.timeModWavePreview.size(); i += 2)
        tmW.add(static_cast<double>(snap.timeModWavePreview[i]));

    obj->setProperty("specA", sA);
    obj->setProperty("specB", sB);
    obj->setProperty("filt", flt);
    obj->setProperty("tmWave", tmW);

    const auto json = juce::JSON::toString(juce::var(obj), true);
    m_webView.evaluateJavascript("window.updateTelemetry && window.updateTelemetry(" + json + ");");
}

void WebViewEditor::setParamFromJS(const juce::String& paramId, float plainValue)
{
    if (! std::isfinite(plainValue))
        return;

    if (auto* param = m_processor.getAPVTS().getParameter(paramId))
    {
        const float norm = juce::jlimit(0.0f, 1.0f, param->convertTo0to1(plainValue));
        param->beginChangeGesture();
        param->setValueNotifyingHost(norm);
        param->endChangeGesture();
    }
}

void WebViewEditor::triggerUpdateCheckIfNeeded()
{
    if (m_updateCheckStarted)
        return;
    m_updateCheckStarted = true;

    juce::Component::SafePointer<WebViewEditor> safeThis(this);
    m_updateChecker = std::make_unique<PrismorUpdate::UpdateCheckerThread>(
        [safeThis](const juce::String& remoteTag)
        {
            if (safeThis == nullptr) return;
            safeThis->m_updateState = PrismorUpdate::UpdateState::UpdateAvailable;
            safeThis->m_updateVersion = remoteTag;
            safeThis->m_forceFullSync = true;
        });
    m_updateChecker->startThread(juce::Thread::Priority::low);
}

void WebViewEditor::triggerSelfUpdate()
{
    if (m_updateState == PrismorUpdate::UpdateState::Downloading ||
        m_updateState == PrismorUpdate::UpdateState::Extracting)
        return;

    const bool isStandalone = (m_processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone);
    m_updateState = PrismorUpdate::UpdateState::Downloading;
    m_updateProgress = 0.05f;
    m_updateStatusMsg = "STARTING...";
    m_forceFullSync = true;

    juce::Component::SafePointer<WebViewEditor> safeThis(this);
    m_selfUpdater = std::make_unique<PrismorUpdate::SelfUpdateWorkerThread>(
        isStandalone,
        [safeThis](float prog, const juce::String& msg)
        {
            if (safeThis == nullptr) return;
            safeThis->m_updateState = (prog >= 0.72f)
                ? PrismorUpdate::UpdateState::Extracting
                : PrismorUpdate::UpdateState::Downloading;
            safeThis->m_updateProgress = prog;
            safeThis->m_updateStatusMsg = msg;
            safeThis->m_forceFullSync = true;
        },
        [safeThis](bool success, bool standalone, const juce::String& err)
        {
            if (safeThis == nullptr) return;
            if (success)
            {
                safeThis->m_updateState = standalone
                    ? PrismorUpdate::UpdateState::InstalledRestartReady
                    : PrismorUpdate::UpdateState::InstalledReloadDAW;
                safeThis->m_updateProgress = 1.0f;
            }
            else
            {
                safeThis->m_updateState = PrismorUpdate::UpdateState::ErrorFallback;
                safeThis->m_updateStatusMsg = err;
            }
            safeThis->m_forceFullSync = true;
        });
    m_selfUpdater->startThread(juce::Thread::Priority::normal);
}

void WebViewEditor::applyRestartOrRelaunch()
{
   #if JUCE_WINDOWS
    juce::File batchFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("PrismorSelfUpdate")
                               .getChildFile("apply_update.bat");
    if (batchFile.existsAsFile())
    {
        juce::ChildProcess cp;
        cp.start("cmd.exe /c \"" + batchFile.getFullPathName() + "\"");
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
   #endif
}

bool WebViewEditor::handleJSAction(const juce::String& url)
{
    if (! url.startsWith("apc://callback"))
        return false;

    const auto params = parseQuery(url);
    const auto action = params["action"];
    auto& engine = m_processor.getSynthEngine();

    if (action == "ready")
    {
        m_uiReady = true;
        sendInitDataToJS();
        m_forceFullSync = true;
        sendStateToJS(true);
        return true;
    }

    if (action == "setParam")
    {
        const auto id = params["id"];
        const auto valStr = params["val"];
        if (id.isNotEmpty() && valStr.isNotEmpty())
            setParamFromJS(id, static_cast<float>(valStr.getDoubleValue()));
        return true;
    }

    if (action == "setWave")
    {
        const int part = params["part"].getIntValue();
        const int wave = params["wave"].getIntValue();
        engine.getResynthesisEngine().resetPartTableToWaveform(part, wave);
        m_curvesDirty = true;
        m_forceFullSync = true;
        return true;
    }

    if (action == "setHarmonicBar")
    {
        const int part = params["part"].getIntValue();
        const int idx  = params["idx"].getIntValue();
        const float val = static_cast<float>(params["val"].getDoubleValue());
        engine.getResynthesisEngine().setHarmonicBar(part, idx, val);
        m_curvesDirty = true;
        return true;
    }

    if (action == "harmOp")
    {
        const int part = params["part"].getIntValue();
        const auto op = params["op"];
        auto& re = engine.getResynthesisEngine();
        if      (op == "odd")       re.emphasizeOddEven(part, 1.25f, 0.45f);
        else if (op == "even")      re.emphasizeOddEven(part, 0.45f, 1.25f);
        else if (op == "lpf")       re.filterHarmonicTable(part, 0);
        else if (op == "hpf")       re.filterHarmonicTable(part, 1);
        else if (op == "smooth")    re.filterHarmonicTable(part, 2);
        else if (op == "randPhase") re.randomizeHarmonicPhases(part, false);
        else if (op == "zeroPhase") re.randomizeHarmonicPhases(part, true);
        else if (op == "shiftL")    re.shiftOrStretchHarmonicTable(part, -1, false);
        else if (op == "shiftR")    re.shiftOrStretchHarmonicTable(part, +1, false);
        else if (op == "stretch")   re.shiftOrStretchHarmonicTable(part, 0, true);
        else if (op == "copyAB")    re.copyPartTable(part, 1 - juce::jlimit(0, 1, part));
        m_curvesDirty = true;
        m_forceFullSync = true;
        return true;
    }

    if (action == "loadPreset")
    {
        const int idx = params["idx"].getIntValue();
        m_processor.loadFactoryPreset(idx);
        m_curvesDirty = true;
        m_forceFullSync = true;
        return true;
    }

    if (action == "loadUserPreset")
    {
        const juce::File f(params["path"]);
        if (f.existsAsFile())
        {
            if (auto xml = juce::XmlDocument::parse(f))
                m_processor.loadPresetFromXml(*xml);
            m_curvesDirty = true;
            m_forceFullSync = true;
        }
        return true;
    }

    if (action == "saveUserPreset")
    {
        const auto name = params["name"].trim();
        if (name.isNotEmpty())
        {
            m_processor.saveUserPreset(name, params["cat"].isEmpty() ? "USER" : params["cat"], "User", params["desc"]);
            sendPresetListToJS();
        }
        return true;
    }

    if (action == "deleteUserPreset")
    {
        const juce::File f(params["path"]);
        if (f.existsAsFile())
        {
            f.deleteFile();
            sendPresetListToJS();
        }
        return true;
    }

    if (action == "loadFilterSub")
    {
        m_processor.loadFilterSubPreset(params["idx"].getIntValue());
        m_forceFullSync = true;
        return true;
    }

    if (action == "loadDistSub")
    {
        m_processor.loadDistortionSubPreset(params["idx"].getIntValue());
        m_forceFullSync = true;
        return true;
    }

    if (action == "fxAdd")
    {
        engine.addFxModule(params["modId"].getIntValue());
        m_forceFullSync = true;
        return true;
    }

    if (action == "fxRemove")
    {
        engine.removeFxSlot(params["slot"].getIntValue());
        m_forceFullSync = true;
        return true;
    }

    if (action == "fxMove")
    {
        engine.moveFxSlot(params["from"].getIntValue(), params["to"].getIntValue());
        m_forceFullSync = true;
        return true;
    }

    if (action == "fxReset")
    {
        engine.resetFxChainOrder();
        m_forceFullSync = true;
        return true;
    }

    if (action == "artSetPreset")
    {
        const int target = juce::jlimit(0, 5, params["target"].getIntValue());
        const int preset = params["preset"].getIntValue();
        auto& curve = engine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(target));
        if (preset == -1) curve.resetToDefault(static_cast<PrismorDSP::ArticulatorTarget>(target));
        else if (preset == -2) curve.clearToFlat(static_cast<PrismorDSP::ArticulatorTarget>(target));
        else curve.applyPresetShape(preset);
        m_curvesDirty = true;
        return true;
    }

    if (action == "artSetNode")
    {
        const int target = juce::jlimit(0, 5, params["target"].getIntValue());
        const int idx = params["idx"].getIntValue();
        const float x = static_cast<float>(params["x"].getDoubleValue());
        const float y = static_cast<float>(params["y"].getDoubleValue());
        engine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(target)).updateNode(idx, x, y);
        m_curvesDirty = true;
        return true;
    }

    if (action == "artAddNode")
    {
        const int target = juce::jlimit(0, 5, params["target"].getIntValue());
        const float x = static_cast<float>(params["x"].getDoubleValue());
        const float y = static_cast<float>(params["y"].getDoubleValue());
        engine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(target)).addNode(x, y);
        m_curvesDirty = true;
        return true;
    }

    if (action == "artRemoveNode")
    {
        const int target = juce::jlimit(0, 5, params["target"].getIntValue());
        engine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(target)).removeNode(params["idx"].getIntValue());
        m_curvesDirty = true;
        return true;
    }

    if (action == "artSetProps")
    {
        const int target = juce::jlimit(0, 5, params["target"].getIntValue());
        auto& c = engine.getArticulator(static_cast<PrismorDSP::ArticulatorTarget>(target));
        if (params.containsKey("rate")) c.setRate(static_cast<float>(params["rate"].getDoubleValue()));
        if (params.containsKey("mode")) c.setMode(static_cast<PrismorDSP::ArticulatorMode>(params["mode"].getIntValue()));
        if (params.containsKey("sync")) c.setTempoSync(params["sync"].getIntValue() != 0);
        if (params.containsKey("div"))  c.setSyncDivision(params["div"].getIntValue());
        engine.notifyArticulatorRateEdited(static_cast<PrismorDSP::ArticulatorTarget>(target));
        m_curvesDirty = true;
        return true;
    }

    if (action == "timeModPreset")
    {
        engine.getTimeModCurve().applyTimeModPreset(params["idx"].getIntValue());
        m_curvesDirty = true;
        return true;
    }

    if (action == "timeModSetNode")
    {
        const int idx = params["idx"].getIntValue();
        const float x = static_cast<float>(params["x"].getDoubleValue());
        const float y = static_cast<float>(params["y"].getDoubleValue());
        engine.getTimeModCurve().updateNode(idx, x, y);
        m_curvesDirty = true;
        return true;
    }

    if (action == "timeModAddNode")
    {
        engine.getTimeModCurve().addNode(static_cast<float>(params["x"].getDoubleValue()),
                                         static_cast<float>(params["y"].getDoubleValue()));
        m_curvesDirty = true;
        return true;
    }

    if (action == "timeModRemoveNode")
    {
        engine.getTimeModCurve().removeNode(params["idx"].getIntValue());
        m_curvesDirty = true;
        return true;
    }

    if (action == "arpSetStep")
    {
        const int idx = params["idx"].getIntValue();
        auto st = engine.getArpStep(idx);
        if (params.containsKey("enabled")) st.enabled = params["enabled"].getIntValue() != 0;
        if (params.containsKey("vel"))     st.velocity = static_cast<float>(params["vel"].getDoubleValue());
        if (params.containsKey("oct"))     st.octaveOffset = params["oct"].getIntValue();
        if (params.containsKey("semi"))    st.semitoneTranspose = params["semi"].getIntValue();
        engine.setArpStep(idx, st);
        m_curvesDirty = true;
        return true;
    }

    if (action == "arpPreset")
    {
        engine.resetArpStepsToDefault(params["idx"].getIntValue());
        m_curvesDirty = true;
        return true;
    }

    if (action == "noteOn")
    {
        const int note = juce::jlimit(0, 127, params["note"].getIntValue());
        const float vel = static_cast<float>(juce::jlimit(0.05, 1.0, params.getValue("vel", "0.85").getDoubleValue()));
        m_processor.getKeyboardState().noteOn(1, note, vel);
        return true;
    }

    if (action == "noteOff")
    {
        const int note = juce::jlimit(0, 127, params["note"].getIntValue());
        m_processor.getKeyboardState().noteOff(1, note, 0.0f);
        return true;
    }

    if (action == "panic")
    {
        m_processor.getKeyboardState().allNotesOff(1);
        engine.resetAllVoices();
        return true;
    }

    if (action == "triggerUpdate")
    {
        if (m_updateState == PrismorUpdate::UpdateState::InstalledRestartReady)
            applyRestartOrRelaunch();
        else if (m_updateState == PrismorUpdate::UpdateState::ErrorFallback)
            juce::URL("https://github.com/znagersu/Prismor/releases/latest/download/Prismor_Latest.zip").launchInDefaultBrowser();
        else
            triggerSelfUpdate();
        return true;
    }

    if (action == "setZoom")
    {
        const double z = juce::jlimit(0.75, 1.5, params.getValue("val", "1.0").getDoubleValue());
        setSize(static_cast<int>(std::round(1240.0 * z)),
                static_cast<int>(std::round(760.0 * z)));
        return true;
    }

    return true;
}
