#pragma once
#include <JuceHeader.h>
#include "../DSP/SpectralSynthEngine.h"
#include "PrismorLookAndFeel.h"
#include <array>
#include <cmath>

namespace PrismorUI
{

class Visualizer3D : public juce::Component,
                     public juce::FileDragAndDropTarget
{
public:
    explicit Visualizer3D(PrismorDSP::SpectralSynthEngine& engineRef)
        : synthEngine(engineRef)
    {
        setOpaque(false);
    }

    std::function<void(int)> onActivePartChanged;
    std::function<void(int, int)> onWaveformSelected;

    void setActiveEditPart(int partIdx)
    {
        activePart = juce::jlimit(0, 1, partIdx);
        repaint();
    }

    int getActiveEditPart() const noexcept { return activePart; }

    void set2DViewMode(bool use2D) noexcept
    {
        is2DView = use2D;
        repaint();
    }

    bool is2DViewMode() const noexcept { return is2DView; }

    void pushSnapshot(const PrismorDSP::VisualizerSnapshot& snap)
    {
        writeHead = (writeHead + 1) % HISTORY_DEPTH;
        history[static_cast<size_t>(writeHead)] = snap;
        latestSnapshot = snap;
        animPhase += 0.045f;
        repaint();
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override
    {
        for (const auto& f : files)
        {
            if (f.endsWithIgnoreCase(".png") || f.endsWithIgnoreCase(".jpg") ||
                f.endsWithIgnoreCase(".jpeg") || f.endsWithIgnoreCase(".bmp") ||
                f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".aif") ||
                f.endsWithIgnoreCase(".aiff") || f.endsWithIgnoreCase(".flac"))
                return true;
        }
        return false;
    }

    void fileDragEnter(const juce::StringArray&, int, int) override
    {
        isDragOver = true;
        repaint();
    }

    void fileDragExit(const juce::StringArray&) override
    {
        isDragOver = false;
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int, int) override
    {
        isDragOver = false;
        if (files.isEmpty())
            return;

        juce::File file(files[0]);
        auto& resynth = synthEngine.getResynthesisEngine();
        if (file.hasFileExtension("png;jpg;jpeg;bmp"))
            resynth.loadImageFile(file);
        else if (file.hasFileExtension("wav;aif;aiff;flac"))
            resynth.loadAudioFile(file);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Recessed Pitch-Black 2.5D Display Stage
        g.setColour(Palette::borderDark);
        g.drawRoundedRectangle(bounds.expanded(0.5f), 5.0f, 1.0f);

        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

        juce::ColourGradient bgGlow(
            Palette::electricCyan.withAlpha(0.06f),
            bounds.getCentreX(), bounds.getY() + 90.0f,
            Palette::trackDark,
            bounds.getX(), bounds.getY(),
            true);
        g.setGradientFill(bgGlow);
        g.fillRoundedRectangle(bounds, 5.0f);

        // Top HUD Header inside Viewport + 2D / 3D View-Mode Toggle Switch
        const auto headerRect = bounds.withHeight(22.0f).reduced(8.0f, 2.0f);
        g.setColour(Palette::electricCyan);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.2f, juce::Font::bold));
        g.drawText(is2DView ? "2D SPECTRAL & WAVEFORM ANALYZER" : "DUAL 3D FREQUENCY FIELDS [F1 + F2]",
                   headerRect.withTrimmedRight(116.0f), juce::Justification::centredLeft, false);

        // 2D / 3D View Toggle Pills in top-right of header
        const juce::Rectangle<float> btn3D(bounds.getRight() - 108.0f, bounds.getY() + 3.0f, 48.0f, 16.0f);
        const juce::Rectangle<float> btn2D(bounds.getRight() - 56.0f,  bounds.getY() + 3.0f, 48.0f, 16.0f);

