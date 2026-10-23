#pragma once
#include <JuceHeader.h>
#include "../DSP/SpectralSynthEngine.h"
#include "../PluginProcessor.h"
#include "PrismorLookAndFeel.h"

namespace PrismorUI
{

class ArticulatorEditor : public juce::Component
{
public:
    explicit ArticulatorEditor(PrismorDSP::SpectralSynthEngine& engineRef)
        : synthEngine(engineRef)
    {
        setOpaque(false);
    }

    void setPlaybackCursor(float cursorPos, bool isVoiceActive)
    {
        liveCursor = cursorPos;
        voiceActive = isVoiceActive;
        repaint();
    }

    void setPlaybackSnapshot(const PrismorDSP::VisualizerSnapshot& snap)
    {
        const size_t tIdx = static_cast<size_t>(juce::jlimit(0, 5, static_cast<int>(currentTarget)));
        liveCursor = snap.envCursors[tIdx];
        voiceActive = snap.voiceActive;
        repaint();
    }

    void setTarget(PrismorDSP::ArticulatorTarget newTarget)
    {
        currentTarget = newTarget;
        selectedNodeIndex = -1;
        draggingTensionSegment = -1;
        draggingRateBox = false;
        repaint();
    }

    PrismorDSP::ArticulatorTarget getTarget() const noexcept { return currentTarget; }

