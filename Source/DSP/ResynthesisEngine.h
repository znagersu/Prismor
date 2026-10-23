#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <atomic>

namespace PrismorDSP
{

class ResynthesisEngine
{
public:
    static constexpr int TIME_FRAMES = 128;
    static constexpr int MAX_PARTIALS = 192;
    static constexpr int EDITABLE_PARTIALS = 64;

    struct PlaneData
    {
        std::array<std::array<float, MAX_PARTIALS>, TIME_FRAMES> amp {};
        std::array<std::array<float, MAX_PARTIALS>, TIME_FRAMES> freqOffset {};
    };

    struct EditablePartTable
    {
        std::array<float, MAX_PARTIALS> harmonicGain {};
        std::array<float, MAX_PARTIALS> harmonicPhase {};
    };

    ResynthesisEngine()
    {
        formatManager.registerBasicFormats();
        resetPartTableToWaveform(0, 0); // Part A: Clean Sawtooth
        resetPartTableToWaveform(1, 1); // Part B: Odd Square / Hollow
        generateProceduralPlane(0);
    }

    // Waveform generators for Part A (0) and Part B (1) Serum-2 / Zenology style harmonic editor
    // waveType: 0=Saw, 1=Square, 2=Triangle, 3=OrganDrawbar, 4=FormantCluster, 5=RandomSparkle,
    //           6=Sine, 7=Sync, 8=Bell, 9=X-Mod
    static juce::StringArray getWaveformNames()
    {
        return {
            "SAW WAVE",
            "SQR WAVE",
            "TRI WAVE",
            "ORGAN",
            "FORMANT",
            "SPARKLE",
            "SINE WAVE",
            "SYNC WAVE",
            "BELL WAVE",
            "X-MOD WAVE"
        };
    }

    int getActiveWaveType(int partIndex) const noexcept
    {
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        return activeWaveType[pIdx];
    }