        g.setColour(!is2DView ? Palette::electricCyan.withAlpha(0.28f) : Palette::cardSurface);
        g.fillRoundedRectangle(btn3D, 2.5f);
        g.setColour(!is2DView ? Palette::electricCyan : Palette::borderSubtle);
        g.drawRoundedRectangle(btn3D, 2.5f, 1.0f);
        g.setColour(!is2DView ? Palette::textPrimary : Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
        g.drawText("3D VIEW", btn3D, juce::Justification::centred, false);

        g.setColour(is2DView ? Palette::energeticAmber.withAlpha(0.28f) : Palette::cardSurface);
        g.fillRoundedRectangle(btn2D, 2.5f);
        g.setColour(is2DView ? Palette::energeticAmber : Palette::borderSubtle);
        g.drawRoundedRectangle(btn2D, 2.5f, 1.0f);
        g.setColour(is2DView ? Palette::textPrimary : Palette::textSecondary);
        g.drawText("2D VIEW", btn2D, juce::Justification::centred, false);

        // 1. Dual 2D or 3D Stage Area
        const auto stage = get3DStageBounds();
        if (is2DView)
        {
            draw2DSpectralStage(g, stage);
        }
        else
        {
            draw3DFloorGrid(g, stage);

            constexpr int numBins = PrismorDSP::VisualizerSnapshot::NUM_BINS;

            for (int slice = HISTORY_DEPTH - 1; slice >= 0; --slice)
            {
                const int ringIdx = (writeHead - slice + HISTORY_DEPTH) % HISTORY_DEPTH;
                const auto& frame = history[static_cast<size_t>(ringIdx)];

                const float zNorm = static_cast<float>(slice) / static_cast<float>(HISTORY_DEPTH - 1);
                const float depthAlpha = std::pow(1.0f - zNorm * 0.85f, 1.5f);

                juce::Path wavePathA;
                juce::Path wavePathB;

                for (int k = 0; k < numBins; ++k)
                {
                    const size_t idx = static_cast<size_t>(k);
                    float xNormA = frame.freqNormA[idx];
                    float xNormB = frame.freqNormB[idx];
                    if (xNormA <= 0.001f && k > 0)
                        xNormA = static_cast<float>(k) / static_cast<float>(numBins - 1);
                    if (xNormB <= 0.001f && k > 0)
                        xNormB = static_cast<float>(k) / static_cast<float>(numBins - 1);

                    float ampA = frame.ampA[idx];
                    float ampB = frame.ampB[idx];

                    if (!frame.voiceActive)
                    {
                        const float hGainA = synthEngine.getResynthesisEngine().getHarmonicBarUI(0, k);
                        const float hGainB = synthEngine.getResynthesisEngine().getHarmonicBarUI(1, k);
                        const float breathe = 0.65f + 0.15f * std::sin(animPhase - zNorm * 4.0f + xNormA * 8.0f);
                        ampA = hGainA * breathe * frame.filterCurve[idx] * 0.78f;
                        ampB = hGainB * breathe * frame.filterCurve[idx] * 0.72f;
                    }

                    const auto ptA = project3D(stage, xNormA, ampA, zNorm);
                    const auto ptB = project3D(stage, xNormB, ampB * 0.92f, std::min(1.0f, zNorm + 0.06f));

                    if (k == 0)
                    {
                        wavePathA.startNewSubPath(ptA);
                        wavePathB.startNewSubPath(ptB);
                    }
                    else
                    {
                        wavePathA.lineTo(ptA);
                        wavePathB.lineTo(ptB);
                    }
                }

                // Draw Frequency Field 2 (Part B - Deep Magenta)
                if (slice == 0)
                {
                    juce::Path fillB = wavePathB;
                    fillB.lineTo(project3D(stage, 1.0f, 0.0f, 0.06f));
                    fillB.lineTo(project3D(stage, 0.0f, 0.0f, 0.06f));
                    fillB.closeSubPath();
                    g.setColour(Palette::deepMagenta.withAlpha(0.11f));
                    g.fillPath(fillB);

                    g.setColour(Palette::deepMagenta.withAlpha(0.92f));
                    g.strokePath(wavePathB, juce::PathStrokeType(2.0f));
                }
                else
                {
                    g.setColour(Palette::deepMagenta.withAlpha(depthAlpha * 0.36f));
                    g.strokePath(wavePathB, juce::PathStrokeType(1.0f));
                }

                // Draw Frequency Field 1 (Part A - Electric Cyan)
                if (slice == 0)
                {
                    juce::Path frontFill = wavePathA;
                    const auto baseRight = project3D(stage, 1.0f, 0.0f, 0.0f);
                    const auto baseLeft  = project3D(stage, 0.0f, 0.0f, 0.0f);
                    frontFill.lineTo(baseRight);
                    frontFill.lineTo(baseLeft);
                    frontFill.closeSubPath();

                    g.setColour(Palette::electricCyan.withAlpha(0.14f));
                    g.fillPath(frontFill);

                    g.setColour(Palette::electricCyan.withAlpha(0.35f));
                    g.strokePath(wavePathA, juce::PathStrokeType(4.0f));
                    g.setColour(Palette::electricCyan);
                    g.strokePath(wavePathA, juce::PathStrokeType(2.0f));

                    for (int k = 0; k < numBins; k += 2)
                    {
                        const size_t idx = static_cast<size_t>(k);
                        const float xNorm = frame.freqNormA[idx];
                        const float amp = frame.voiceActive
                            ? frame.ampA[idx]
                            : (synthEngine.getResynthesisEngine().getHarmonicBarUI(0, k) * frame.filterCurve[idx] * 0.72f);
                        if (amp > 0.02f)
                        {
                            const auto pBase = project3D(stage, xNorm, 0.0f, 0.0f);
                            const auto pTop  = project3D(stage, xNorm, amp, 0.0f);
                            g.setColour(Palette::electricCyan.withAlpha(0.42f));
                            g.drawLine(pBase.x, pBase.y, pTop.x, pTop.y, 1.1f);
                            g.setColour(Palette::textPrimary);
                            g.fillEllipse(pTop.x - 1.5f, pTop.y - 1.5f, 3.0f, 3.0f);
                        }
                    }
                }
                else
                {
                    g.setColour(Palette::electricCyan.withAlpha(depthAlpha * 0.46f));
                    g.strokePath(wavePathA, juce::PathStrokeType(1.0f));
                }
            }

            // Live Dual Filter + Parametric EQ Spectral Transfer Envelope (Energetic Amber)
            {
                juce::Path filterPath;
                for (int k = 0; k < numBins; ++k)
                {
                    const float xNorm = static_cast<float>(k) / static_cast<float>(numBins - 1);
                    const float fVal = latestSnapshot.filterCurve[static_cast<size_t>(k)];
                    const auto pt = project3D(stage, xNorm, fVal, 0.0f);
                    if (k == 0)
                        filterPath.startNewSubPath(pt);
                    else
                        filterPath.lineTo(pt);
                }
                const float dash[] = { 4.0f, 3.0f };
                juce::Path dashedFilter;
                juce::PathStrokeType(1.5f).createDashedStroke(dashedFilter, filterPath, dash, 2);
                g.setColour(Palette::energeticAmber.withAlpha(0.88f));
                g.fillPath(dashedFilter);
            }
        }

        // 2. Expanded & Deeper Harmonic Editor (with 2-row sculpting toolbar + 64 partials)
        drawHarmonicEditor(g, getHarmonicEditorBounds());

        // 3. Bottom Resynthesis Matrix Strip (Image / Audio Resynthesis Preview & Controls)
        drawResynthesisDock(g, getResynthesisDockBounds());

        if (isDragOver)
        {
            g.setColour(Palette::electricCyan.withAlpha(0.18f));
            g.fillRoundedRectangle(bounds, 5.0f);
            g.setColour(Palette::electricCyan);
            g.drawRoundedRectangle(bounds.reduced(2.0f), 5.0f, 2.0f);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
            g.drawText("DROP IMAGE (.PNG/.JPG) OR AUDIO (.WAV) FOR SPECTRAL RESYNTHESIS",
                       bounds, juce::Justification::centred, false);
        }
        else
        {
            g.setColour(Palette::borderSubtle);
            g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        lastDragPartialIdx = -1;
        const auto bounds = getLocalBounds().toFloat();

        // Check 3D VIEW / 2D VIEW toggle buttons in top-right header
        if (e.position.y <= 22.0f)
        {
            const juce::Rectangle<float> btn3D(bounds.getRight() - 108.0f, bounds.getY() + 3.0f, 48.0f, 16.0f);
            const juce::Rectangle<float> btn2D(bounds.getRight() - 56.0f,  bounds.getY() + 3.0f, 48.0f, 16.0f);
            if (btn3D.contains(e.position))
            {
                is2DView = false;
                repaint();
                return;
            }
            if (btn2D.contains(e.position))
            {
                is2DView = true;
                repaint();
                return;
            }
        }

        const auto harmRect = getHarmonicEditorBounds();
        if (harmRect.contains(e.position))
        {
            handleHarmonicEditorMouse(e.position, harmRect, true);
            return;
        }

        const auto dockRect = getResynthesisDockBounds();
        if (dockRect.contains(e.position))
        {
            handleResynthesisDockClick(e.position, dockRect);
            return;
        }

        const auto stage = get3DStageBounds();
        if (is2DView && stage.contains(e.position))
        {
            handle2DStageSculpt(e.position, stage);
            return;
        }

        dragStartPos = e.position;
        startYaw = cameraYaw;
        startPitch = cameraPitch;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const auto harmRect = getHarmonicEditorBounds();
        if (harmRect.contains(e.mouseDownPosition))
        {
            handleHarmonicEditorMouse(e.position, harmRect, false);
            return;
        }

        if (getResynthesisDockBounds().contains(e.mouseDownPosition))
            return;

        const auto stage = get3DStageBounds();
        if (is2DView && stage.contains(e.mouseDownPosition))
        {
            handle2DStageSculpt(e.position, stage);
            return;
        }

        const auto bounds = getLocalBounds().toFloat();
        const float dx = (e.position.x - dragStartPos.x) / std::max(100.0f, bounds.getWidth());
        const float dy = (e.position.y - dragStartPos.y) / std::max(100.0f, bounds.getHeight());
        cameraYaw   = juce::jlimit(-0.45f, 0.45f, startYaw + dx * 0.9f);
        cameraPitch = juce::jlimit(0.15f, 0.65f, startPitch + dy * 0.7f);
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        lastDragPartialIdx = -1;
    }

private:
    static constexpr int HISTORY_DEPTH = 22;
    static constexpr int VISIBLE_HARMONIC_BARS = 64;

    juce::Rectangle<float> get3DStageBounds() const noexcept
    {
        const auto b = getLocalBounds().toFloat();
        return b.withTrimmedTop(22.0f).withHeight(b.getHeight() - 270.0f).reduced(6.0f, 2.0f);
    }

    // Significantly Larger & Deeper Harmonic Editor (184px tall!)
    juce::Rectangle<float> getHarmonicEditorBounds() const noexcept
    {
        const auto b = getLocalBounds().toFloat();
        return juce::Rectangle<float>(b.getX() + 6.0f, b.getBottom() - 242.0f, b.getWidth() - 12.0f, 182.0f);
    }

    juce::Rectangle<float> getResynthesisDockBounds() const noexcept
    {
        const auto b = getLocalBounds().toFloat();
        return juce::Rectangle<float>(b.getX() + 6.0f, b.getBottom() - 56.0f, b.getWidth() - 12.0f, 50.0f);
    }

    juce::Point<float> project3D(juce::Rectangle<float> stage, float xNorm, float yAmp, float zDepth) const noexcept
    {
        const float persp = 1.0f / (1.0f + zDepth * 0.52f);
        const float centeredX = (xNorm - 0.5f) * 2.0f;

        const float isoX = (centeredX * 0.82f + (zDepth - 0.3f) * cameraYaw) * persp;
        const float baseFloorY = stage.getBottom() - 14.0f - (zDepth * stage.getHeight() * cameraPitch * 0.68f);
        const float heightScale = stage.getHeight() * 0.56f * persp;

        const float screenX = stage.getCentreX() + isoX * (stage.getWidth() * 0.48f);
        const float screenY = baseFloorY - yAmp * heightScale;
        return { screenX, screenY };
    }

    void draw3DFloorGrid(juce::Graphics& g, juce::Rectangle<float> stage)
    {
        g.setColour(Palette::borderSubtle.withAlpha(0.45f));

        constexpr int freqCols = 8;
        for (int c = 0; c <= freqCols; ++c)
        {
            const float xN = static_cast<float>(c) / static_cast<float>(freqCols);
            const auto pFront = project3D(stage, xN, 0.0f, 0.0f);
            const auto pBack  = project3D(stage, xN, 0.0f, 1.0f);
            g.drawLine(pFront.x, pFront.y, pBack.x, pBack.y, 1.0f);
        }

        constexpr int depthRows = 5;
        for (int r = 0; r <= depthRows; ++r)
        {
            const float zN = static_cast<float>(r) / static_cast<float>(depthRows);
            const auto pLeft  = project3D(stage, 0.0f, 0.0f, zN);
            const auto pRight = project3D(stage, 1.0f, 0.0f, zN);
            g.drawLine(pLeft.x, pLeft.y, pRight.x, pRight.y, 1.0f);
        }

        static const char* freqLabels[] = { "30Hz", "120Hz", "500Hz", "2kHz", "18kHz" };
        g.setColour(Palette::textDim);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
        for (int i = 0; i < 5; ++i)
        {
            const float xN = static_cast<float>(i) / 4.0f;
            const auto pt = project3D(stage, xN, 0.0f, 0.0f);
            g.drawText(freqLabels[i], static_cast<int>(pt.x - 18.0f), static_cast<int>(pt.y + 1.0f), 36, 11, juce::Justification::centred, false);
        }
    }

    void draw2DSpectralStage(juce::Graphics& g, juce::Rectangle<float> stage)
    {
        const auto plot = stage.reduced(4.0f, 4.0f);
        g.setColour(Palette::bgObsidian.withAlpha(0.75f));
        g.fillRoundedRectangle(plot, 4.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(plot, 4.0f, 1.0f);

        // Precision 2D Grid
        g.setColour(Palette::borderSubtle.withAlpha(0.45f));
        for (int c = 1; c < 8; ++c)
        {
            const float gx = plot.getX() + plot.getWidth() * (static_cast<float>(c) / 8.0f);
            g.drawVerticalLine(static_cast<int>(gx), plot.getY(), plot.getBottom());
        }
        for (int r = 1; r < 4; ++r)
        {
            const float gy = plot.getY() + plot.getHeight() * (static_cast<float>(r) / 4.0f);
            g.drawHorizontalLine(static_cast<int>(gy), plot.getX(), plot.getRight());
        }

        constexpr int numBins = PrismorDSP::VisualizerSnapshot::NUM_BINS;
        const float plotBottom = plot.getBottom() - 12.0f;
        const float plotHeight = plot.getHeight() - 18.0f;

        juce::Path pathA, fillA, pathB, fillB, filtPath;

        for (int k = 0; k < numBins; ++k)
        {
            const size_t idx = static_cast<size_t>(k);
            const float xNorm = static_cast<float>(k) / static_cast<float>(numBins - 1);
            const float px = plot.getX() + 4.0f + xNorm * (plot.getWidth() - 8.0f);

            float ampA = latestSnapshot.ampA[idx];
            float ampB = latestSnapshot.ampB[idx];
            if (!latestSnapshot.voiceActive)
            {
                const float hGainA = synthEngine.getResynthesisEngine().getHarmonicBarUI(0, k);
                const float hGainB = synthEngine.getResynthesisEngine().getHarmonicBarUI(1, k);
                ampA = hGainA * latestSnapshot.filterCurve[idx] * 0.85f;
                ampB = hGainB * latestSnapshot.filterCurve[idx] * 0.78f;
            }

            const float pyA = plotBottom - juce::jlimit(0.0f, 1.0f, ampA) * plotHeight;
            const float pyB = plotBottom - juce::jlimit(0.0f, 1.0f, ampB) * plotHeight;
            const float pyF = plotBottom - juce::jlimit(0.0f, 1.0f, latestSnapshot.filterCurve[idx]) * plotHeight;

            if (k == 0)
            {
                pathA.startNewSubPath(px, pyA);
                fillA.startNewSubPath(px, plotBottom);
                fillA.lineTo(px, pyA);

                pathB.startNewSubPath(px, pyB);
                fillB.startNewSubPath(px, plotBottom);
                fillB.lineTo(px, pyB);

                filtPath.startNewSubPath(px, pyF);
            }
            else
            {
                pathA.lineTo(px, pyA);
                fillA.lineTo(px, pyA);

                pathB.lineTo(px, pyB);
                fillB.lineTo(px, pyB);

                filtPath.lineTo(px, pyF);
            }

            // Crisp 2D spectral stems
            if (k % 2 == 0)
            {
                g.setColour(Palette::deepMagenta.withAlpha(0.32f));
                g.drawVerticalLine(static_cast<int>(px + 1.0f), pyB, plotBottom);
                g.setColour(Palette::electricCyan.withAlpha(0.42f));
                g.drawVerticalLine(static_cast<int>(px), pyA, plotBottom);
            }
        }

        fillA.lineTo(plot.getRight() - 4.0f, plotBottom);
        fillA.closeSubPath();
        fillB.lineTo(plot.getRight() - 4.0f, plotBottom);
        fillB.closeSubPath();

        g.setColour(Palette::deepMagenta.withAlpha(0.14f));
        g.fillPath(fillB);
        g.setColour(Palette::deepMagenta);
        g.strokePath(pathB, juce::PathStrokeType(1.6f));

        g.setColour(Palette::electricCyan.withAlpha(0.16f));
        g.fillPath(fillA);
        g.setColour(Palette::electricCyan);
        g.strokePath(pathA, juce::PathStrokeType(2.0f));

        const float dash[] = { 4.0f, 3.0f };
        juce::Path dashedFilter;
        juce::PathStrokeType(1.5f).createDashedStroke(dashedFilter, filtPath, dash, 2);
        g.setColour(Palette::energeticAmber);
        g.fillPath(dashedFilter);

        // Single-Cycle Reconstructed 2D Waveform Scope Inset (Top-Right of 2D Stage)
        const juce::Rectangle<float> waveBox(plot.getRight() - 122.0f, plot.getY() + 5.0f, 116.0f, 44.0f);
        g.setColour(Palette::cardSurface.withAlpha(0.92f));
        g.fillRoundedRectangle(waveBox, 3.0f);
        g.setColour(Palette::borderBright);
        g.drawRoundedRectangle(waveBox, 3.0f, 1.0f);

        juce::Path cycleA, cycleB;
        const auto& resynth = synthEngine.getResynthesisEngine();
        constexpr int wSteps = 64;
        for (int i = 0; i <= wSteps; ++i)
        {
            const float phase = (static_cast<float>(i) / static_cast<float>(wSteps)) * juce::MathConstants<float>::twoPi;
            float sumA = 0.0f, sumB = 0.0f;
            for (int h = 0; h < 16; ++h)
            {
                const float gA = resynth.getHarmonicGainRaw(0, h);
                const float gB = resynth.getHarmonicGainRaw(1, h);
                const float pA = resynth.getHarmonicPhaseRaw(0, h) * juce::MathConstants<float>::twoPi;
                const float pB = resynth.getHarmonicPhaseRaw(1, h) * juce::MathConstants<float>::twoPi;
                sumA += gA * std::sin(static_cast<float>(h + 1) * phase + pA);
                sumB += gB * std::sin(static_cast<float>(h + 1) * phase + pB);
            }
            const float wx = waveBox.getX() + 4.0f + (static_cast<float>(i) / static_cast<float>(wSteps)) * (waveBox.getWidth() - 8.0f);
            const float wyA = waveBox.getCentreY() + 2.0f - juce::jlimit(-1.0f, 1.0f, sumA * 0.42f) * (waveBox.getHeight() * 0.36f);
            const float wyB = waveBox.getCentreY() + 2.0f - juce::jlimit(-1.0f, 1.0f, sumB * 0.42f) * (waveBox.getHeight() * 0.36f);
            if (i == 0) { cycleA.startNewSubPath(wx, wyA); cycleB.startNewSubPath(wx, wyB); }
            else        { cycleA.lineTo(wx, wyA);          cycleB.lineTo(wx, wyB); }
        }
        g.setColour(Palette::deepMagenta.withAlpha(0.75f));
        g.strokePath(cycleB, juce::PathStrokeType(1.2f));
        g.setColour(Palette::electricCyan);
        g.strokePath(cycleA, juce::PathStrokeType(1.5f));
        g.setColour(Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.0f, juce::Font::bold));
        g.drawText("2D CYCLE WAVE", waveBox.reduced(4.0f, 1.0f), juce::Justification::topLeft, false);

        static const char* freqLabels[] = { "30Hz", "120Hz", "500Hz", "2kHz", "18kHz" };
        g.setColour(Palette::textDim);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.5f, juce::Font::bold));
        for (int i = 0; i < 5; ++i)
        {
            const float xN = static_cast<float>(i) / 4.0f;
            const float lx = plot.getX() + 14.0f + xN * (plot.getWidth() - 28.0f);
            g.drawText(freqLabels[i], static_cast<int>(lx - 18.0f), static_cast<int>(plot.getBottom() - 11.0f), 36, 11, juce::Justification::centred, false);
        }
    }

    void handle2DStageSculpt(juce::Point<float> pos, juce::Rectangle<float> stage)
    {
        const auto plot = stage.reduced(4.0f, 4.0f);
        const float normX = juce::jlimit(0.0f, 1.0f, (pos.x - plot.getX()) / plot.getWidth());
        const float normY = juce::jlimit(0.0f, 1.0f, (plot.getBottom() - 12.0f - pos.y) / (plot.getHeight() - 18.0f));
        const int partialIdx = juce::jlimit(0, VISIBLE_HARMONIC_BARS - 1, static_cast<int>(normX * static_cast<float>(VISIBLE_HARMONIC_BARS)));
        synthEngine.getResynthesisEngine().setHarmonicBar(activePart, partialIdx, normY);
        repaint();
    }

    void drawHarmonicEditor(juce::Graphics& g, juce::Rectangle<float> rect)
    {
        g.setColour(Palette::cardSurface);
        g.fillRoundedRectangle(rect, 4.0f);

        const juce::Colour partAccent = (activePart == 0) ? Palette::electricCyan : Palette::deepMagenta;
        g.setColour(partAccent.withAlpha(0.65f));
        g.drawRoundedRectangle(rect, 4.0f, 1.1f);

        // Row 1: Field 1 / Field 2 selector tabs + 10 Waveform Generators (incl. Sine, Sync, Bell, X-Mod)
        const auto row1 = rect.reduced(5.0f, 3.0f).withHeight(17.0f);
        {
            // Field 1 / Field 2 switch pills
            juce::Rectangle<float> f1Btn(row1.getX(), row1.getY(), 58.0f, 16.0f);
            juce::Rectangle<float> f2Btn(row1.getX() + 60.0f, row1.getY(), 58.0f, 16.0f);

            g.setColour(activePart == 0 ? Palette::electricCyan.withAlpha(0.26f) : Palette::trackDark);
            g.fillRoundedRectangle(f1Btn, 2.5f);
            g.setColour(activePart == 0 ? Palette::electricCyan : Palette::borderSubtle);
            g.drawRoundedRectangle(f1Btn, 2.5f, 1.0f);
            g.setColour(activePart == 0 ? Palette::textPrimary : Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText("FIELD 1 [A]", f1Btn, juce::Justification::centred, false);

            g.setColour(activePart == 1 ? Palette::deepMagenta.withAlpha(0.26f) : Palette::trackDark);
            g.fillRoundedRectangle(f2Btn, 2.5f);
            g.setColour(activePart == 1 ? Palette::deepMagenta : Palette::borderSubtle);
            g.drawRoundedRectangle(f2Btn, 2.5f, 1.0f);
            g.setColour(activePart == 1 ? Palette::textPrimary : Palette::textSecondary);
            g.drawText("FIELD 2 [B]", f2Btn, juce::Justification::centred, false);

            static const char* genLabels[10] = {
                "SAW", "SQR", "TRI", "ORG", "FMT", "RND", "SIN", "SYN", "BEL", "XMD"
            };
            const int curWave = synthEngine.getResynthesisEngine().getActiveWaveType(activePart);
            const float availW = row1.getWidth() - 122.0f;
            const float btnW = availW / 10.0f;
            float bx = row1.getX() + 122.0f;
            for (int i = 0; i < 10; ++i)
            {
                const bool isSel = (curWave == i);
                const juce::Colour wCol = (i >= 6) ? Palette::energeticAmber : partAccent;
                juce::Rectangle<float> br(bx, row1.getY(), btnW - 1.8f, 16.0f);
                g.setColour(isSel ? wCol.withAlpha(0.28f) : Palette::trackDark);
                g.fillRoundedRectangle(br, 2.5f);
                g.setColour(isSel ? wCol : wCol.withAlpha(0.65f));
                g.drawRoundedRectangle(br, 2.5f, isSel ? 1.3f : 1.0f);
                g.setColour(isSel ? Palette::textPrimary : Palette::textSecondary);
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.2f, juce::Font::bold));
                g.drawText(genLabels[i], br, juce::Justification::centred, false);
                bx += btnW;
            }
        }

        // Row 2: Deep Harmonic Sculpting Toolset (Odd/Even, Filter LPF/HPF/Smooth, Shift/Stretch, Phase Rnd/Clr)
        const auto row2 = rect.reduced(5.0f, 0.0f).withTop(row1.getBottom() + 2.0f).withHeight(16.0f);
        {
            static const char* toolLabels[] = {
                "ODD+", "EVEN+", "LPF", "HPF", "SMOOTH", "<<SHF", "SHF>>", "STRCH", "RND PH", "0 PHS"
            };
            const float toolW = row2.getWidth() / 10.0f;
            for (int i = 0; i < 10; ++i)
            {
                juce::Rectangle<float> tr(row2.getX() + static_cast<float>(i) * toolW, row2.getY(), toolW - 2.0f, 15.0f);
                const juce::Colour tCol = (i < 2) ? partAccent : ((i < 5) ? Palette::energeticAmber : ((i < 8) ? Palette::neonEmerald : Palette::deepMagenta));
                g.setColour(Palette::trackDark);
                g.fillRoundedRectangle(tr, 2.0f);
                g.setColour(tCol.withAlpha(0.65f));
                g.drawRoundedRectangle(tr, 2.0f, 1.0f);
                g.setColour(Palette::textPrimary);
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.2f, juce::Font::bold));
                g.drawText(toolLabels[i], tr, juce::Justification::centred, false);
            }
        }

        // Interactive 64-Partial Harmonic Amplitude + Phase Canvas
        const auto barsArea  = rect.reduced(5.0f, 4.0f).withTrimmedTop(38.0f);
        const auto ampArea   = barsArea.withTrimmedBottom(26.0f);
        const auto phaseArea = barsArea.withTop(ampArea.getBottom() + 2.0f);

        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(ampArea, 3.0f);
        g.fillRoundedRectangle(phaseArea, 2.5f);

        // Subtle horizontal dB grid lines inside amplitude editor
        g.setColour(Palette::borderSubtle.withAlpha(0.35f));
        for (int r = 1; r <= 3; ++r)
        {
            const float gy = ampArea.getY() + ampArea.getHeight() * (static_cast<float>(r) / 4.0f);
            g.drawHorizontalLine(static_cast<int>(gy), ampArea.getX() + 2.0f, ampArea.getRight() - 2.0f);
        }

        const float slotW = ampArea.getWidth() / static_cast<float>(VISIBLE_HARMONIC_BARS);
        const auto& resynth = synthEngine.getResynthesisEngine();

        for (int k = 0; k < VISIBLE_HARMONIC_BARS; ++k)
        {
            const float uiVal = resynth.getHarmonicBarUI(activePart, k);
            const float phVal = resynth.getHarmonicPhaseRaw(activePart, k);
            const bool isOdd  = (k % 2 == 0); // k=0 is Fundamental (1st harmonic = odd)

            const float x = ampArea.getX() + static_cast<float>(k) * slotW;
            const float barW = std::max(1.5f, slotW - 1.5f);
            const float barH = uiVal * (ampArea.getHeight() - 4.0f);
            const juce::Rectangle<float> barRect(x + 0.75f, ampArea.getBottom() - 2.0f - barH, barW, barH);

            g.setColour(partAccent.withAlpha(isOdd ? 0.16f : 0.08f));
            g.fillRect(x + 0.75f, ampArea.getY() + 2.0f, barW, ampArea.getHeight() - 4.0f);

            g.setColour(partAccent.withAlpha(isOdd ? 0.92f : 0.68f));
            g.fillRect(barRect);

            if (barH > 2.0f)
            {
                g.setColour(Palette::textPrimary);
                g.fillRect(x + 0.75f, barRect.getY(), barW, 1.5f);
            }

            // Phase bar in bottom strip (Energetic Amber)
            const float phH = phVal * (phaseArea.getHeight() - 2.0f);
            g.setColour(Palette::energeticAmber.withAlpha(0.82f));
            g.fillRect(x + 0.75f, phaseArea.getBottom() - 1.0f - phH, barW, phH);
        }

        // Subtle label overlay in phase strip
        g.setColour(Palette::textDim.withAlpha(0.75f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.5f, juce::Font::bold));
        g.drawText("PARTIAL PHASE [0..360]", phaseArea.reduced(4.0f, 1.0f), juce::Justification::topRight, false);
    }

    void handleHarmonicEditorMouse(juce::Point<float> pos, juce::Rectangle<float> rect, bool isClick)
    {
        const auto row1 = rect.reduced(5.0f, 3.0f).withHeight(17.0f);
        const auto row2 = rect.reduced(5.0f, 0.0f).withTop(row1.getBottom() + 2.0f).withHeight(16.0f);

        if (isClick && pos.y <= row1.getBottom())
        {
            const float startX = row1.getX() + 122.0f;
            const float availW = row1.getWidth() - 122.0f;
            const float btnW = availW / 10.0f;
            if (pos.x >= startX && pos.x <= row1.getRight())
            {
                const int genIdx = juce::jlimit(0, 9, static_cast<int>((pos.x - startX) / btnW));
                synthEngine.getResynthesisEngine().resetPartTableToWaveform(activePart, genIdx);
                if (onWaveformSelected)
                    onWaveformSelected(activePart, genIdx);
                repaint();
                return;
            }
            if (pos.x < row1.getX() + 59.0f)
                activePart = 0;
            else if (pos.x < row1.getX() + 120.0f)
                activePart = 1;
            else
                activePart = 1 - activePart;

            if (onActivePartChanged)
                onActivePartChanged(activePart);
            repaint();
            return;
        }

        if (isClick && pos.y >= row2.getY() && pos.y <= row2.getBottom())
        {
            const float toolW = row2.getWidth() / 10.0f;
            const int toolIdx = juce::jlimit(0, 9, static_cast<int>((pos.x - row2.getX()) / toolW));
            auto& resynth = synthEngine.getResynthesisEngine();

            switch (toolIdx)
            {
                case 0: resynth.emphasizeOddEven(activePart, 1.25f, 0.78f); break; // ODD+
                case 1: resynth.emphasizeOddEven(activePart, 0.78f, 1.25f); break; // EVEN+
                case 2: resynth.filterHarmonicTable(activePart, 0);         break; // LPF
                case 3: resynth.filterHarmonicTable(activePart, 1);         break; // HPF
                case 4: resynth.filterHarmonicTable(activePart, 2);         break; // SMOOTH
                case 5: resynth.shiftOrStretchHarmonicTable(activePart, -1, false); break; // <<SHF
                case 6: resynth.shiftOrStretchHarmonicTable(activePart, +1, false); break; // SHF>>
                case 7: resynth.shiftOrStretchHarmonicTable(activePart, 0,  true);  break; // STRCH
                case 8: resynth.randomizeHarmonicPhases(activePart, false); break; // RND PH
                case 9: resynth.randomizeHarmonicPhases(activePart, true);  break; // 0 PHS
                default: break;
            }
            repaint();
            return;
        }

        const auto barsArea  = rect.reduced(5.0f, 4.0f).withTrimmedTop(38.0f);
        const auto ampArea   = barsArea.withTrimmedBottom(26.0f);
        const auto phaseArea = barsArea.withTop(ampArea.getBottom() + 2.0f);

        const int partialIdx = juce::jlimit(0, VISIBLE_HARMONIC_BARS - 1,
            static_cast<int>(((pos.x - ampArea.getX()) / ampArea.getWidth()) * static_cast<float>(VISIBLE_HARMONIC_BARS)));

        if (isClick)
            editingPhaseLane = (pos.y >= phaseArea.getY());

        if (editingPhaseLane)
        {
            const float normPhase = juce::jlimit(0.0f, 1.0f, (phaseArea.getBottom() - pos.y) / phaseArea.getHeight());
            if (!isClick && lastDragPartialIdx >= 0 && std::abs(partialIdx - lastDragPartialIdx) > 1)
            {
                const int p0 = std::min(lastDragPartialIdx, partialIdx);
                const int p1 = std::max(lastDragPartialIdx, partialIdx);
                for (int k = p0; k <= p1; ++k)
                {
                    const float t = static_cast<float>(k - lastDragPartialIdx) / static_cast<float>(partialIdx - lastDragPartialIdx);
                    const float interpVal = lastDragValue + t * (normPhase - lastDragValue);
                    synthEngine.getResynthesisEngine().setHarmonicPhaseBar(activePart, k, interpVal);
                }
            }
            else
            {
                synthEngine.getResynthesisEngine().setHarmonicPhaseBar(activePart, partialIdx, normPhase);
            }
            lastDragPartialIdx = partialIdx;
            lastDragValue = normPhase;
        }
        else
        {
            const float normAmp = juce::jlimit(0.0f, 1.0f, (ampArea.getBottom() - pos.y) / ampArea.getHeight());
            if (!isClick && lastDragPartialIdx >= 0 && std::abs(partialIdx - lastDragPartialIdx) > 1)
            {
                const int p0 = std::min(lastDragPartialIdx, partialIdx);
                const int p1 = std::max(lastDragPartialIdx, partialIdx);
                for (int k = p0; k <= p1; ++k)
                {
                    const float t = static_cast<float>(k - lastDragPartialIdx) / static_cast<float>(partialIdx - lastDragPartialIdx);
                    const float interpVal = lastDragValue + t * (normAmp - lastDragValue);
                    synthEngine.getResynthesisEngine().setHarmonicBar(activePart, k, interpVal);
                }
            }
            else
            {
                synthEngine.getResynthesisEngine().setHarmonicBar(activePart, partialIdx, normAmp);
            }
            lastDragPartialIdx = partialIdx;
            lastDragValue = normAmp;
        }
        repaint();
    }

    void drawResynthesisDock(juce::Graphics& g, juce::Rectangle<float> dock)
    {
        g.setColour(Palette::cardSurface);
        g.fillRoundedRectangle(dock, 4.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(dock, 4.0f, 1.0f);

        const auto thumbRect = dock.withWidth(72.0f).reduced(4.0f);
        const auto previewImg = synthEngine.getResynthesisEngine().getPreviewImage();
        if (previewImg.isValid())
            g.drawImage(previewImg, thumbRect, juce::RectanglePlacement::stretchToFit);
        g.setColour(Palette::electricCyan.withAlpha(0.6f));
        g.drawRect(thumbRect, 1.0f);

        const auto rightArea = dock.withTrimmedLeft(76.0f).reduced(4.0f, 3.0f);
        const auto topRow = rightArea.withHeight(14.0f);
        const auto botRow = rightArea.withTrimmedTop(16.0f);

        g.setColour(Palette::electricCyan);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.drawText(synthEngine.getResynthesisEngine().getSourceName(), topRow, juce::Justification::centredLeft, false);

        static const char* btnLabels[] = { "FORMANT", "HYPERSAW", "NEURO", "CRYSTAL", "LOAD IMG", "LOAD WAV" };
        const float btnW = botRow.getWidth() / 6.0f;
        for (int i = 0; i < 6; ++i)
        {
            juce::Rectangle<float> r(botRow.getX() + static_cast<float>(i) * btnW, botRow.getY(), btnW - 3.0f, botRow.getHeight());
            const bool isFileBtn = (i >= 4);
            const juce::Colour col = isFileBtn ? Palette::deepMagenta : Palette::electricCyan;

            g.setColour(col.withAlpha(0.14f));
            g.fillRoundedRectangle(r, 3.0f);
            g.setColour(col.withAlpha(0.75f));
            g.drawRoundedRectangle(r, 3.0f, 1.0f);

            g.setColour(Palette::textPrimary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
            g.drawText(btnLabels[i], r, juce::Justification::centred, false);
        }
    }

    void handleResynthesisDockClick(juce::Point<float> pos, juce::Rectangle<float> dock)
    {
        const auto rightArea = dock.withTrimmedLeft(76.0f).reduced(4.0f, 3.0f);
        const auto botRow = rightArea.withTrimmedTop(16.0f);
        if (!botRow.contains(pos))
            return;

        const float btnW = botRow.getWidth() / 6.0f;
        const int idx = juce::jlimit(0, 5, static_cast<int>((pos.x - botRow.getX()) / btnW));

        if (idx < 4)
        {
            synthEngine.getResynthesisEngine().generateProceduralPlane(idx);
            repaint();
        }
        else if (idx == 4)
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Image for Spectral Resynthesis",
                juce::File(),
                "*.png;*.jpg;*.jpeg;*.bmp");
            fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        synthEngine.getResynthesisEngine().loadImageFile(file);
                        repaint();
                    }
                });
        }
        else if (idx == 5)
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Audio for Spectral Resynthesis",
                juce::File(),
                "*.wav;*.aif;*.aiff;*.flac");
            fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        synthEngine.getResynthesisEngine().loadAudioFile(file);
                        repaint();
                    }
                });
        }
    }

    PrismorDSP::SpectralSynthEngine& synthEngine;
    std::array<PrismorDSP::VisualizerSnapshot, HISTORY_DEPTH> history {};
    PrismorDSP::VisualizerSnapshot latestSnapshot {};
    int writeHead { 0 };
    int activePart { 0 };
    int lastDragPartialIdx { -1 };
    float lastDragValue { 0.5f };
    bool editingPhaseLane { false };
    float animPhase { 0.0f };
    float cameraYaw { 0.22f };
    float cameraPitch { 0.38f };
    juce::Point<float> dragStartPos {};
    float startYaw { 0.22f };
    float startPitch { 0.38f };
    bool is2DView { false };
    bool isDragOver { false };
    std::unique_ptr<juce::FileChooser> fileChooser;
};

} // namespace PrismorUI