    juce::Colour getTargetAccent() const noexcept
    {
        switch (currentTarget)
        {
            case PrismorDSP::ArticulatorTarget::Volume:            return Palette::goldOrange;
            case PrismorDSP::ArticulatorTarget::Pitch:             return Palette::iceBlue;
            case PrismorDSP::ArticulatorTarget::Filter1Cutoff:     return Palette::goldOrange;
            case PrismorDSP::ArticulatorTarget::CustomFilterShape: return Palette::pinkRed;
            case PrismorDSP::ArticulatorTarget::Timbre:            return Palette::iceBlue;
            case PrismorDSP::ArticulatorTarget::Prism:             return Palette::pinkRed;
            case PrismorDSP::ArticulatorTarget::Count:
            default:                                               return Palette::goldOrange;
        }
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const auto topToolbar = bounds.withHeight(21.0f);
        const auto presetStrip = bounds.withTop(23.0f).withHeight(18.0f);
        const auto canvas = getCanvasBounds();

        // 1. Top Toolbar (Targets + Curve Shape/Tension + Rate/Sync + Loop + Reset)
        drawTopToolbar(g, topToolbar);

        // 2. ShaperBox 3 Style Gate/Curve Preset Bar
        drawShaperBoxPresetStrip(g, presetStrip);

        // 3. Recessed Pitch-Black Display Canvas (#090B10 with 1px dark trench)
        g.setColour(Palette::borderDark);
        g.drawRoundedRectangle(canvas.expanded(0.5f), 4.0f, 1.0f);

        g.setColour(juce::Colour(0xFF090B10));
        g.fillRoundedRectangle(canvas, 4.0f);

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine(static_cast<int>(canvas.getY() + 1.0f), canvas.getX() + 2.0f, canvas.getRight() - 2.0f);

        // Precision rhythmic grid with ultra-fine low-opacity grid lines (rgba(255,255,255,0.04))
        constexpr int vSubCols = 16;
        constexpr int hRows = 4;
        for (int c = 1; c < vSubCols; ++c)
        {
            const float gx = canvas.getX() + canvas.getWidth() * (static_cast<float>(c) / static_cast<float>(vSubCols));
            const bool isBeatLine = (c % 4 == 0);
            g.setColour(juce::Colours::white.withAlpha(isBeatLine ? 0.075f : 0.035f));
            g.drawVerticalLine(static_cast<int>(gx), canvas.getY(), canvas.getBottom());
        }
        for (int r = 1; r < hRows; ++r)
        {
            const float gy = canvas.getY() + canvas.getHeight() * (static_cast<float>(r) / static_cast<float>(hRows));
            g.setColour(juce::Colours::white.withAlpha(0.045f));
            g.drawHorizontalLine(static_cast<int>(gy), canvas.getX(), canvas.getRight());
        }

        // Bipolar center reference line for Pitch / Filter / Prism
        if (currentTarget == PrismorDSP::ArticulatorTarget::Pitch ||
            currentTarget == PrismorDSP::ArticulatorTarget::Filter1Cutoff ||
            currentTarget == PrismorDSP::ArticulatorTarget::Prism)
        {
            const float midY = canvas.getCentreY();
            g.setColour(Palette::textDim.withAlpha(0.5f));
            const float dashLengths[] = { 4.0f, 4.0f };
            g.drawDashedLine(juce::Line<float>(canvas.getX(), midY, canvas.getRight(), midY), dashLengths, 2, 1.0f);
        }

        const auto& curve = synthEngine.getArticulator(currentTarget);
        const auto nodes = curve.getNodesCopy();
        const juce::Colour accent = getTargetAccent();

        // Build spline path & shaded area
        juce::Path curvePath;
        juce::Path fillPath;

        constexpr int numSteps = 192;
        for (int i = 0; i <= numSteps; ++i)
        {
            const float normX = static_cast<float>(i) / static_cast<float>(numSteps);
            const float normY = curve.evaluateLUT(normX);
            const float px = canvas.getX() + normX * canvas.getWidth();
            const float py = canvas.getBottom() - normY * canvas.getHeight();

            if (i == 0)
            {
                curvePath.startNewSubPath(px, py);
                fillPath.startNewSubPath(px, canvas.getBottom());
                fillPath.lineTo(px, py);
            }
            else
            {
                curvePath.lineTo(px, py);
                fillPath.lineTo(px, py);
            }
        }
        fillPath.lineTo(canvas.getRight(), canvas.getBottom());
        fillPath.closeSubPath();

        // Rich translucent gradient fill underneath active curve
        juce::ColourGradient grad(accent.withAlpha(0.24f), 0.0f, canvas.getY(),
                                  accent.withAlpha(0.02f), 0.0f, canvas.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(fillPath);

        // Glowing primary spline curve
        g.setColour(accent.withAlpha(0.32f));
        g.strokePath(curvePath, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accent);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Live playback cursor
        if (voiceActive && currentTarget != PrismorDSP::ArticulatorTarget::CustomFilterShape)
        {
            const float curX = canvas.getX() + juce::jlimit(0.0f, 1.0f, liveCursor) * canvas.getWidth();
            const float curVal = curve.evaluateLUT(liveCursor);
            const float curY = canvas.getBottom() - curVal * canvas.getHeight();

            g.setColour(Palette::textPrimary.withAlpha(0.65f));
            g.drawVerticalLine(static_cast<int>(curX), canvas.getY(), canvas.getBottom());

            g.setColour(accent);
            g.fillEllipse(curX - 4.0f, curY - 4.0f, 8.0f, 8.0f);
        }

        // Draw Segment Tension/Shape Handles (diamonds at midpoint of each segment)
        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const float midNormX = 0.5f * (nodes[i].x + nodes[i + 1].x);
            const float midNormY = curve.evaluateNodesDirect(midNormX);
            const float tx = canvas.getX() + midNormX * canvas.getWidth();
            const float ty = canvas.getBottom() - midNormY * canvas.getHeight();

            const bool isDraggingThis = (static_cast<int>(i) == draggingTensionSegment);
            const float d = isDraggingThis ? 6.0f : 4.5f;

            juce::Path diamond;
            diamond.startNewSubPath(tx, ty - d);
            diamond.lineTo(tx + d, ty);
            diamond.lineTo(tx, ty + d);
            diamond.lineTo(tx - d, ty);
            diamond.closeSubPath();

            const juce::Colour handleCol = (nodes[i].shapeMode == 1)
                ? Palette::deepMagenta
                : (nodes[i].shapeMode == 2 ? Palette::neonEmerald : accent);

            if (isDraggingThis)
            {
                g.setColour(handleCol.withAlpha(0.32f));
                g.fillEllipse(tx - 9.0f, ty - 9.0f, 18.0f, 18.0f);
            }

            g.setColour(Palette::bgObsidian);
            g.fillPath(diamond);
            g.setColour(isDraggingThis ? Palette::textPrimary : handleCol);
            g.strokePath(diamond, juce::PathStrokeType(1.6f));
        }

        // Draw Primary Spline Control Nodes
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const float nx = canvas.getX() + nodes[i].x * canvas.getWidth();
            const float ny = canvas.getBottom() - nodes[i].y * canvas.getHeight();
            const bool isSel = (static_cast<int>(i) == selectedNodeIndex);

            const float r = isSel ? 5.5f : 4.4f;
            if (isSel)
            {
                g.setColour(accent.withAlpha(0.35f));
                g.fillEllipse(nx - 8.5f, ny - 8.5f, 17.0f, 17.0f);
            }

            g.setColour(Palette::bgObsidian);
            g.fillEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f);
            g.setColour(isSel ? Palette::textPrimary : accent);
            g.drawEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f, 1.8f);
        }

        // Caption in bottom-right of canvas
        g.setColour(Palette::textDim);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.8f, juce::Font::plain));
        const juce::String hint = "DRAG DIAMOND: CURVE TENSION  |  R-CLICK DIAMOND: EXP / S-CURVE / LINEAR  |  DBL-CLICK: ADD/DEL NODE";
        g.drawText(hint, canvas.reduced(6.0f, 2.0f), juce::Justification::bottomRight, false);

        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(canvas, 4.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto pos = e.position;

        if (pos.y < 22.0f)
        {
            handleTopToolbarClick(pos);
            return;
        }
        if (pos.y < 42.0f)
        {
            handlePresetStripClick(pos);
            return;
        }

        const auto canvas = getCanvasBounds();
        if (!canvas.contains(pos))
            return;

        auto& curve = synthEngine.getArticulator(currentTarget);
        const auto nodes = curve.getNodesCopy();

        // 1. Check if clicking a segment tension diamond FIRST so midpoints are always easy to grab
        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const float midNormX = 0.5f * (nodes[i].x + nodes[i + 1].x);
            const float midNormY = curve.evaluateNodesDirect(midNormX);
            const float tx = canvas.getX() + midNormX * canvas.getWidth();
            const float ty = canvas.getBottom() - midNormY * canvas.getHeight();
            if (pos.getDistanceFrom({ tx, ty }) <= 11.0f)
            {
                if (e.mods.isRightButtonDown())
                {
                    curve.cycleSegmentShapeMode(static_cast<int>(i));
                    repaint();
                    return;
                }
                draggingTensionSegment = static_cast<int>(i);
                tensionDragStartY = pos.y;
                tensionDragStartVal = nodes[i].tension;
                if (nodes[i].shapeMode == 2) // If linear, switch to Exp/Log when user drags tension
                    curve.cycleSegmentShapeMode(static_cast<int>(i));
                selectedNodeIndex = -1;
                repaint();
                return;
            }
        }

        // 2. Check if clicking a primary node
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const float nx = canvas.getX() + nodes[i].x * canvas.getWidth();
            const float ny = canvas.getBottom() - nodes[i].y * canvas.getHeight();
            if (pos.getDistanceFrom({ nx, ny }) <= 10.0f)
            {
                if (e.mods.isRightButtonDown())
                {
                    curve.removeNode(static_cast<int>(i));
                    selectedNodeIndex = -1;
                    repaint();
                    return;
                }
                selectedNodeIndex = static_cast<int>(i);
                draggingTensionSegment = -1;
                repaint();
                return;
            }
        }

        // 3. Right click on empty canvas adds a node
        if (e.mods.isRightButtonDown())
        {
            const float normX = (pos.x - canvas.getX()) / canvas.getWidth();
            const float normY = (canvas.getBottom() - pos.y) / canvas.getHeight();
            selectedNodeIndex = curve.addNode(normX, normY);
            repaint();
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        const auto pos = e.position;
        const auto canvas = getCanvasBounds();
        if (!canvas.contains(pos))
            return;

        auto& curve = synthEngine.getArticulator(currentTarget);
        const auto nodes = curve.getNodesCopy();

        // Double-clicking a tension diamond resets its tension to 0 (linear)
        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const float midNormX = 0.5f * (nodes[i].x + nodes[i + 1].x);
            const float midNormY = curve.evaluateNodesDirect(midNormX);
            const float tx = canvas.getX() + midNormX * canvas.getWidth();
            const float ty = canvas.getBottom() - midNormY * canvas.getHeight();
            if (pos.getDistanceFrom({ tx, ty }) <= 11.0f)
            {
                curve.updateTension(static_cast<int>(i), 0.0f);
                repaint();
                return;
            }
        }

        // Double-clicking a node removes it
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const float nx = canvas.getX() + nodes[i].x * canvas.getWidth();
            const float ny = canvas.getBottom() - nodes[i].y * canvas.getHeight();
            if (pos.getDistanceFrom({ nx, ny }) <= 10.0f)
            {
                curve.removeNode(static_cast<int>(i));
                selectedNodeIndex = -1;
                repaint();
                return;
            }
        }

        // Double-clicking empty canvas adds a node
        const float normX = (pos.x - canvas.getX()) / canvas.getWidth();
        const float normY = (canvas.getBottom() - pos.y) / canvas.getHeight();
        selectedNodeIndex = curve.addNode(normX, normY);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto& curve = synthEngine.getArticulator(currentTarget);

        if (draggingRateBox)
        {
            const float dy = (rateDragStartY - e.position.y);
            if (curve.isTempoSync())
            {
                const int deltaDiv = static_cast<int>(dy / 14.0f);
                curve.setSyncDivision(rateDragStartDiv + deltaDiv);
            }
            else
            {
                const float newRate = rateDragStartHz * std::pow(2.0f, dy / 45.0f);
                curve.setRate(newRate);
            }
            if (curve.getMode() == PrismorDSP::ArticulatorMode::Envelope && currentTarget != PrismorDSP::ArticulatorTarget::CustomFilterShape)
                curve.setMode(PrismorDSP::ArticulatorMode::Loop);
            synthEngine.notifyArticulatorRateEdited(currentTarget);
            repaint();
            return;
        }

        const auto canvas = getCanvasBounds();

        if (selectedNodeIndex >= 0)
        {
            const float normX = (e.position.x - canvas.getX()) / canvas.getWidth();
            const float normY = (canvas.getBottom() - e.position.y) / canvas.getHeight();
            curve.updateNode(selectedNodeIndex, normX, normY);
            repaint();
        }
        else if (draggingTensionSegment >= 0)
        {
            const auto nodes = curve.getNodesCopy();
            if (draggingTensionSegment + 1 < static_cast<int>(nodes.size()))
            {
                // Natural screen-Y mapping: dragging UP (smaller e.position.y) always bends curve UP
                const float dy = (e.position.y - tensionDragStartY) / std::max(36.0f, canvas.getHeight() * 0.45f);
                const float deltaY = nodes[static_cast<size_t>(draggingTensionSegment + 1)].y
                                   - nodes[static_cast<size_t>(draggingTensionSegment)].y;
                const float slopeSign = (deltaY >= 0.0f) ? 1.0f : -1.0f;
                curve.updateTension(draggingTensionSegment, tensionDragStartVal + dy * slopeSign);
                repaint();
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingTensionSegment = -1;
        draggingRateBox = false;
        repaint();
    }

private:
    juce::Rectangle<float> getCanvasBounds() const noexcept
    {
        return getLocalBounds().toFloat().withTrimmedTop(43.0f);
    }

    static void drawMiniButton(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label,
                               juce::Colour col, bool active)
    {
        g.setColour(active ? col.withAlpha(0.26f) : Palette::trackDark);
        g.fillRoundedRectangle(r, 2.5f);
        g.setColour(active ? col : Palette::borderSubtle);
        g.drawRoundedRectangle(r, 2.5f, active ? 1.2f : 1.0f);
        g.setColour(active ? Palette::textPrimary : Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.2f, juce::Font::bold));
        g.drawText(label, r, juce::Justification::centred, false);
    }

    void drawTopToolbar(juce::Graphics& g, juce::Rectangle<float> bar)
    {
        const auto& curve = synthEngine.getArticulator(currentTarget);

        // 1. Target Selector Tabs (VOLUME, PITCH, FILTER 1, CUSTOM, TIMBRE, PRISM)
        static const char* targetNames[] = { "VOLUME", "PITCH", "FILTER 1", "CUSTOM", "TIMBRE", "PRISM" };
        const float tabW = 44.0f;
        float x = bar.getX();

        for (int i = 0; i < 6; ++i)
        {
            const bool active = (static_cast<int>(currentTarget) == i);
            juce::Rectangle<float> r(x, bar.getY(), tabW - 2.0f, 19.0f);
            const juce::Colour tabCol = (i == 3 || i == 5) ? Palette::deepMagenta : ((i == 0) ? Palette::energeticAmber : Palette::electricCyan);
            drawMiniButton(g, r, targetNames[i], tabCol, active);
            x += tabW;
        }

        // 2. Curve Shape Mode & Tension Controls: [EXP] [S-CRV] [LINE] [T-] [T+]
        x += 4.0f;
        static const char* shapeModes[] = { "EXP", "S-CRV", "LINE" };
        for (int m = 0; m < 3; ++m)
        {
            juce::Rectangle<float> r(x, bar.getY(), 29.0f, 19.0f);
            drawMiniButton(g, r, shapeModes[m], Palette::deepMagenta, curve.getGlobalShapeMode() == m);
            x += 31.0f;
        }
        drawMiniButton(g, { x, bar.getY(), 20.0f, 19.0f }, "T-", Palette::energeticAmber, false);
        x += 22.0f;
        drawMiniButton(g, { x, bar.getY(), 20.0f, 19.0f }, "T+", Palette::energeticAmber, false);
        x += 24.0f;

        // 3. Rate / Speed & Tempo Sync Controls: [SYNC] [<] [RATE] [>] [LOOP]
        const bool isSync = curve.isTempoSync();
        drawMiniButton(g, { x, bar.getY(), 32.0f, 19.0f }, isSync ? "SYNC" : "FREE", Palette::electricCyan, isSync);
        x += 34.0f;

        drawMiniButton(g, { x, bar.getY(), 15.0f, 19.0f }, "<", Palette::electricCyan, false);
        x += 17.0f;

        const auto divNames = PrismorDSP::SplineCurve::getSyncDivisionNames();
        const juce::String rateLabel = isSync
            ? ("RATE: " + divNames[curve.getSyncDivision()])
            : ("RATE: " + juce::String(curve.getRate(), 1) + "Hz");
        rateValueBoxRect = { x, bar.getY(), 58.0f, 19.0f };
        drawMiniButton(g, rateValueBoxRect, rateLabel, Palette::electricCyan, true);
        x += 60.0f;

        drawMiniButton(g, { x, bar.getY(), 15.0f, 19.0f }, ">", Palette::electricCyan, false);
        x += 17.0f;

        const bool loopOn = (curve.getMode() != PrismorDSP::ArticulatorMode::Envelope);
        drawMiniButton(g, { x, bar.getY(), 32.0f, 19.0f }, "LOOP", Palette::neonEmerald, loopOn);

        // 4. Right-aligned FLAT & RESET buttons
        const float rstW = 36.0f;
        drawMiniButton(g, { bar.getRight() - rstW * 2.0f - 2.0f, bar.getY(), rstW, 19.0f }, "FLAT", Palette::energeticAmber, false);
        drawMiniButton(g, { bar.getRight() - rstW, bar.getY(), rstW, 19.0f }, "RESET", Palette::deepMagenta, false);
    }

    void drawShaperBoxPresetStrip(juce::Graphics& g, juce::Rectangle<float> bar)
    {
        g.setColour(Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("SHAPER PRESETS:", bar.withWidth(86.0f), juce::Justification::centredLeft, false);

        static const char* presetNames[] = {
            "SIDECHAIN", "TRANCE GATE", "STUTTER", "SAW RAMP",
            "TRIPLET", "SINE WAVE", "PLUCK", "SWELL"
        };
        const auto btnsArea = bar.withTrimmedLeft(88.0f);
        const float btnW = btnsArea.getWidth() / 8.0f;

        for (int i = 0; i < 8; ++i)
        {
            juce::Rectangle<float> r(btnsArea.getX() + static_cast<float>(i) * btnW, bar.getY(), btnW - 3.0f, 16.0f);
            const juce::Colour col = (i < 5) ? Palette::electricCyan : Palette::energeticAmber;
            drawMiniButton(g, r, presetNames[i], col, activeShaperPreset == i);
        }
    }

    void handleTopToolbarClick(juce::Point<float> pos)
    {
        const auto bar = getLocalBounds().toFloat().withHeight(21.0f);
        auto& curve = synthEngine.getArticulator(currentTarget);

        // 1. Target tabs
        const float tabW = 44.0f;
        float x = bar.getX();
        for (int i = 0; i < 6; ++i)
        {
            juce::Rectangle<float> r(x, bar.getY(), tabW - 2.0f, 19.0f);
            if (r.contains(pos))
            {
                setTarget(static_cast<PrismorDSP::ArticulatorTarget>(i));
                return;
            }
            x += tabW;
        }

        // 2. Shape Modes (EXP, S-CRV, LINE) + Tension Nudge (T-, T+)
        x += 4.0f;
        for (int m = 0; m < 3; ++m)
        {
            juce::Rectangle<float> r(x, bar.getY(), 29.0f, 19.0f);
            if (r.contains(pos))
            {
                curve.setAllSegmentsShapeMode(m);
                repaint();
                return;
            }
            x += 31.0f;
        }
        if (juce::Rectangle<float>(x, bar.getY(), 20.0f, 19.0f).contains(pos))
        {
            curve.nudgeAllSegmentsTension(-0.18f);
            repaint();
            return;
        }
        x += 22.0f;
        if (juce::Rectangle<float>(x, bar.getY(), 20.0f, 19.0f).contains(pos))
        {
            curve.nudgeAllSegmentsTension(0.18f);
            repaint();
            return;
        }
        x += 24.0f;

        // 3. SYNC/FREE toggle
        if (juce::Rectangle<float>(x, bar.getY(), 32.0f, 19.0f).contains(pos))
        {
            curve.setTempoSync(!curve.isTempoSync());
            synthEngine.notifyArticulatorRateEdited(currentTarget);
            repaint();
            return;
        }
        x += 34.0f;

        // Rate '<'
        if (juce::Rectangle<float>(x, bar.getY(), 15.0f, 19.0f).contains(pos))
        {
            if (curve.isTempoSync())
                curve.setSyncDivision(curve.getSyncDivision() - 1);
            else
                curve.setRate(curve.getRate() * 0.8f);
            if (curve.getMode() == PrismorDSP::ArticulatorMode::Envelope && currentTarget != PrismorDSP::ArticulatorTarget::CustomFilterShape)
                curve.setMode(PrismorDSP::ArticulatorMode::Loop);
            synthEngine.notifyArticulatorRateEdited(currentTarget);
            repaint();
            return;
        }
        x += 17.0f;

        // Rate Value Box (drag vertically to scrub rate/division)
        if (juce::Rectangle<float>(x, bar.getY(), 58.0f, 19.0f).contains(pos))
        {
            draggingRateBox = true;
            rateDragStartY = pos.y;
            rateDragStartDiv = curve.getSyncDivision();
            rateDragStartHz = curve.getRate();
            return;
        }
        x += 60.0f;

        // Rate '>'
        if (juce::Rectangle<float>(x, bar.getY(), 15.0f, 19.0f).contains(pos))
        {
            if (curve.isTempoSync())
                curve.setSyncDivision(curve.getSyncDivision() + 1);
            else
                curve.setRate(curve.getRate() * 1.25f);
            if (curve.getMode() == PrismorDSP::ArticulatorMode::Envelope && currentTarget != PrismorDSP::ArticulatorTarget::CustomFilterShape)
                curve.setMode(PrismorDSP::ArticulatorMode::Loop);
            synthEngine.notifyArticulatorRateEdited(currentTarget);
            repaint();
            return;
        }
        x += 17.0f;

        // LOOP toggle
        if (juce::Rectangle<float>(x, bar.getY(), 32.0f, 19.0f).contains(pos))
        {
            const auto nextMode = (curve.getMode() == PrismorDSP::ArticulatorMode::Envelope)
                ? PrismorDSP::ArticulatorMode::Loop
                : PrismorDSP::ArticulatorMode::Envelope;
            curve.setMode(nextMode);
            repaint();
            return;
        }

        // 4. FLAT & RESET buttons
        const float rstW = 36.0f;
        if (juce::Rectangle<float>(bar.getRight() - rstW * 2.0f - 2.0f, bar.getY(), rstW, 19.0f).contains(pos))
        {
            curve.clearToFlat(currentTarget);
            activeShaperPreset = -1;
            repaint();
            return;
        }
        if (juce::Rectangle<float>(bar.getRight() - rstW, bar.getY(), rstW, 19.0f).contains(pos))
        {
            curve.resetToDefault(currentTarget);
            activeShaperPreset = -1;
            repaint();
            return;
        }
    }

    void handlePresetStripClick(juce::Point<float> pos)
    {
        const auto bar = getLocalBounds().toFloat().withTop(23.0f).withHeight(18.0f);
        const auto btnsArea = bar.withTrimmedLeft(88.0f);
        if (!btnsArea.contains(pos))
            return;

        const float btnW = btnsArea.getWidth() / 8.0f;
        const int idx = juce::jlimit(0, 7, static_cast<int>((pos.x - btnsArea.getX()) / btnW));
        activeShaperPreset = idx;
        synthEngine.getArticulator(currentTarget).applyPresetShape(idx);
        repaint();
    }

    PrismorDSP::SpectralSynthEngine& synthEngine;
    PrismorDSP::ArticulatorTarget currentTarget { PrismorDSP::ArticulatorTarget::Volume };
    int selectedNodeIndex { -1 };
    int draggingTensionSegment { -1 };
    int activeShaperPreset { -1 };
    float tensionDragStartY { 0.0f };
    float tensionDragStartVal { 0.0f };

    bool draggingRateBox { false };
    float rateDragStartY { 0.0f };
    int rateDragStartDiv { 2 };
    float rateDragStartHz { 1.0f };
    juce::Rectangle<float> rateValueBoxRect {};

    float liveCursor { 0.0f };
    bool voiceActive { false };
};