    void resetPartTableToWaveform(int partIndex, int waveType)
    {
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        activeWaveType[pIdx] = juce::jlimit(0, 9, waveType);
        auto& tbl = partTables[pIdx];
        juce::Random rng(1337 + partIndex * 99 + waveType * 17);

        for (int k = 0; k < MAX_PARTIALS; ++k)
        {
            const float h = static_cast<float>(k + 1);
            float g = 0.0f;
            float ph = 0.0f;

            switch (waveType)
            {
                case 0: // Clean Calibrated Sawtooth (1/h)
                    g = 1.0f / h;
                    ph = 0.0f;
                    break;
                case 1: // Square (Odd harmonics 1/h)
                    g = (k % 2 == 0) ? (1.0f / h) : 0.0f;
                    ph = 0.0f;
                    break;
                case 2: // Triangle (Odd harmonics 1/h^2 with alternating phase)
                    g = (k % 2 == 0) ? (1.0f / (h * h)) : 0.0f;
                    ph = ((k / 2) % 2 == 1) ? 0.5f : 0.0f;
                    break;
                case 3: // Cyber Organ Drawbars ( harmonics 1, 2, 3, 4, 6, 8, 12, 16 )
                    if (k == 0 || k == 1 || k == 2 || k == 3 || k == 5 || k == 7 || k == 11 || k == 15)
                        g = 1.0f / std::pow(h, 0.55f);
                    else
                        g = 0.02f / h;
                    break;
                case 4: // Formant Harmonic Cluster
                {
                    const float f1 = std::exp(-0.5f * std::pow((h - 1.0f) / 1.5f, 2.0f));
                    const float f2 = 0.85f * std::exp(-0.5f * std::pow((h - 7.0f) / 2.2f, 2.0f));
                    const float f3 = 0.65f * std::exp(-0.5f * std::pow((h - 19.0f) / 4.0f, 2.0f));
                    g = juce::jlimit(0.0f, 1.0f, (f1 + f2 + f3) / std::pow(h, 0.35f));
                    break;
                }
                case 5: // Metallic / Random Harmonic Distribution
                    g = (k == 0) ? 1.0f : (rng.nextFloat() / std::pow(h, 0.72f));
                    ph = rng.nextFloat();
                    break;
                case 6: // Zenology Pure Sine Wave (Fundamental + warm micro-overtones)
                    if (k == 0)      g = 1.0f;
                    else if (k == 1) g = 0.032f;
                    else if (k == 2) g = 0.014f;
                    else             g = 0.0f;
                    ph = 0.0f;
                    break;
                case 7: // Zenology Hard-Sync Wave (Resonant Sync Formant Cluster at 3.5x & 7.0x f0)
                {
                    const float syncRatio = 3.5f;
                    const float lobe1 = std::exp(-0.5f * std::pow((h - syncRatio) / 1.35f, 2.0f));
                    const float lobe2 = 0.82f * std::exp(-0.5f * std::pow((h - syncRatio * 2.0f) / 1.8f, 2.0f));
                    const float lobe3 = 0.58f * std::exp(-0.5f * std::pow((h - syncRatio * 3.0f) / 2.4f, 2.0f));
                    const float syncRipple = 0.55f + 0.45f * std::cos((h / syncRatio) * juce::MathConstants<float>::twoPi);
                    g = ((k == 0 ? 0.88f : 0.28f / std::pow(h, 0.65f)) + (lobe1 + lobe2 + lobe3) * 0.92f) * syncRipple;
                    ph = std::fmod(h * 0.1428f, 1.0f);
                    break;
                }
                case 8: // Zenology FM / Bessel Bell Wave (Modal Metallic Bell Partials)
                {
                    // Classic FM bell spectrum: strong clangorous partials at modal ratios
                    const bool isModal = (k == 0 || k == 1 || k == 2 || k == 4 || k == 6 ||
                                          k == 8 || k == 11 || k == 15 || k == 20 || k == 26 ||
                                          k == 33 || k == 41 || k == 50);
                    if (isModal)
                        g = 1.0f / std::pow(h, 0.44f);
                    else
                        g = (0.08f * std::abs(std::sin(h * 1.61803f))) / std::pow(h, 0.80f);
                    ph = (k % 3 == 0) ? 0.0f : ((k % 3 == 1) ? 0.25f : 0.75f);
                    break;
                }
                case 9: // Zenology X-Mod Wave (Cross-Modulation Carrier/Modulator Sideband Series)
                {
                    // Sidebands around carrier harmonics 1, 4, 9, 14, 21 with ring-like symmetry
                    const float sb1 = std::abs(std::cos(h * 0.785398f));
                    const float sb2 = std::abs(std::sin(h * 1.30899f));
                    const float ringEnv = (k % 2 == 0) ? sb1 : (sb2 * 0.92f);
                    g = (k == 0) ? 0.95f : (ringEnv / std::pow(h, 0.52f));
                    ph = (k % 2 == 0) ? 0.125f : 0.625f;
                    break;
                }
                default:
                    g = 1.0f / h;
                    break;
            }

            tbl.harmonicGain[static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, g);
            tbl.harmonicPhase[static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, ph);
        }
    }

    void setHarmonicBar(int partIndex, int partialIndex, float normalizedBarValue)
    {
        if (partialIndex < 0 || partialIndex >= MAX_PARTIALS)
            return;
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        // Convert perceptual UI bar height [0..1] into partial amplitude
        const float val = juce::jlimit(0.0f, 1.0f, normalizedBarValue);
        partTables[pIdx].harmonicGain[static_cast<size_t>(partialIndex)] = val * val;
    }

    void setHarmonicPhaseBar(int partIndex, int partialIndex, float normalizedPhase)
    {
        if (partialIndex < 0 || partialIndex >= MAX_PARTIALS)
            return;
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        partTables[pIdx].harmonicPhase[static_cast<size_t>(partialIndex)] = juce::jlimit(0.0f, 1.0f, normalizedPhase);
    }

    float getHarmonicBarUI(int partIndex, int partialIndex) const noexcept
    {
        if (partialIndex < 0 || partialIndex >= MAX_PARTIALS)
            return 0.0f;
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        const float raw = partTables[pIdx].harmonicGain[static_cast<size_t>(partialIndex)];
        return std::sqrt(juce::jlimit(0.0f, 1.0f, raw));
    }

    float getHarmonicGainRaw(int partIndex, int partialIndex) const noexcept
    {
        if (partialIndex < 0 || partialIndex >= MAX_PARTIALS)
            return 0.0f;
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        return partTables[pIdx].harmonicGain[static_cast<size_t>(partialIndex)];
    }

    float getHarmonicPhaseRaw(int partIndex, int partialIndex) const noexcept
    {
        if (partialIndex < 0 || partialIndex >= MAX_PARTIALS)
            return 0.0f;
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        return partTables[pIdx].harmonicPhase[static_cast<size_t>(partialIndex)];
    }

