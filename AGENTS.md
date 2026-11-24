# FL Studio & Windows JUCE 9 Engineering Reference

## 1. WebView2 Inside FL Studio (`Program Files` Permission Trap)
FL Studio (`FL64.exe`) runs from `C:\Program Files\Image-Line\FL Studio 2026\`. Without an explicit `UserDataFolder`, WebView2 attempts to write cache files beside `FL64.exe`, receives `E_ACCESSDENIED`, and JUCE silently falls back to legacy IE (`Win32WebView`), breaking `https://juce.backend/`.

Always configure `WebBrowserComponent::Options` on Windows like this:
```cpp
#if JUCE_WINDOWS
const auto userDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                             .getChildFile ("EndioAudio")
                             .getChildFile ("<PluginName>")
                             .getChildFile ("WebView2");
userDataDir.createDirectory();

options = options
              .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
              .withWinWebView2Options (
                  juce::WebBrowserComponent::Options::WinWebView2{}
                      .withUserDataFolder (userDataDir)
                      .withBackgroundColour (juce::Colour (0xff090b12))
                      .withStatusBarDisabled()
                      .withBuiltInErrorPageDisabled());
#endif
```
In `PluginBrowser::pageAboutToLoad`, allow both `getResourceProviderRoot()` and `getResourceProviderRoot().trimCharactersAtEnd ("/")`.

## 2. FL Studio Variable Buffer Sizes
FL Studio dynamically varies `buffer.getNumSamples()` on every `processBlock()` call (including `1–16` samples around automation events and non-power-of-two sizes):
- Pre-allocate all scratch buffers in `prepareToPlay()` for `kMaxChunkSamples` (e.g. `512`).
- Loop in `<= kMaxChunkSamples` chunks inside `process()` so neither tiny blocks nor oversized blocks allocate or overflow.
- For fixed-frame algorithms (FFT/STFT/ML), decouple via a pre-allocated circular FIFO (`juce::AbstractFifo`).

## 3. LFO / Phase Accumulator Continuity
Never derive multiple modulated LFOs by multiplying a single wrapped `[0, 1)` phase by non-integer ratios (e.g. `m_lfoPhase * 1.37`). When `m_lfoPhase` wraps `1.0 -> 0.0`, `1.37 -> 0.0` causes an audible click visible as a vertical broadband line in the STFT spectrogram. Always advance independent phase accumulators (`m_lfoPhase1`, `m_lfoPhase2`, ...).

## 4. FL Studio Wrapper, Sidechain & MIDI Quirks
- **Automation:** Register all user parameters in `APVTS` with stable string IDs (never rename IDs across versions—`.flp` saves bind to them).
- **Sidechain:** Declare optional auxiliary buses explicitly in `isBusesLayoutSupported()` (FL Studio users map sidechain pins manually in *Wrapper → Processing*).
- **MIDI Effects:** Set `NEEDS_MIDI_INPUT TRUE` in `juce_add_plugin(...)` even for effects if they respond to MIDI notes or CCs.

## 5. `apc-mcp` & Windows Build Notes
- Always configure CMake with `generator: "Visual Studio 17 2022"` and `options: "-DJUCE_DIR=C:/Users/endio/SDKs/JUCE"`.
- Build multiple targets sequentially (`--target A`, then `--target B`) via `cmake --build` to avoid parallel `.exe` write contention with Windows Defender (`LNK1104`) and 3-minute MCP call timeouts.
- Run `pluginval` on Windows via `Start-Process -Wait -PassThru -NoNewWindow` with `--output-dir` so PowerShell waits for completion and captures the full validation log.