// -----------------------------------------------------------------------------
// Serum-2 / u-he Hive-2 Inspired 16-Step Arpeggiator & Step Sequencer Grid
// -----------------------------------------------------------------------------
class ArpeggiatorStepGrid : public juce::Component
{
public:
    explicit ArpeggiatorStepGrid(PrismorDSP::SpectralSynthEngine& engineRef)
        : synthEngine(engineRef)
    {
        setOpaque(false);
    }

    void setPlaybackStep(int stepIdx, bool isVoiceActive)
    {
        if (liveStep != stepIdx || voiceActive != isVoiceActive)
        {
            liveStep = stepIdx;
            voiceActive = isVoiceActive;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Top Pattern Preset Bar (18px)
        const auto topBar = bounds.withHeight(18.0f);
        g.setColour(Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("ARP PATTERNS:", topBar.withWidth(76.0f), juce::Justification::centredLeft, false);

        static const char* patLabels[] = { "DEFAULT", "CYBER ARP", "OCT BASS", "ACID 303", "TRANCE GATE" };
        const float patW = 64.0f;
        float px = topBar.getX() + 78.0f;
        for (int i = 0; i < 5; ++i)
        {
            juce::Rectangle<float> br(px, topBar.getY(), patW - 3.0f, 16.0f);
            const juce::Colour col = (i == 0) ? Palette::energeticAmber : Palette::electricCyan;
            g.setColour(Palette::trackDark);
            g.fillRoundedRectangle(br, 2.5f);
            g.setColour(col.withAlpha(0.75f));
            g.drawRoundedRectangle(br, 2.5f, 1.0f);
            g.setColour(Palette::textPrimary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(patLabels[i], br, juce::Justification::centred, false);
            px += patW;
        }

        // Row labels on the left (36px column)
        const auto gridArea = bounds.withTrimmedTop(21.0f);
        const float labelColW = 34.0f;
        const auto stepsRect = gridArea.withTrimmedLeft(labelColW);

        const float gateRowH = 14.0f;
        const float octRowH  = 17.0f;
        const float semiRowH = 17.0f;
        const float velAreaH = std::max(24.0f, stepsRect.getHeight() - gateRowH - octRowH - semiRowH - 6.0f);

        const float yGate = stepsRect.getY();
        const float yVel  = yGate + gateRowH + 2.0f;
        const float yOct  = yVel + velAreaH + 2.0f;
        const float ySemi = yOct + octRowH + 2.0f;

        g.setColour(Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.5f, juce::Font::bold));
        g.drawText("STEP", juce::Rectangle<float>(gridArea.getX(), yGate, labelColW - 2.0f, gateRowH), juce::Justification::centredLeft, false);
        g.drawText("VEL",  juce::Rectangle<float>(gridArea.getX(), yVel,  labelColW - 2.0f, velAreaH), juce::Justification::centredLeft, false);
        g.drawText("OCT",  juce::Rectangle<float>(gridArea.getX(), yOct,  labelColW - 2.0f, octRowH),  juce::Justification::centredLeft, false);
        g.drawText("SEMI", juce::Rectangle<float>(gridArea.getX(), ySemi, labelColW - 2.0f, semiRowH), juce::Justification::centredLeft, false);

        constexpr int numSteps = PrismorDSP::SpectralSynthEngine::NUM_ARP_STEPS;
        const float colW = stepsRect.getWidth() / static_cast<float>(numSteps);

        for (int i = 0; i < numSteps; ++i)
        {
            const auto step = synthEngine.getArpStep(i);
            const float x = stepsRect.getX() + static_cast<float>(i) * colW;
            const float w = colW - 2.5f;
            const bool isPlaying = (voiceActive && liveStep == i);
            const bool isBeatStart = (i % 4 == 0);

            const juce::Colour stepAccent = isPlaying
                ? Palette::energeticAmber
                : (isBeatStart ? Palette::electricCyan : Palette::deepMagenta);

            // 1. Step Enable / Number Pill
            juce::Rectangle<float> gateRect(x, yGate, w, gateRowH);
            g.setColour(step.enabled ? stepAccent.withAlpha(isPlaying ? 0.45f : 0.24f) : Palette::trackDark);
            g.fillRoundedRectangle(gateRect, 2.0f);
            g.setColour(isPlaying ? Palette::textPrimary : (step.enabled ? stepAccent : Palette::borderSubtle));
            g.drawRoundedRectangle(gateRect, 2.0f, isPlaying ? 1.4f : 1.0f);
            g.setColour(step.enabled ? Palette::textPrimary : Palette::textDim);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.6f, juce::Font::bold));
            g.drawText(juce::String(i + 1), gateRect, juce::Justification::centred, false);

            // 2. Velocity Bar
            juce::Rectangle<float> velSlot(x, yVel, w, velAreaH);
            g.setColour(Palette::trackDark);
            g.fillRoundedRectangle(velSlot, 2.5f);
            g.setColour(isPlaying ? Palette::energeticAmber : Palette::borderSubtle.withAlpha(0.75f));
            g.drawRoundedRectangle(velSlot, 2.5f, 1.0f);

            if (step.enabled)
            {
                const float barH = juce::jlimit(2.0f, velAreaH - 4.0f, step.velocity * (velAreaH - 4.0f));
                juce::Rectangle<float> fillR(x + 1.5f, velSlot.getBottom() - 2.0f - barH, w - 3.0f, barH);
                g.setColour(stepAccent.withAlpha(0.78f));
                g.fillRoundedRectangle(fillR, 1.5f);
                g.setColour(Palette::textPrimary);
                g.fillRect(fillR.getX(), fillR.getY(), fillR.getWidth(), 1.5f);
            }

            // 3. Octave Offset Trigger Box (-1 .. +2)
            juce::Rectangle<float> octRect(x, yOct, w, octRowH);
            const bool hasOct = (step.octaveOffset != 0);
            g.setColour(hasOct ? Palette::electricCyan.withAlpha(0.24f) : Palette::trackDark);
            g.fillRoundedRectangle(octRect, 2.0f);
            g.setColour(hasOct ? Palette::electricCyan : Palette::borderSubtle);
            g.drawRoundedRectangle(octRect, 2.0f, 1.0f);
            g.setColour(hasOct ? Palette::textPrimary : Palette::textSecondary);
            const juce::String octStr = (step.octaveOffset > 0 ? "+" : "") + juce::String(step.octaveOffset);
            g.drawText(octStr, octRect, juce::Justification::centred, false);

            // 4. Semitone Transpose Box (-12 .. +12)
            juce::Rectangle<float> semiRect(x, ySemi, w, semiRowH);
            const bool hasSemi = (step.semitoneTranspose != 0);
            g.setColour(hasSemi ? Palette::deepMagenta.withAlpha(0.24f) : Palette::trackDark);
            g.fillRoundedRectangle(semiRect, 2.0f);
            g.setColour(hasSemi ? Palette::deepMagenta : Palette::borderSubtle);
            g.drawRoundedRectangle(semiRect, 2.0f, 1.0f);
            g.setColour(hasSemi ? Palette::textPrimary : Palette::textSecondary);
            const juce::String semiStr = (step.semitoneTranspose > 0 ? "+" : "") + juce::String(step.semitoneTranspose);
            g.drawText(semiStr, semiRect, juce::Justification::centred, false);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto pos = e.position;
        const auto bounds = getLocalBounds().toFloat();

        if (pos.y <= 19.0f)
        {
            const float startX = bounds.getX() + 78.0f;
            const float patW = 64.0f;
            if (pos.x >= startX && pos.x < startX + patW * 5.0f)
            {
                const int patIdx = juce::jlimit(0, 4, static_cast<int>((pos.x - startX) / patW));
                synthEngine.resetArpStepsToDefault(patIdx);
                repaint();
            }
            return;
        }

        handleGridMouse(e, true);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mouseDownPosition.y > 19.0f)
            handleGridMouse(e, false);
    }

private:
    void handleGridMouse(const juce::MouseEvent& e, bool isClick)
    {
        const auto bounds = getLocalBounds().toFloat();
        const auto gridArea = bounds.withTrimmedTop(21.0f);
        const auto stepsRect = gridArea.withTrimmedLeft(34.0f);
        if (e.position.x < stepsRect.getX())
            return;

        constexpr int numSteps = PrismorDSP::SpectralSynthEngine::NUM_ARP_STEPS;
        const int stepIdx = juce::jlimit(0, numSteps - 1,
            static_cast<int>(((e.position.x - stepsRect.getX()) / stepsRect.getWidth()) * static_cast<float>(numSteps)));

        const float gateRowH = 14.0f;
        const float octRowH  = 17.0f;
        const float semiRowH = 17.0f;
        const float velAreaH = std::max(24.0f, stepsRect.getHeight() - gateRowH - octRowH - semiRowH - 6.0f);

        const float yGate = stepsRect.getY();
        const float yVel  = yGate + gateRowH + 2.0f;
        const float yOct  = yVel + velAreaH + 2.0f;
        const float ySemi = yOct + octRowH + 2.0f;

        auto step = synthEngine.getArpStep(stepIdx);

        if (isClick && e.position.y >= yGate && e.position.y <= yGate + gateRowH)
        {
            step.enabled = !step.enabled;
            synthEngine.setArpStep(stepIdx, step);
            repaint();
            return;
        }

        if (isClick && e.position.y >= yOct && e.position.y <= yOct + octRowH)
        {
            if (e.mods.isRightButtonDown())
                step.octaveOffset = 0;
            else
            {
                step.octaveOffset += 1;
                if (step.octaveOffset > 2)
                    step.octaveOffset = -1;
            }
            synthEngine.setArpStep(stepIdx, step);
            repaint();
            return;
        }

        if (e.position.y >= ySemi && e.position.y <= ySemi + semiRowH + 4.0f)
        {
            if (isClick)
            {
                draggingSemiStep = stepIdx;
                semiDragStartY = e.position.y;
                semiDragStartVal = step.semitoneTranspose;
                if (e.mods.isRightButtonDown())
                    step.semitoneTranspose = 0;
                else
                    step.semitoneTranspose = (step.semitoneTranspose >= 12) ? -12 : (step.semitoneTranspose + 1);
                synthEngine.setArpStep(stepIdx, step);
                repaint();
            }
            return;
        }

        if (!isClick && draggingSemiStep >= 0 && e.mouseDownPosition.y >= ySemi)
        {
            auto sDrag = synthEngine.getArpStep(draggingSemiStep);
            const int delta = static_cast<int>((semiDragStartY - e.position.y) / 6.0f);
            sDrag.semitoneTranspose = juce::jlimit(-12, 12, semiDragStartVal + delta);
            synthEngine.setArpStep(draggingSemiStep, sDrag);
            repaint();
            return;
        }

        // Velocity Bar Drag
        if (e.mouseDownPosition.y >= yVel && e.mouseDownPosition.y < yOct)
        {
            draggingSemiStep = -1;
            const float normVel = juce::jlimit(0.05f, 1.0f, (yVel + velAreaH - e.position.y) / velAreaH);
            step.velocity = normVel;
            step.enabled = true;
            synthEngine.setArpStep(stepIdx, step);
            repaint();
        }
    }