    void copyPartTable(int fromPart, int toPart)
    {
        const size_t src = static_cast<size_t>(juce::jlimit(0, 1, fromPart));
        const size_t dst = static_cast<size_t>(juce::jlimit(0, 1, toPart));
        if (src == dst)
            return;
        juce::ScopedLock sl(metaLock);
        partTables[dst] = partTables[src];
    }

    // Odd/Even harmonic emphasis: oddMult scales odd harmonics (k=0,2,4...), evenMult scales even harmonics (k=1,3,5...)
    void emphasizeOddEven(int partIndex, float oddMult, float evenMult)
    {
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        auto& tbl = partTables[pIdx];
        for (int k = 0; k < MAX_PARTIALS; ++k)
        {
            const bool isOddHarmonic = (k % 2 == 0); // k=0 is 1st harmonic (odd)
            const float mult = isOddHarmonic ? oddMult : evenMult;
            tbl.harmonicGain[static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, tbl.harmonicGain[static_cast<size_t>(k)] * mult);
        }
    }

    // Per-partial phase randomization or alignment
    void randomizeHarmonicPhases(int partIndex, bool clearToZero = false)
    {
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        auto& tbl = partTables[pIdx];
        juce::Random rng;
        for (int k = 0; k < MAX_PARTIALS; ++k)
        {
            tbl.harmonicPhase[static_cast<size_t>(k)] = clearToZero ? 0.0f : rng.nextFloat();
        }
    }