    PrismorDSP::SpectralSynthEngine& synthEngine;
    int liveStep { 0 };
    bool voiceActive { false };
    int draggingSemiStep { -1 };
    float semiDragStartY { 0.0f };
    int semiDragStartVal { 0 };
};

// -----------------------------------------------------------------------------
// Gross Beat / ShaperBox 3 Interactive Time-Modulation Buffer & Curve Editor
// -----------------------------------------------------------------------------
class TimeModCurveEditor : public juce::Component
{
public:
    explicit TimeModCurveEditor(PrismorDSP::SpectralSynthEngine& engineRef)
        : synthEngine(engineRef)
    {
        setOpaque(false);
    }

    std::function<void(int)> onTimePresetSelected;

    void pushSnapshot(const PrismorDSP::VisualizerSnapshot& snap)
    {
        latestSnap = snap;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const auto patternBar = bounds.withHeight(28.0f);
        const auto canvas = getCanvasBounds();

        // 1. Top Gross Beat / ShaperBox 3 Pattern Slot Bar
        static const char* slotNames[8] = {
            "1. HALF-TIME 1/2x", "2. 1/8 STUTTER", "3. TAPE STOP", "4. REVERSE BAR",
            "5. DJ SCRATCH", "6. TRIPLET GLITCH", "7. VINYL DROP", "8. CHAOS REPEAT"
        };
        const float slotW = patternBar.getWidth() / 8.0f;
        for (int i = 0; i < 8; ++i)
        {
            juce::Rectangle<float> r(patternBar.getX() + static_cast<float>(i) * slotW, patternBar.getY(), slotW - 4.0f, 24.0f);
            const bool isSel = (activePattern == i);
            const juce::Colour col = (i < 4) ? Palette::electricCyan : Palette::energeticAmber;
            g.setColour(isSel ? col.withAlpha(0.30f) : Palette::trackDark);
            g.fillRoundedRectangle(r, 3.5f);
            g.setColour(isSel ? col : Palette::borderSubtle);
            g.drawRoundedRectangle(r, 3.5f, isSel ? 1.5f : 1.0f);
            g.setColour(isSel ? Palette::textPrimary : Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.2f, juce::Font::bold));
            g.drawText(slotNames[i], r, juce::Justification::centred, false);
        }

        // 2. Time-Warp Buffer & Spline Canvas
        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(canvas, 5.0f);

        // 16-Step Beat Grid + Beat Delay Axis Labels
        for (int c = 1; c < 16; ++c)
        {
            const float gx = canvas.getX() + canvas.getWidth() * (static_cast<float>(c) / 16.0f);
            const bool isBeat = (c % 4 == 0);
            g.setColour(Palette::borderSubtle.withAlpha(isBeat ? 0.65f : 0.28f));
            g.drawVerticalLine(static_cast<int>(gx), canvas.getY(), canvas.getBottom());
        }

        static const char* beatOffsetLabels[5] = {
            "0.0 BEATS [REAL-TIME]", "-0.5 BEATS", "-1.0 BEAT DELAY", "-1.5 BEATS", "-2.0 BEATS MAX"
        };
        for (int r = 0; r <= 4; ++r)
        {
            const float gy = canvas.getY() + 10.0f + (canvas.getHeight() - 20.0f) * (static_cast<float>(r) / 4.0f);
            if (r > 0 && r < 4)
            {
                g.setColour(Palette::borderSubtle.withAlpha(0.45f));
                g.drawHorizontalLine(static_cast<int>(gy), canvas.getX(), canvas.getRight());
            }
            g.setColour(Palette::textDim);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(beatOffsetLabels[r], static_cast<int>(canvas.getX() + 8.0f), static_cast<int>(gy - 6.0f), 130, 12,
                       juce::Justification::centredLeft, false);
        }

        // 3. Live Captured Circular Audio Buffer Waveform Overlay
        {
            juce::Path waveP;
            const float midY = canvas.getCentreY();
            const float ampScale = canvas.getHeight() * 0.36f;
            for (int i = 0; i < 64; ++i)
            {
                const float nx = static_cast<float>(i) / 63.0f;
                const float px = canvas.getX() + nx * canvas.getWidth();
                float sampleVal = latestSnap.timeModWavePreview[static_cast<size_t>(i)];
                if (!latestSnap.voiceActive && std::abs(sampleVal) < 0.01f)
                {
                    sampleVal = 0.25f * std::sin(nx * 25.13f + latestSnap.timeModCursor * 6.28f)
                              * std::cos(nx * 6.28f);
                }
                const float py = midY - sampleVal * ampScale;
                if (i == 0) waveP.startNewSubPath(px, py);
                else        waveP.lineTo(px, py);
            }
            g.setColour(Palette::electricCyan.withAlpha(0.22f));
            g.strokePath(waveP, juce::PathStrokeType(1.6f));
        }

        // 4. Time-Warp Spline Curve (Y=0 at top = 0 delay, Y=1 at bottom = max delay)
        const auto& curve = synthEngine.getTimeModCurve();
        const auto nodes = curve.getNodesCopy();

        juce::Path curvePath, fillPath;
        constexpr int numSteps = 192;
        for (int i = 0; i <= numSteps; ++i)
        {
            const float normX = static_cast<float>(i) / static_cast<float>(numSteps);
            const float delayNorm = curve.evaluateLUT(normX);
            const float px = canvas.getX() + normX * canvas.getWidth();
            const float py = canvas.getY() + 8.0f + delayNorm * (canvas.getHeight() - 16.0f);

            if (i == 0)
            {
                curvePath.startNewSubPath(px, py);
                fillPath.startNewSubPath(px, canvas.getY() + 8.0f);
                fillPath.lineTo(px, py);
            }
            else
            {
                curvePath.lineTo(px, py);
                fillPath.lineTo(px, py);
            }
        }
        fillPath.lineTo(canvas.getRight(), canvas.getY() + 8.0f);
        fillPath.closeSubPath();

        juce::ColourGradient grad(Palette::energeticAmber.withAlpha(0.24f), 0.0f, canvas.getY(),
                                  Palette::deepMagenta.withAlpha(0.06f), 0.0f, canvas.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(fillPath);

        g.setColour(Palette::energeticAmber.withAlpha(0.32f));
        g.strokePath(curvePath, juce::PathStrokeType(4.5f));
        g.setColour(Palette::energeticAmber);
        g.strokePath(curvePath, juce::PathStrokeType(2.2f));

        // 5. Live Write-Head & Warped Read-Head Playhead Cursors
        {
            const float curX = canvas.getX() + juce::jlimit(0.0f, 1.0f, latestSnap.timeModCursor) * canvas.getWidth();
            const float curDelay = curve.evaluateLUT(latestSnap.timeModCursor);
            const float curY = canvas.getY() + 8.0f + curDelay * (canvas.getHeight() - 16.0f);

            g.setColour(Palette::electricCyan.withAlpha(0.75f));
            g.drawVerticalLine(static_cast<int>(curX), canvas.getY(), canvas.getBottom());

            g.setColour(Palette::energeticAmber);
            g.fillEllipse(curX - 5.0f, curY - 5.0f, 10.0f, 10.0f);
            g.setColour(Palette::textPrimary);
            g.drawEllipse(curX - 5.0f, curY - 5.0f, 10.0f, 10.0f, 1.5f);
        }

        // 6. Segment Tension Diamonds & Spline Control Nodes
        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const float midX = 0.5f * (nodes[i].x + nodes[i + 1].x);
            const float midY = curve.evaluateNodesDirect(midX);
            const float tx = canvas.getX() + midX * canvas.getWidth();
            const float ty = canvas.getY() + 8.0f + midY * (canvas.getHeight() - 16.0f);

            juce::Path d;
            d.startNewSubPath(tx, ty - 4.5f);
            d.lineTo(tx + 4.5f, ty);
            d.lineTo(tx, ty + 4.5f);
            d.lineTo(tx - 4.5f, ty);
            d.closeSubPath();
            g.setColour(Palette::bgObsidian);
            g.fillPath(d);
            g.setColour(Palette::electricCyan);
            g.strokePath(d, juce::PathStrokeType(1.5f));
        }

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const float nx = canvas.getX() + nodes[i].x * canvas.getWidth();
            const float ny = canvas.getY() + 8.0f + nodes[i].y * (canvas.getHeight() - 16.0f);
            const bool isSel = (static_cast<int>(i) == selectedNode);
            const float r = isSel ? 5.8f : 4.6f;

            g.setColour(Palette::bgObsidian);
            g.fillEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f);
            g.setColour(isSel ? Palette::textPrimary : Palette::energeticAmber);
            g.drawEllipse(nx - r, ny - r, r * 2.0f, r * 2.0f, 1.8f);
        }

        g.setColour(Palette::textDim);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.drawText("DIAGONAL SLOPE = HALF-SPEED  |  FLAT HORIZONTAL = STUTTER HOLD  |  UPWARD SLOPE = REVERSE  |  DBL-CLICK: ADD/DEL NODE",
                   canvas.reduced(8.0f, 3.0f), juce::Justification::bottomRight, false);

        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(canvas, 5.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto pos = e.position;
        if (pos.y <= 28.0f)
        {
            const float slotW = getLocalBounds().toFloat().getWidth() / 8.0f;
            const int idx = juce::jlimit(0, 7, static_cast<int>(pos.x / slotW));
            activePattern = idx;
            synthEngine.getTimeModCurve().applyTimeModPreset(idx);
            if (onTimePresetSelected)
                onTimePresetSelected(idx);
            repaint();
            return;
        }

        const auto canvas = getCanvasBounds();
        if (!canvas.contains(pos))
            return;

        auto& curve = synthEngine.getTimeModCurve();
        const auto nodes = curve.getNodesCopy();

        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            const float midX = 0.5f * (nodes[i].x + nodes[i + 1].x);
            const float midY = curve.evaluateNodesDirect(midX);
            const float tx = canvas.getX() + midX * canvas.getWidth();
            const float ty = canvas.getY() + 8.0f + midY * (canvas.getHeight() - 16.0f);
            if (pos.getDistanceFrom({ tx, ty }) <= 11.0f)
            {
                if (e.mods.isRightButtonDown())
                {
                    curve.cycleSegmentShapeMode(static_cast<int>(i));
                    repaint();
                    return;
                }
                draggingSegment = static_cast<int>(i);
                dragStartY = pos.y;
                dragStartTension = nodes[i].tension;
                if (nodes[i].shapeMode == 2)
                    curve.cycleSegmentShapeMode(static_cast<int>(i));
                selectedNode = -1;
                repaint();
                return;
            }
        }

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const float nx = canvas.getX() + nodes[i].x * canvas.getWidth();
            const float ny = canvas.getY() + 8.0f + nodes[i].y * (canvas.getHeight() - 16.0f);
            if (pos.getDistanceFrom({ nx, ny }) <= 10.0f)
            {
                if (e.mods.isRightButtonDown())
                {
                    curve.removeNode(static_cast<int>(i));
                    selectedNode = -1;
                    repaint();
                    return;
                }
                selectedNode = static_cast<int>(i);
                draggingSegment = -1;
                repaint();
                return;
            }
        }

        if (e.mods.isRightButtonDown())
        {
            const float normX = juce::jlimit(0.0f, 1.0f, (pos.x - canvas.getX()) / canvas.getWidth());
            const float normY = juce::jlimit(0.0f, 1.0f, (pos.y - (canvas.getY() + 8.0f)) / (canvas.getHeight() - 16.0f));
            selectedNode = curve.addNode(normX, normY);
            repaint();
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        const auto pos = e.position;
        const auto canvas = getCanvasBounds();
        if (!canvas.contains(pos))
            return;

        auto& curve = synthEngine.getTimeModCurve();
        const auto nodes = curve.getNodesCopy();
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const float nx = canvas.getX() + nodes[i].x * canvas.getWidth();
            const float ny = canvas.getY() + 8.0f + nodes[i].y * (canvas.getHeight() - 16.0f);
            if (pos.getDistanceFrom({ nx, ny }) <= 10.0f)
            {
                curve.removeNode(static_cast<int>(i));
                selectedNode = -1;
                repaint();
                return;
            }
        }

        const float normX = juce::jlimit(0.0f, 1.0f, (pos.x - canvas.getX()) / canvas.getWidth());
        const float normY = juce::jlimit(0.0f, 1.0f, (pos.y - (canvas.getY() + 8.0f)) / (canvas.getHeight() - 16.0f));
        selectedNode = curve.addNode(normX, normY);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        auto& curve = synthEngine.getTimeModCurve();
        const auto canvas = getCanvasBounds();

        if (selectedNode >= 0)
        {
            const float normX = juce::jlimit(0.0f, 1.0f, (e.position.x - canvas.getX()) / canvas.getWidth());
            const float normY = juce::jlimit(0.0f, 1.0f, (e.position.y - (canvas.getY() + 8.0f)) / (canvas.getHeight() - 16.0f));
            curve.updateNode(selectedNode, normX, normY);
            repaint();
        }
        else if (draggingSegment >= 0)
        {
            const float dy = (e.position.y - dragStartY) / 100.0f;
            curve.updateTension(draggingSegment, dragStartTension + dy);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingSegment = -1;
    }

private:
    juce::Rectangle<float> getCanvasBounds() const noexcept
    {
        return getLocalBounds().toFloat().withTrimmedTop(30.0f);
    }

    PrismorDSP::SpectralSynthEngine& synthEngine;
    PrismorDSP::VisualizerSnapshot latestSnap {};
    int activePattern { 0 };
    int selectedNode { -1 };
    int draggingSegment { -1 };
    float dragStartY { 0.0f };
    float dragStartTension { 0.0f };
};

// -----------------------------------------------------------------------------
// Serum 2 Style Save / Author User Preset Modal Dialog Overlay
// -----------------------------------------------------------------------------
class SavePresetOverlay : public juce::Component
{
public:
    SavePresetOverlay()
    {
        setOpaque(false);
        setVisible(false);

        addAndMakeVisible(nameEditor);
        nameEditor.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold));
        nameEditor.setColour(juce::TextEditor::backgroundColourId, Palette::bgObsidian);
        nameEditor.setColour(juce::TextEditor::textColourId, Palette::textPrimary);
        nameEditor.setColour(juce::TextEditor::outlineColourId, Palette::borderSubtle);
        nameEditor.setColour(juce::TextEditor::focusedOutlineColourId, Palette::energeticAmber);

        addAndMakeVisible(catBox);
        catBox.addItemList({ "BASS", "LEADS", "PADS", "PLUCKS", "ARP & SEQ", "FX & TRANS", "USER" }, 1);
        catBox.setSelectedId(1, juce::dontSendNotification);

        addAndMakeVisible(authorEditor);
        authorEditor.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain));
        authorEditor.setColour(juce::TextEditor::backgroundColourId, Palette::bgObsidian);
        authorEditor.setColour(juce::TextEditor::textColourId, Palette::textPrimary);
        authorEditor.setColour(juce::TextEditor::outlineColourId, Palette::borderSubtle);

        addAndMakeVisible(descEditor);
        descEditor.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain));
        descEditor.setColour(juce::TextEditor::backgroundColourId, Palette::bgObsidian);
        descEditor.setColour(juce::TextEditor::textColourId, Palette::textPrimary);
        descEditor.setColour(juce::TextEditor::outlineColourId, Palette::borderSubtle);

        addAndMakeVisible(saveBtn);
        saveBtn.setColour(juce::TextButton::buttonColourId, Palette::energeticAmber.withAlpha(0.35f));
        saveBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        saveBtn.setColour(juce::TextButton::textColourOffId, Palette::energeticAmber);
        saveBtn.onClick = [this]()
        {
            if (onSave)
            {
                juce::String catName = catBox.getText();
                if (catName.isEmpty()) catName = "USER";
                onSave(nameEditor.getText().trim(), catName, authorEditor.getText().trim(), descEditor.getText().trim());
            }
        };

        addAndMakeVisible(cancelBtn);
        cancelBtn.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
        cancelBtn.onClick = [this]()
        {
            setVisible(false);
        };
    }

    std::function<void(const juce::String&, const juce::String&, const juce::String&, const juce::String&)> onSave;

    void show(const juce::String& defaultName, const juce::String& category, const juce::String& author, const juce::String& description)
    {
        nameEditor.setText(defaultName);
        for (int i = 0; i < catBox.getNumItems(); ++i)
        {
            if (catBox.getItemText(i).equalsIgnoreCase(category))
            {
                catBox.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
        authorEditor.setText(author.isNotEmpty() ? author : "Studio Producer");
        descEditor.setText(description.isNotEmpty() ? description : "Custom dual spectral synthesis patch");
        setVisible(true);
        toFront(true);
        nameEditor.grabKeyboardFocus();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.70f));

        const auto b = getLocalBounds().toFloat();
        const float cardW = 440.0f;
        const float cardH = 290.0f;
        juce::Rectangle<float> card((b.getWidth() - cardW) * 0.5f, (b.getHeight() - cardH) * 0.5f, cardW, cardH);

        g.setColour(Palette::cardSurface);
        g.fillRoundedRectangle(card, 6.0f);
        g.setColour(Palette::cardHeader);
        g.fillRoundedRectangle(card.withHeight(32.0f), 6.0f);
        g.fillRect(card.getX(), card.getY() + 24.0f, card.getWidth(), 8.0f);

        g.setColour(Palette::energeticAmber);
        g.fillRoundedRectangle(card.getX() + 8.0f, card.getY() + 8.0f, 4.0f, 16.0f, 2.0f);
        g.drawRoundedRectangle(card.reduced(0.5f), 6.0f, 1.5f);

        g.setColour(Palette::textPrimary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.5f, juce::Font::bold));
        g.drawText("SAVE USER PRESET (SERUM 2 STYLE)", static_cast<int>(card.getX() + 18.0f), static_cast<int>(card.getY() + 4.0f), static_cast<int>(card.getWidth() - 30.0f), 24, juce::Justification::centredLeft);

        auto drawFieldLabel = [&](const char* label, float y)
        {
            g.setColour(Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.2f, juce::Font::bold));
            g.drawText(label, static_cast<int>(card.getX() + 16.0f), static_cast<int>(y), 160, 14, juce::Justification::centredLeft);
        };

        drawFieldLabel("PRESET NAME:", card.getY() + 38.0f);
        drawFieldLabel("CATEGORY:", card.getY() + 86.0f);
        drawFieldLabel("AUTHOR:", card.getY() + 134.0f);
        drawFieldLabel("DESCRIPTION / NOTES:", card.getY() + 182.0f);
    }

    void resized() override
    {
        const auto b = getLocalBounds();
        const int cardW = 440;
        const int cardH = 290;
        const int cardX = (b.getWidth() - cardW) / 2;
        const int cardY = (b.getHeight() - cardH) / 2;

        nameEditor.setBounds(cardX + 16, cardY + 54, cardW - 32, 24);
        catBox.setBounds(cardX + 16, cardY + 102, cardW - 32, 24);
        authorEditor.setBounds(cardX + 16, cardY + 150, cardW - 32, 24);
        descEditor.setBounds(cardX + 16, cardY + 198, cardW - 32, 24);

        saveBtn.setBounds(cardX + cardW - 170, cardY + cardH - 38, 154, 26);
        cancelBtn.setBounds(cardX + cardW - 264, cardY + cardH - 38, 86, 26);
    }