    // Spectral filtering directly on the harmonic distribution table (0=LPF, 1=HPF, 2=Smooth)
    void filterHarmonicTable(int partIndex, int filterMode)
    {
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        auto& tbl = partTables[pIdx];

        if (filterMode == 0) // Spectral Low-Pass Rolloff
        {
            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const float norm = static_cast<float>(k) / static_cast<float>(EDITABLE_PARTIALS);
                const float atten = 1.0f / (1.0f + norm * norm * 3.5f);
                tbl.harmonicGain[static_cast<size_t>(k)] *= atten;
            }
        }
        else if (filterMode == 1) // Spectral High-Pass Tilt
        {
            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const float norm = std::min(1.5f, static_cast<float>(k + 1) / 16.0f);
                tbl.harmonicGain[static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f,
                    tbl.harmonicGain[static_cast<size_t>(k)] * (0.35f + 0.95f * norm));
            }
        }
        else // 3-Tap Spectral Smoothing
        {
            auto copy = tbl.harmonicGain;
            for (int k = 1; k + 1 < MAX_PARTIALS; ++k)
            {
                tbl.harmonicGain[static_cast<size_t>(k)] =
                    0.25f * copy[static_cast<size_t>(k - 1)] +
                    0.50f * copy[static_cast<size_t>(k)] +
                    0.25f * copy[static_cast<size_t>(k + 1)];
            }
        }
    }

    // Harmonic Shift (deltaBins = -1 or +1) & Harmonic Stretch (expands partial spacing)
    void shiftOrStretchHarmonicTable(int partIndex, int deltaBins, bool stretchSpread = false)
    {
        const size_t pIdx = static_cast<size_t>(juce::jlimit(0, 1, partIndex));
        juce::ScopedLock sl(metaLock);
        auto& tbl = partTables[pIdx];
        const auto oldGain  = tbl.harmonicGain;
        const auto oldPhase = tbl.harmonicPhase;

        tbl.harmonicGain.fill(0.0f);
        tbl.harmonicPhase.fill(0.0f);

        if (stretchSpread)
        {
            // Stretch harmonics outward (spreads partials to every 2nd harmonic bin with interpolation)
            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const int dst = k * 2;
                if (dst < MAX_PARTIALS)
                {
                    tbl.harmonicGain[static_cast<size_t>(dst)] = oldGain[static_cast<size_t>(k)];
                    tbl.harmonicPhase[static_cast<size_t>(dst)] = oldPhase[static_cast<size_t>(k)];
                }
            }
        }
        else
        {
            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const int src = k - deltaBins;
                if (src >= 0 && src < MAX_PARTIALS)
                {
                    tbl.harmonicGain[static_cast<size_t>(k)]  = oldGain[static_cast<size_t>(src)];
                    tbl.harmonicPhase[static_cast<size_t>(k)] = oldPhase[static_cast<size_t>(src)];
                }
            }
        }
    }

    void generateProceduralPlane(int presetType)
    {
        const int writeIdx = 1 - activePlaneIndex.load(std::memory_order_relaxed);
        auto& plane = planes[static_cast<size_t>(writeIdx)];

        juce::Image preview(juce::Image::ARGB, TIME_FRAMES, MAX_PARTIALS, true);

        for (int t = 0; t < TIME_FRAMES; ++t)
        {
            const float normT = static_cast<float>(t) / static_cast<float>(TIME_FRAMES - 1);

            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const float h = static_cast<float>(k + 1);
                const float normK = static_cast<float>(k) / static_cast<float>(MAX_PARTIALS - 1);
                float amp = 0.0f;
                float fOff = 0.0f;

                if (presetType == 0) // Cyber Formant Matrix
                {
                    const float f1 = 3.0f + 5.0f * std::sin(normT * juce::MathConstants<float>::twoPi);
                    const float f2 = 14.0f + 10.0f * std::cos(normT * juce::MathConstants<float>::twoPi * 1.5f);
                    const float f3 = 36.0f + 16.0f * std::sin(normT * juce::MathConstants<float>::twoPi * 0.5f);

                    const float p1 = std::exp(-0.5f * std::pow((h - f1) / 2.2f, 2.0f));
                    const float p2 = 0.75f * std::exp(-0.5f * std::pow((h - f2) / 4.5f, 2.0f));
                    const float p3 = 0.45f * std::exp(-0.5f * std::pow((h - f3) / 7.5f, 2.0f));
                    const float baseSaw = 1.0f / std::pow(h, 0.78f);

                    amp = juce::jlimit(0.0f, 1.0f, baseSaw * (0.25f + 0.95f * (p1 + p2 + p3)));
                    fOff = 0.04f * std::sin(normT * 6.28318f + normK * 9.0f);
                }
                else if (presetType == 1) // Hyper-Saw Interference
                {
                    const float comb = 0.5f + 0.5f * std::cos(std::log2(h) * 5.5f - normT * 12.566f);
                    amp = (1.0f / std::pow(h, 0.72f)) * (0.15f + 0.85f * comb * comb);
                    fOff = 0.08f * std::sin(h * 0.35f + normT * 6.28318f);
                }
                else if (presetType == 2) // Obsidian Neuro Morph
                {
                    const float notch1 = std::abs(std::sin(std::log2(h) * 3.2f + normT * 6.28318f));
                    const float vocal = std::exp(-0.5f * std::pow((std::log2(h) - (2.2f + 1.8f * std::sin(normT * 6.28f))) / 0.85f, 2.0f));
                    const float sub = (k == 0) ? 1.0f : ((k % 2 == 0) ? 0.85f : 0.55f);
                    amp = juce::jlimit(0.0f, 1.0f, sub * (1.0f / std::pow(h, 0.68f)) * notch1 * (0.35f + 0.85f * vocal));
                    fOff = 0.03f * std::cos(normT * 12.56f - normK * 4.0f);
                }
                else // Crystal Harmonic Cascade
                {
                    const float wave = std::max(0.0f, std::sin(h * 0.45f - normT * 9.42f));
                    amp = (1.0f / std::pow(h, 0.62f)) * (0.1f + 0.9f * std::pow(wave, 3.0f));
                    fOff = 0.14f * normK * std::sin(normT * 6.28318f + h * 0.2f);
                }

                plane.amp[static_cast<size_t>(t)][static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, amp);
                plane.freqOffset[static_cast<size_t>(t)][static_cast<size_t>(k)] = juce::jlimit(-0.5f, 0.5f, fOff);

                const int y = (MAX_PARTIALS - 1) - k;
                const float c = std::pow(juce::jlimit(0.0f, 1.0f, amp), 0.55f);
                const float m = juce::jlimit(0.0f, 1.0f, 0.5f + fOff * 3.0f);
                juce::Colour col = juce::Colour::fromFloatRGBA(
                    c * 0.15f + m * 0.65f * c,
                    c * 0.92f,
                    c * 1.0f,
                    1.0f);
                preview.setPixelAt(t, y, col);
            }
        }

        activePlaneIndex.store(writeIdx, std::memory_order_release);

        {
            juce::ScopedLock sl(metaLock);
            previewImage = preview;
            static const char* names[] = {
                "SYNTH MATRIX: CYBER FORMANT",
                "SYNTH MATRIX: HYPER-SAW",
                "SYNTH MATRIX: NEURO MORPH",
                "SYNTH MATRIX: CRYSTAL CASCADE"
            };
            sourceName = names[juce::jlimit(0, 3, presetType)];
            sourceMode = "PROCEDURAL 2D TABLE";
        }
    }

    bool loadImageFile(const juce::File& file)
    {
        juce::Image img = juce::ImageFileFormat::loadFrom(file);
        if (!img.isValid())
            return false;

        loadFromImage(img, file.getFileName());
        return true;
    }

    void loadFromImage(const juce::Image& img, const juce::String& name)
    {
        const int writeIdx = 1 - activePlaneIndex.load(std::memory_order_relaxed);
        auto& plane = planes[static_cast<size_t>(writeIdx)];

        juce::Image scaled = img.rescaled(TIME_FRAMES, MAX_PARTIALS, juce::Graphics::mediumResamplingQuality);
        juce::Image preview(juce::Image::ARGB, TIME_FRAMES, MAX_PARTIALS, true);

        for (int t = 0; t < TIME_FRAMES; ++t)
        {
            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const int y = (MAX_PARTIALS - 1) - k;
                const juce::Colour px = scaled.getPixelAt(t, y);

                const float r = px.getFloatRed();
                const float g = px.getFloatGreen();
                const float b = px.getFloatBlue();
                const float a = px.getFloatAlpha();

                const float luma = a * (0.45f * r + 0.45f * g + 0.10f * b);
                const float naturalWeight = 1.0f / std::pow(static_cast<float>(k + 1), 0.45f);
                const float amp = std::pow(luma, 1.8f) * naturalWeight;
                const float fOff = (b - r) * 0.25f;

                plane.amp[static_cast<size_t>(t)][static_cast<size_t>(k)] = juce::jlimit(0.0f, 1.0f, amp);
                plane.freqOffset[static_cast<size_t>(t)][static_cast<size_t>(k)] = juce::jlimit(-0.5f, 0.5f, fOff);

                preview.setPixelAt(t, y, px);
            }
        }

        activePlaneIndex.store(writeIdx, std::memory_order_release);

        {
            juce::ScopedLock sl(metaLock);
            previewImage = preview;
            sourceName = "IMG: " + name.toUpperCase();
            sourceMode = "IMAGE RESYNTHESIS (2D FREQ/AMP)";
        }
    }

    bool loadAudioFile(const juce::File& file)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr || reader->lengthInSamples <= 0)
            return false;

        const int maxSamples = static_cast<int>(std::min<juce::int64>(reader->lengthInSamples, static_cast<juce::int64>(reader->sampleRate * 6.0)));
        juce::AudioBuffer<float> buffer(1, maxSamples);
        reader->read(&buffer, 0, maxSamples, 0, true, false);

        const float* samples = buffer.getReadPointer(0);
        const double sr = reader->sampleRate;

        float baseFreq = 130.81f;
        {
            const int minLag = std::max(16, static_cast<int>(sr / 600.0));
            const int maxLag = std::min(maxSamples / 2, static_cast<int>(sr / 55.0));
            const int winLen = std::min(2048, maxSamples / 2);
            const int startOffset = maxSamples / 4;
            float bestCorr = -1.0f;
            int bestLag = static_cast<int>(sr / 130.81);

            for (int lag = minLag; lag < maxLag; ++lag)
            {
                float sum = 0.0f;
                for (int n = 0; n < winLen && (startOffset + n + lag) < maxSamples; ++n)
                    sum += samples[startOffset + n] * samples[startOffset + n + lag];
                if (sum > bestCorr)
                {
                    bestCorr = sum;
                    bestLag = lag;
                }
            }
            if (bestLag > 0)
                baseFreq = juce::jlimit(55.0f, 600.0f, static_cast<float>(sr / static_cast<double>(bestLag)));
        }

        const int writeIdx = 1 - activePlaneIndex.load(std::memory_order_relaxed);
        auto& plane = planes[static_cast<size_t>(writeIdx)];
        juce::Image preview(juce::Image::ARGB, TIME_FRAMES, MAX_PARTIALS, true);

        const int windowSize = 512;
        float maxObservedAmp = 0.001f;

        for (int t = 0; t < TIME_FRAMES; ++t)
        {
            const float normT = static_cast<float>(t) / static_cast<float>(TIME_FRAMES - 1);
            const int centerSample = static_cast<int>(normT * static_cast<float>(std::max(1, maxSamples - windowSize)));

            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                const float targetFreq = baseFreq * static_cast<float>(k + 1);
                if (targetFreq >= static_cast<float>(sr * 0.47))
                {
                    plane.amp[static_cast<size_t>(t)][static_cast<size_t>(k)] = 0.0f;
                    plane.freqOffset[static_cast<size_t>(t)][static_cast<size_t>(k)] = 0.0f;
                    continue;
                }

                const float omega = juce::MathConstants<float>::twoPi * targetFreq / static_cast<float>(sr);
                float re = 0.0f, im = 0.0f;
                for (int n = 0; n < windowSize && (centerSample + n) < maxSamples; ++n)
                {
                    const float hann = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * static_cast<float>(n) / static_cast<float>(windowSize - 1)));
                    const float s = samples[centerSample + n] * hann;
                    const float ang = omega * static_cast<float>(n);
                    re += s * std::cos(ang);
                    im -= s * std::sin(ang);
                }

                const float mag = std::sqrt(re * re + im * im) / static_cast<float>(windowSize * 0.25f);
                plane.amp[static_cast<size_t>(t)][static_cast<size_t>(k)] = mag;
                plane.freqOffset[static_cast<size_t>(t)][static_cast<size_t>(k)] = 0.0f;
                if (mag > maxObservedAmp)
                    maxObservedAmp = mag;
            }
        }

        for (int t = 0; t < TIME_FRAMES; ++t)
        {
            for (int k = 0; k < MAX_PARTIALS; ++k)
            {
                float normAmp = plane.amp[static_cast<size_t>(t)][static_cast<size_t>(k)] / maxObservedAmp;
                normAmp = juce::jlimit(0.0f, 1.0f, std::pow(normAmp, 0.75f));
                plane.amp[static_cast<size_t>(t)][static_cast<size_t>(k)] = normAmp;

                const int y = (MAX_PARTIALS - 1) - k;
                const float v = std::pow(normAmp, 0.55f);
                preview.setPixelAt(t, y, juce::Colour::fromFloatRGBA(v * 0.95f, v * 0.35f, v * 0.85f, 1.0f));
            }
        }

        activePlaneIndex.store(writeIdx, std::memory_order_release);

        {
            juce::ScopedLock sl(metaLock);
            previewImage = preview;
            sourceName = "WAV: " + file.getFileName().toUpperCase();
            sourceMode = "AUDIO HARMONIC RESYNTHESIS";
        }
        return true;
    }

    void samplePartial(float normalizedTime, int partialIndex, float& outAmp, float& outFreqOffset) const noexcept
    {
        if (partialIndex < 0 || partialIndex >= MAX_PARTIALS)
        {
            outAmp = 0.0f;
            outFreqOffset = 0.0f;
            return;
        }

        const int readIdx = activePlaneIndex.load(std::memory_order_acquire);
        const auto& plane = planes[static_cast<size_t>(readIdx)];

        const float wrappedT = normalizedTime - std::floor(normalizedTime);
        const float scaledT = juce::jlimit(0.0f, 1.0f, wrappedT) * static_cast<float>(TIME_FRAMES - 1);
        const int t0 = static_cast<int>(scaledT);
        const int t1 = std::min(t0 + 1, TIME_FRAMES - 1);
        const float frac = scaledT - static_cast<float>(t0);

        const size_t k = static_cast<size_t>(partialIndex);
        outAmp = plane.amp[static_cast<size_t>(t0)][k] + frac * (plane.amp[static_cast<size_t>(t1)][k] - plane.amp[static_cast<size_t>(t0)][k]);
        outFreqOffset = plane.freqOffset[static_cast<size_t>(t0)][k] + frac * (plane.freqOffset[static_cast<size_t>(t1)][k] - plane.freqOffset[static_cast<size_t>(t0)][k]);
    }

    juce::Image getPreviewImage() const
    {
        juce::ScopedLock sl(metaLock);
        return previewImage;
    }

    juce::String getSourceName() const
    {
        juce::ScopedLock sl(metaLock);
        return sourceName;
    }

    juce::String getSourceMode() const
    {
        juce::ScopedLock sl(metaLock);
        return sourceMode;
    }

private:
    juce::AudioFormatManager formatManager;
    std::array<PlaneData, 2> planes;
    std::array<EditablePartTable, 2> partTables;
    std::array<int, 2> activeWaveType { 0, 1 };
    std::atomic<int> activePlaneIndex { 0 };

    mutable juce::CriticalSection metaLock;
    juce::Image previewImage;
    juce::String sourceName { "SYNTH MATRIX: CYBER FORMANT" };
    juce::String sourceMode { "PROCEDURAL 2D TABLE" };
};

} // namespace PrismorDSP