private:
    juce::TextEditor nameEditor;
    juce::ComboBox   catBox;
    juce::TextEditor authorEditor;
    juce::TextEditor descEditor;
    juce::TextButton saveBtn   { "SAVE PRESET TO DISK" };
    juce::TextButton cancelBtn { "CANCEL" };
};

// -----------------------------------------------------------------------------
// Serum-2 / Zenology Style Full-Tab Categorized Preset Browser & Patch Inspector
// -----------------------------------------------------------------------------
class PresetBrowserView : public juce::Component
{
public:
    explicit PresetBrowserView(PrismorAudioProcessor& proc)
        : processor(proc)
    {
        setOpaque(false);
        favorites.resize(1024, false);
        favorites[3] = favorites[6] = favorites[23] = favorites[43] = favorites[65] = favorites[83] = favorites[103] = true;

        // Search Bar Setup
        searchBox.setTextToShowWhenEmpty("SEARCH PRESETS (NAME, CATEGORY, OSC, FX, TAGS)...", Palette::textDim);
        searchBox.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::bold));
        searchBox.setColour(juce::TextEditor::backgroundColourId, Palette::bgObsidian);
        searchBox.setColour(juce::TextEditor::textColourId, Palette::textPrimary);
        searchBox.setColour(juce::TextEditor::outlineColourId, Palette::borderSubtle);
        searchBox.setColour(juce::TextEditor::focusedOutlineColourId, Palette::energeticAmber);
        searchBox.onTextChange = [this]()
        {
            scrollOffset = 0;
            refreshFilteredList();
        };
        addAndMakeVisible(searchBox);

        clearSearchBtn.setButtonText("x");
        clearSearchBtn.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
        clearSearchBtn.setColour(juce::TextButton::textColourOffId, Palette::textDim);
        clearSearchBtn.onClick = [this]()
        {
            searchBox.clear();
        };
        addAndMakeVisible(clearSearchBtn);

        // Right Column Action Buttons
        savePresetBtn.setColour(juce::TextButton::buttonColourId, Palette::energeticAmber.withAlpha(0.25f));
        savePresetBtn.setColour(juce::TextButton::textColourOffId, Palette::energeticAmber);
        savePresetBtn.onClick = [this]()
        {
            const int curIdx = processor.getCurrentPresetIndex();
            const auto bank = processor.getFactoryPresetBank();
            juce::String defName = "My Custom Preset";
            juce::String defCat = "USER";
            juce::String defAuthor = "Studio Producer";
            juce::String defDesc = "Custom dual spectral synthesis patch";

            if (curIdx >= 0 && curIdx < static_cast<int>(bank.size()))
            {
                defName = bank[static_cast<size_t>(curIdx)].name.replaceFirstOccurrenceOf(juce::String(curIdx) + ". ", "");
                defName = defName.replaceFirstOccurrenceOf(juce::String(curIdx < 10 ? "0" : "") + juce::String(curIdx) + ". ", "");
                defCat = bank[static_cast<size_t>(curIdx)].category;
            }

            saveOverlay.show(defName, defCat, defAuthor, defDesc);
        };
        addAndMakeVisible(savePresetBtn);

        importPresetBtn.setColour(juce::TextButton::buttonColourId, Palette::electricCyan.withAlpha(0.20f));
        importPresetBtn.setColour(juce::TextButton::textColourOffId, Palette::electricCyan);
        importPresetBtn.onClick = [this]()
        {
            auto fileChooser = std::make_shared<juce::FileChooser>(
                "Import Prismor Preset",
                processor.getUserPresetDirectory(),
                "*.prismor;*.xml;*.json");

            fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, fileChooser](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        processor.importPresetFromFile(file);
                        refreshFilteredList();
                        if (onPresetLoaded)
                            onPresetLoaded(processor.getCurrentPresetIndex());
                    }
                });
        };
        addAndMakeVisible(importPresetBtn);

        exportPresetBtn.setColour(juce::TextButton::buttonColourId, Palette::deepMagenta.withAlpha(0.20f));
        exportPresetBtn.setColour(juce::TextButton::textColourOffId, Palette::deepMagenta.brighter(0.2f));
        exportPresetBtn.onClick = [this]()
        {
            const int curIdx = processor.getCurrentPresetIndex();
            const auto bank = processor.getFactoryPresetBank();
            juce::String defName = "PrismorPreset";
            juce::String defCat = "USER";
            juce::String defAuthor = "Producer";
            juce::String defDesc = "";
            if (curIdx >= 0 && curIdx < static_cast<int>(bank.size()))
            {
                defName = bank[static_cast<size_t>(curIdx)].name;
                defCat = bank[static_cast<size_t>(curIdx)].category;
                defAuthor = bank[static_cast<size_t>(curIdx)].author;
                defDesc = bank[static_cast<size_t>(curIdx)].description;
            }

            auto fileChooser = std::make_shared<juce::FileChooser>(
                "Export Prismor Preset",
                processor.getUserPresetDirectory().getChildFile(defName + ".prismor"),
                "*.prismor");

            fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, fileChooser, defName, defCat, defAuthor, defDesc](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        processor.exportPresetToFile(file, defName, defCat, defAuthor, defDesc);
                        refreshFilteredList();
                    }
                });
        };
        addAndMakeVisible(exportPresetBtn);

        openFolderBtn.setColour(juce::TextButton::buttonColourId, Palette::trackDark);
        openFolderBtn.setColour(juce::TextButton::textColourOffId, Palette::textSecondary);
        openFolderBtn.onClick = [this]()
        {
            processor.getUserPresetDirectory().startAsProcess();
        };
        addAndMakeVisible(openFolderBtn);

        // Modal Save Overlay
        saveOverlay.onSave = [this](const juce::String& name, const juce::String& cat, const juce::String& author, const juce::String& desc)
        {
            processor.saveUserPreset(name, cat, author, desc);
            refreshFilteredList();
            saveOverlay.setVisible(false);
        };
        addChildComponent(saveOverlay);

        refreshFilteredList();
    }

    std::function<void(int)> onPresetLoaded;

    void refreshFilteredList()
    {
        const auto allPresets = processor.getFactoryPresetBank();
        filteredPresets.clear();

        static const char* catNames[9] = {
            "ALL", "INIT", "BASS", "LEADS", "PADS", "PLUCKS", "ARP & SEQ", "FX & TRANS", "USER"
        };
        const juce::String targetCat = catNames[juce::jlimit(0, 8, selectedCategory)];
        const juce::String query = searchBox.getText().trim();

        for (const auto& p : allPresets)
        {
            if (selectedCategory > 0)
            {
                if (selectedCategory == 8) // USER PRESETS
                {
                    if (!p.isUserPreset && p.category != "USER")
                        continue;
                }
                else if (selectedCategory == 7)
                {
                    if (p.category != "FX & TRANS" && p.category != "TIME & LO-FI")
                        continue;
                }
                else if (p.category != targetCat)
                {
                    continue;
                }
            }

            if (selectedTagFilter == 1 && !p.author.containsIgnoreCase("Zenology") && !p.fxUsed.containsIgnoreCase("Ring")
                && !p.fxUsed.containsIgnoreCase("Crush") && !p.fxUsed.containsIgnoreCase("Flange"))
                continue;
            if (selectedTagFilter == 2 && p.category != "FX & TRANS" && p.category != "TIME & LO-FI" && !p.fxUsed.containsIgnoreCase("Time Mod") && !p.fxUsed.containsIgnoreCase("Gross Beat") && !p.fxUsed.containsIgnoreCase("Stutter"))
                continue;
            if (selectedTagFilter == 3 && !p.fxUsed.containsIgnoreCase("Talkbox") && !p.fxUsed.containsIgnoreCase("Vocoder")
                && !p.fxUsed.containsIgnoreCase("Vox"))
                continue;
            if (selectedTagFilter == 4 && (p.index >= static_cast<int>(favorites.size()) || !favorites[static_cast<size_t>(p.index)]))
                continue;

            // Genre Expansion Pack Filter (Phase 5.5)
            if (selectedGenreFilter == 1 && !p.description.containsIgnoreCase("CYBERPUNK & SYNTHWAVE")
                && !p.author.containsIgnoreCase("Cyberpunk") && !p.author.containsIgnoreCase("Synthwave"))
                continue;
            if (selectedGenreFilter == 2 && !p.description.containsIgnoreCase("CINEMATIC & AMBIENT")
                && !p.author.containsIgnoreCase("Cinematic") && !p.author.containsIgnoreCase("Ambient"))
                continue;
            if (selectedGenreFilter == 3 && !p.description.containsIgnoreCase("MODERN TECH HOUSE")
                && !p.author.containsIgnoreCase("Tech House"))
                continue;
            if (selectedGenreFilter == 4 && !p.description.containsIgnoreCase("LO-FI & CHILLHOP")
                && !p.author.containsIgnoreCase("Lo-Fi") && !p.author.containsIgnoreCase("Chillhop"))
                continue;

            // Instant Search Query Matching Across Name, Category, Author, Waves, FX, and Description
            if (query.isNotEmpty())
            {
                const bool matches = p.name.containsIgnoreCase(query) ||
                                     p.category.containsIgnoreCase(query) ||
                                     p.author.containsIgnoreCase(query) ||
                                     p.wavesUsed.containsIgnoreCase(query) ||
                                     p.fxUsed.containsIgnoreCase(query) ||
                                     p.description.containsIgnoreCase(query);
                if (!matches)
                    continue;
            }

            filteredPresets.push_back(p);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float leftW = 210.0f;
        const float rightW = 290.0f;
        const float gap = 8.0f;

        const juce::Rectangle<float> leftCard(bounds.getX(), bounds.getY(), leftW, bounds.getHeight());
        const juce::Rectangle<float> rightCard(bounds.getRight() - rightW, bounds.getY(), rightW, bounds.getHeight());
        const juce::Rectangle<float> centerCard(leftCard.getRight() + gap, bounds.getY(),
                                                bounds.getWidth() - leftW - rightW - gap * 2.0f, bounds.getHeight());

        auto drawCardFrame = [&](juce::Rectangle<float> r, const char* title, juce::Colour accent)
        {
            g.setColour(Palette::cardSurface);
            g.fillRoundedRectangle(r, 5.0f);
            g.setColour(Palette::cardHeader);
            g.fillRoundedRectangle(r.withHeight(24.0f), 5.0f);
            g.fillRect(r.getX(), r.getY() + 16.0f, r.getWidth(), 8.0f);
            g.setColour(accent);
            g.fillRoundedRectangle(r.getX() + 6.0f, r.getY() + 5.0f, 3.0f, 13.0f, 1.5f);
            g.setColour(Palette::textPrimary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.2f, juce::Font::bold));
            g.drawText(title, static_cast<int>(r.getX() + 14.0f), static_cast<int>(r.getY() + 4.0f),
                       static_cast<int>(r.getWidth() - 20.0f), 16, juce::Justification::centredLeft, false);
            g.setColour(Palette::borderSubtle);
            g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
        };

        const auto allPresets = processor.getFactoryPresetBank();
        int catCounts[9] = { 0 };
        for (const auto& p : allPresets)
        {
            catCounts[0]++; // ALL
            if (p.isUserPreset || p.category == "USER") catCounts[8]++;
            else if (p.category == "INIT") catCounts[1]++;
            else if (p.category == "BASS") catCounts[2]++;
            else if (p.category == "LEADS") catCounts[3]++;
            else if (p.category == "PADS") catCounts[4]++;
            else if (p.category == "PLUCKS") catCounts[5]++;
            else if (p.category == "ARP & SEQ") catCounts[6]++;
            else if (p.category == "FX & TRANS" || p.category == "TIME & LO-FI") catCounts[7]++;
        }

        drawCardFrame(leftCard,   "BANK CATEGORIES & TAGS",                 Palette::electricCyan);
        const juce::String centerTitle = "PRESET DIRECTORY [" + juce::String(filteredPresets.size()) + " / " + juce::String(allPresets.size()) + " PATCHES]";
        drawCardFrame(centerCard, centerTitle.toRawUTF8(),  Palette::energeticAmber);
        drawCardFrame(rightCard,  "PATCH INSPECTOR & USER MANAGEMENT", Palette::deepMagenta);

        // 1. Left Column: Category Bank Buttons + Tag Filter Pills
        const juce::String catLabels[9] = {
            "ALL PATCHES [" + juce::String(catCounts[0]) + "]",
            "INIT & TEMPLATES [" + juce::String(catCounts[1]) + "]",
            "BASS & REESE [" + juce::String(catCounts[2]) + "]",
            "LEADS & SYNTHS [" + juce::String(catCounts[3]) + "]",
            "PADS & CHOIRS [" + juce::String(catCounts[4]) + "]",
            "PLUCKS & BELLS [" + juce::String(catCounts[5]) + "]",
            "ARP & SEQUENCES [" + juce::String(catCounts[6]) + "]",
            "FX & TRANSITIONS [" + juce::String(catCounts[7]) + "]",
            "USER PRESETS [" + juce::String(catCounts[8]) + "]"
        };
        float cy = leftCard.getY() + 28.0f;
        for (int i = 0; i < 9; ++i)
        {
            juce::Rectangle<float> btn(leftCard.getX() + 8.0f, cy, leftCard.getWidth() - 16.0f, 23.0f);
            const bool isSel = (selectedCategory == i);
            g.setColour(isSel ? Palette::electricCyan.withAlpha(0.26f) : Palette::trackDark);
            g.fillRoundedRectangle(btn, 3.5f);
            g.setColour(isSel ? Palette::electricCyan : Palette::borderSubtle);
            g.drawRoundedRectangle(btn, 3.5f, isSel ? 1.4f : 1.0f);
            g.setColour(isSel ? Palette::textPrimary : Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.2f, juce::Font::bold));
            g.drawText(catLabels[i], btn.reduced(10.0f, 0.0f), juce::Justification::centredLeft, false);
            cy += 25.5f;
        }

        cy += 4.0f;
        g.setColour(Palette::energeticAmber);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("FEATURE / PACK FILTER:", static_cast<int>(leftCard.getX() + 10.0f), static_cast<int>(cy), 180, 14,
                   juce::Justification::centredLeft, false);
        cy += 15.0f;

        static const char* tagLabels[5] = {
            "ALL FEATURES", "ZENOLOGY FX PACKS", "TIME SHAPER / GROSS BEAT", "VOCODER & TALKBOX", "FAVORITES [*]"
        };
        for (int t = 0; t < 5; ++t)
        {
            juce::Rectangle<float> tb(leftCard.getX() + 8.0f, cy, leftCard.getWidth() - 16.0f, 21.0f);
            const bool isSel = (selectedTagFilter == t);
            g.setColour(isSel ? Palette::energeticAmber.withAlpha(0.26f) : Palette::trackDark);
            g.fillRoundedRectangle(tb, 3.0f);
            g.setColour(isSel ? Palette::energeticAmber : Palette::borderSubtle);
            g.drawRoundedRectangle(tb, 3.0f, 1.0f);
            g.setColour(isSel ? Palette::textPrimary : Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(tagLabels[t], tb.reduced(10.0f, 0.0f), juce::Justification::centredLeft, false);
            cy += 23.5f;
        }

        cy += 4.0f;
        g.setColour(Palette::neonEmerald);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("GENRE EXPANSIONS (PHASE 5.5):", static_cast<int>(leftCard.getX() + 10.0f), static_cast<int>(cy), 180, 14,
                   juce::Justification::centredLeft, false);
        cy += 15.0f;

        static const char* genreLabels[5] = {
            "ALL GENRES", "CYBERPUNK & SYNTHWAVE", "CINEMATIC & AMBIENT", "MODERN TECH HOUSE", "LO-FI & CHILLHOP"
        };
        for (int gIdx = 0; gIdx < 5; ++gIdx)
        {
            juce::Rectangle<float> gb(leftCard.getX() + 8.0f, cy, leftCard.getWidth() - 16.0f, 21.0f);
            const bool isSel = (selectedGenreFilter == gIdx);
            g.setColour(isSel ? Palette::neonEmerald.withAlpha(0.26f) : Palette::trackDark);
            g.fillRoundedRectangle(gb, 3.0f);
            g.setColour(isSel ? Palette::neonEmerald : Palette::borderSubtle);
            g.drawRoundedRectangle(gb, 3.0f, 1.0f);
            g.setColour(isSel ? Palette::textPrimary : Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(genreLabels[gIdx], gb.reduced(10.0f, 0.0f), juce::Justification::centredLeft, false);
            cy += 23.5f;
        }

        // 2. Center Column: Scrollable/Paginated Preset Table
        const auto tableArea = centerCard.reduced(8.0f, 0.0f).withTrimmedTop(54.0f).withTrimmedBottom(8.0f);
        const auto headerBar = tableArea.withHeight(20.0f);
        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(headerBar, 3.0f);
        g.setColour(Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("FAV",         static_cast<int>(headerBar.getX() + 4.0f),   static_cast<int>(headerBar.getY()), 24, 20, juce::Justification::centred, false);
        g.drawText("PATCH NAME",  static_cast<int>(headerBar.getX() + 30.0f),  static_cast<int>(headerBar.getY()), 220, 20, juce::Justification::centredLeft, false);
        g.drawText("CATEGORY",    static_cast<int>(headerBar.getX() + 260.0f), static_cast<int>(headerBar.getY()), 90, 20, juce::Justification::centredLeft, false);
        g.drawText("OSC A/B WAVES",static_cast<int>(headerBar.getX() + 360.0f),static_cast<int>(headerBar.getY()), 110, 20, juce::Justification::centredLeft, false);
        g.drawText("ACTIVE FX & MODULATION", static_cast<int>(headerBar.getX() + 478.0f), static_cast<int>(headerBar.getY()), 220, 20, juce::Justification::centredLeft, false);

        const int curPresetIdx = processor.getCurrentPresetIndex();
        const float rowsTop = headerBar.getBottom() + 4.0f;
        const float rowH = 22.0f;
        const int maxVisibleRows = std::max(1, static_cast<int>((tableArea.getBottom() - 26.0f - rowsTop) / rowH));

        scrollOffset = juce::jlimit(0, std::max(0, static_cast<int>(filteredPresets.size()) - maxVisibleRows), scrollOffset);

        for (int r = 0; r < maxVisibleRows; ++r)
        {
            const int listIdx = scrollOffset + r;
            if (listIdx >= static_cast<int>(filteredPresets.size()))
                break;

            const auto& p = filteredPresets[static_cast<size_t>(listIdx)];
            const bool isCurrent = (p.index == curPresetIdx);
            const bool isFav = (p.index < static_cast<int>(favorites.size()) && favorites[static_cast<size_t>(p.index)]);

            juce::Rectangle<float> rowRect(tableArea.getX(), rowsTop + static_cast<float>(r) * rowH,
                                           tableArea.getWidth(), rowH - 2.0f);

            g.setColour(isCurrent ? Palette::electricCyan.withAlpha(0.24f) : ((r % 2 == 0) ? Palette::trackDark : Palette::bgObsidian.withAlpha(0.6f)));
            g.fillRoundedRectangle(rowRect, 2.5f);
            if (isCurrent)
            {
                g.setColour(Palette::electricCyan);
                g.drawRoundedRectangle(rowRect, 2.5f, 1.2f);
            }

            g.setColour(isFav ? Palette::energeticAmber : Palette::textDim);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::bold));
            g.drawText(isFav ? "*" : "+", static_cast<int>(rowRect.getX() + 6.0f), static_cast<int>(rowRect.getY()), 20, static_cast<int>(rowRect.getHeight()), juce::Justification::centred, false);

            g.setColour(isCurrent ? Palette::textPrimary : Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.4f, isCurrent ? juce::Font::bold : juce::Font::plain));
            g.drawText(p.name, static_cast<int>(rowRect.getX() + 30.0f), static_cast<int>(rowRect.getY()), 225, static_cast<int>(rowRect.getHeight()), juce::Justification::centredLeft, false);

            g.setColour(p.isUserPreset ? Palette::energeticAmber : Palette::electricCyan);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(p.category, static_cast<int>(rowRect.getX() + 260.0f), static_cast<int>(rowRect.getY()), 90, static_cast<int>(rowRect.getHeight()), juce::Justification::centredLeft, false);

            g.setColour(Palette::energeticAmber);
            g.drawText(p.wavesUsed, static_cast<int>(rowRect.getX() + 360.0f), static_cast<int>(rowRect.getY()), 110, static_cast<int>(rowRect.getHeight()), juce::Justification::centredLeft, false);

            g.setColour(Palette::deepMagenta.brighter(0.2f));
            g.drawText(p.fxUsed, static_cast<int>(rowRect.getX() + 478.0f), static_cast<int>(rowRect.getY()), 220, static_cast<int>(rowRect.getHeight()), juce::Justification::centredLeft, false);
        }

        // Pagination / Scroll Buttons at bottom of Center Table
        const juce::Rectangle<float> pageUpBtn  (tableArea.getRight() - 148.0f, tableArea.getBottom() - 20.0f, 70.0f, 19.0f);
        const juce::Rectangle<float> pageDownBtn(tableArea.getRight() - 74.0f,  tableArea.getBottom() - 20.0f, 70.0f, 19.0f);
        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(pageUpBtn, 3.0f);
        g.fillRoundedRectangle(pageDownBtn, 3.0f);
        g.setColour(Palette::electricCyan);
        g.drawRoundedRectangle(pageUpBtn, 3.0f, 1.0f);
        g.drawRoundedRectangle(pageDownBtn, 3.0f, 1.0f);
        g.setColour(Palette::textPrimary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("^ PAGE UP",   pageUpBtn,   juce::Justification::centred, false);
        g.drawText("v PAGE DOWN", pageDownBtn, juce::Justification::centred, false);

        g.setColour(Palette::textSecondary);
        g.drawText("SHOWING " + juce::String(filteredPresets.size()) + " MATCHING PATCHES (SCROLL WHEEL OR CLICK TO LOAD)",
                   static_cast<int>(tableArea.getX()), static_cast<int>(tableArea.getBottom() - 20.0f), 380, 19,
                   juce::Justification::centredLeft, false);

        // 3. Right Column: Patch Inspector Details
        const auto allBanks = processor.getFactoryPresetBank();
        const auto& activeMeta = allBanks[static_cast<size_t>(juce::jlimit(0, static_cast<int>(allBanks.size()) - 1, curPresetIdx))];
        auto insp = rightCard.reduced(12.0f, 32.0f);

        g.setColour(Palette::electricCyan);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.5f, juce::Font::bold));
        g.drawText(activeMeta.name, insp.removeFromTop(22.0f), juce::Justification::centredLeft, false);

        auto drawInspectorBadge = [&](const char* key, const juce::String& val, juce::Colour col)
        {
            auto row = insp.removeFromTop(24.0f);
            g.setColour(Palette::trackDark);
            g.fillRoundedRectangle(row, 3.5f);
            g.setColour(col.withAlpha(0.6f));
            g.drawRoundedRectangle(row, 3.5f, 1.0f);
            g.setColour(Palette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
            g.drawText(key, row.reduced(8.0f, 0.0f), juce::Justification::centredLeft, false);
            g.setColour(col);
            g.drawText(val, row.reduced(8.0f, 0.0f), juce::Justification::centredRight, false);
            insp.removeFromTop(4.0f);
        };

        drawInspectorBadge("CATEGORY BANK:",    activeMeta.category,  Palette::electricCyan);
        drawInspectorBadge("SOUND DESIGNER:",   activeMeta.author,    Palette::neonEmerald);
        drawInspectorBadge("FIELD 1 & 2 WAVES:",activeMeta.wavesUsed, Palette::energeticAmber);
        drawInspectorBadge("ACTIVE FX RACK:",   activeMeta.fxUsed,    Palette::deepMagenta.brighter(0.25f));

        insp.removeFromTop(2.0f);
        g.setColour(Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("PATCH ARCHITECTURE NOTES:", insp.removeFromTop(14.0f), juce::Justification::centredLeft, false);

        auto descBox = insp.removeFromTop(74.0f);
        g.setColour(Palette::trackDark);
        g.fillRoundedRectangle(descBox, 4.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(descBox, 4.0f, 1.0f);
        g.setColour(Palette::textPrimary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::plain));
        g.drawFittedText(activeMeta.description, descBox.reduced(8.0f).toNearestInt(), juce::Justification::topLeft, 5);

        // Section label for User Preset Actions
        insp.removeFromTop(6.0f);
        g.setColour(Palette::energeticAmber);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::bold));
        g.drawText("SERUM 2 USER PRESETS & I/O:", insp.removeFromTop(14.0f), juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float leftW = 210.0f;
        const float rightW = 290.0f;
        const float gap = 8.0f;

        const juce::Rectangle<float> centerCard(leftW + gap, bounds.getY(),
                                                bounds.getWidth() - leftW - rightW - gap * 2.0f, bounds.getHeight());
        const juce::Rectangle<float> rightCard(bounds.getRight() - rightW, bounds.getY(), rightW, bounds.getHeight());

        // Instant Search Input at top of Center Table
        searchBox.setBounds(static_cast<int>(centerCard.getX() + 8.0f),
                            static_cast<int>(centerCard.getY() + 27.0f),
                            static_cast<int>(centerCard.getWidth() - 16.0f - 24.0f),
                            22);
        clearSearchBtn.setBounds(static_cast<int>(centerCard.getRight() - 28.0f),
                                 static_cast<int>(centerCard.getY() + 27.0f),
                                 20,
                                 22);

        // Right Column Action Buttons
        const float btnY = rightCard.getBottom() - 104.0f;
        savePresetBtn.setBounds(static_cast<int>(rightCard.getX() + 12.0f), static_cast<int>(btnY), static_cast<int>(rightCard.getWidth() - 24.0f), 24);

        const float halfW = (rightCard.getWidth() - 30.0f) / 2.0f;
        importPresetBtn.setBounds(static_cast<int>(rightCard.getX() + 12.0f), static_cast<int>(btnY + 28.0f), static_cast<int>(halfW), 22);
        exportPresetBtn.setBounds(static_cast<int>(rightCard.getX() + 18.0f + halfW), static_cast<int>(btnY + 28.0f), static_cast<int>(halfW), 22);

        openFolderBtn.setBounds(static_cast<int>(rightCard.getX() + 12.0f), static_cast<int>(btnY + 54.0f), static_cast<int>(rightCard.getWidth() - 24.0f), 20);

        saveOverlay.setBounds(getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (saveOverlay.isVisible())
            return;

        const auto pos = e.position;
        const auto bounds = getLocalBounds().toFloat();
        const float leftW = 210.0f;
        const float rightW = 290.0f;
        const float gap = 8.0f;

        const juce::Rectangle<float> leftCard(bounds.getX(), bounds.getY(), leftW, bounds.getHeight());
        const juce::Rectangle<float> centerCard(leftCard.getRight() + gap, bounds.getY(),
                                                bounds.getWidth() - leftW - rightW - gap * 2.0f, bounds.getHeight());

        if (leftCard.contains(pos))
        {
            float cy = leftCard.getY() + 28.0f;
            for (int i = 0; i < 9; ++i)
            {
                juce::Rectangle<float> btn(leftCard.getX() + 8.0f, cy, leftCard.getWidth() - 16.0f, 23.0f);
                if (btn.contains(pos))
                {
                    selectedCategory = i;
                    scrollOffset = 0;
                    refreshFilteredList();
                    return;
                }
                cy += 25.5f;
            }

            cy += 19.0f;
            for (int t = 0; t < 5; ++t)
            {
                juce::Rectangle<float> tb(leftCard.getX() + 8.0f, cy, leftCard.getWidth() - 16.0f, 21.0f);
                if (tb.contains(pos))
                {
                    selectedTagFilter = t;
                    scrollOffset = 0;
                    refreshFilteredList();
                    return;
                }
                cy += 23.5f;
            }

            cy += 19.0f;
            for (int gIdx = 0; gIdx < 5; ++gIdx)
            {
                juce::Rectangle<float> gb(leftCard.getX() + 8.0f, cy, leftCard.getWidth() - 16.0f, 21.0f);
                if (gb.contains(pos))
                {
                    selectedGenreFilter = gIdx;
                    scrollOffset = 0;
                    refreshFilteredList();
                    return;
                }
                cy += 23.5f;
            }
            return;
        }

        if (centerCard.contains(pos))
        {
            const auto tableArea = centerCard.reduced(8.0f, 0.0f).withTrimmedTop(54.0f).withTrimmedBottom(8.0f);
            const juce::Rectangle<float> pageUpBtn  (tableArea.getRight() - 148.0f, tableArea.getBottom() - 20.0f, 70.0f, 19.0f);
            const juce::Rectangle<float> pageDownBtn(tableArea.getRight() - 74.0f,  tableArea.getBottom() - 20.0f, 70.0f, 19.0f);

            if (pageUpBtn.contains(pos))
            {
                scrollOffset = std::max(0, scrollOffset - 8);
                repaint();
                return;
            }
            if (pageDownBtn.contains(pos))
            {
                scrollOffset += 8;
                repaint();
                return;
            }

            const float rowsTop = tableArea.getY() + 24.0f;
            const float rowH = 22.0f;
            if (pos.y >= rowsTop && pos.y < tableArea.getBottom() - 24.0f)
            {
                const int rowIdx = static_cast<int>((pos.y - rowsTop) / rowH);
                const int listIdx = scrollOffset + rowIdx;
                if (listIdx >= 0 && listIdx < static_cast<int>(filteredPresets.size()))
                {
                    const int pIdx = filteredPresets[static_cast<size_t>(listIdx)].index;
                    if (pos.x < tableArea.getX() + 28.0f && pIdx < static_cast<int>(favorites.size()))
                    {
                        favorites[static_cast<size_t>(pIdx)] = !favorites[static_cast<size_t>(pIdx)];
                        refreshFilteredList();
                        return;
                    }
                    processor.loadFactoryPreset(pIdx);
                    if (onPresetLoaded)
                        onPresetLoaded(pIdx);
                    repaint();
                }
            }
        }
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (wheel.deltaY > 0.01f)
            scrollOffset = std::max(0, scrollOffset - 2);
        else if (wheel.deltaY < -0.01f)
            scrollOffset += 2;
        repaint();
    }

private:
    PrismorAudioProcessor& processor;
    std::vector<PrismorAudioProcessor::FactoryPresetMetadata> filteredPresets;
    std::vector<bool> favorites;
    int selectedCategory { 0 };
    int selectedTagFilter { 0 };
    int selectedGenreFilter { 0 };
    int scrollOffset { 0 };

    // Search bar
    juce::TextEditor searchBox;
    juce::TextButton clearSearchBtn;

    // Serum 2 Style User Preset Action Buttons
    juce::TextButton savePresetBtn   { "+ SAVE USER PRESET" };
    juce::TextButton importPresetBtn { "IMPORT" };
    juce::TextButton exportPresetBtn { "EXPORT" };
    juce::TextButton openFolderBtn   { "OPEN USER FOLDER" };

    SavePresetOverlay saveOverlay;
};

} // namespace PrismorUI
