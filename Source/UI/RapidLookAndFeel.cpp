#include "RapidLookAndFeel.h"

namespace PrismorUI
{

RapidLookAndFeel::RapidLookAndFeel()
{
    refreshThemeColours();
}

void RapidLookAndFeel::refreshThemeColours()
{
    setColour(juce::Slider::rotarySliderFillColourId, Palette::iceBlue);
    setColour(juce::ComboBox::backgroundColourId,     Palette::panelRecessed);
    setColour(juce::ComboBox::textColourId,           Palette::textPrimary);
    setColour(juce::ComboBox::outlineColourId,        Palette::borderSubtle);
    setColour(juce::PopupMenu::backgroundColourId,    Palette::panelDark);
    setColour(juce::PopupMenu::textColourId,          Palette::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::iceBlue.withAlpha(0.24f));
    setColour(juce::ScrollBar::thumbColourId,         Palette::panelElevated);
    setColour(juce::ScrollBar::backgroundColourId,    Palette::panelRecessed);
}

juce::Colour RapidLookAndFeel::getAccentForSlider(const juce::Slider& slider)
{
    const auto accentProp = slider.getProperties()["accent"];
    if (accentProp.isString())
    {
        const juce::String s = accentProp.toString();
        if (s == "amber" || s == "gold")    return Palette::goldOrange;
        if (s == "magenta" || s == "pink")  return Palette::pinkRed;
        if (s == "emerald" || s == "green") return Palette::greenLed;
        if (s == "cyan" || s == "blue")     return Palette::iceBlue;
    }
    return Palette::iceBlue;
}

// -----------------------------------------------------------------------------
// Rotary Sliders: Ringless 3D CNC-Machined Black Anodized Knobs (Phase 5.3)
// Reference: Authentic 3D cylinder depth, knurled barrel, stepped chamfer bevel,
// concentric spun aluminum micro-grooves, illuminated fiber-optic indicator pill,
// and discrete laser-engraved chassis micro-ticks / sub-surface LED scale.
// -----------------------------------------------------------------------------
void RapidLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    const juce::Colour accent = getAccentForSlider(slider);

    // 1. Digital Pitch Offset Box: Authentic RAPID Split Parameter Badge (OCT / SEMI / FIN / CRS)
    if (static_cast<bool>(slider.getProperties().getWithDefault("pitchBox", false)))
    {
        const auto box = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(width), static_cast<float>(height)).reduced(1.0f, 0.5f);
        const bool hovered = slider.isMouseOverOrDragging();

        // 1px Dark Outer Trench Groove (#0A0C10)
        g.setColour(Palette::borderDark);
        g.drawRoundedRectangle(box.expanded(0.5f), 3.0f, 1.0f);

        // Entire badge base
        g.setColour(Palette::panelRecessed);
        g.fillRoundedRectangle(box, 3.0f);

        // Split Badge Geometry: Left = Raised Matte Label Strip (48%), Right = Deep Recessed Box (52%)
        const float splitW = std::floor(box.getWidth() * 0.48f);
        const auto leftLabelRect = box.withWidth(splitW);
        const auto rightValRect  = box.withTrimmedLeft(splitW);

        // Left Label Surface (#222838 raised matte strip)
        juce::ColourGradient lblGrad(
            juce::Colour(0xFF222838), leftLabelRect.getX(), leftLabelRect.getY(),
            juce::Colour(0xFF171B26), leftLabelRect.getX(), leftLabelRect.getBottom(),
            false);
        g.setGradientFill(lblGrad);
        g.fillRoundedRectangle(leftLabelRect, 3.0f);
        g.fillRect(leftLabelRect.getRight() - 3.0f, leftLabelRect.getY(), 3.0f, leftLabelRect.getHeight());

        // Vertical 1px separating groove trench
        g.setColour(Palette::borderDark);
        g.drawVerticalLine(static_cast<int>(splitW + box.getX()), box.getY(), box.getBottom());

        // Right Numeric Box (#090B10 deeply recessed)
        g.setColour(juce::Colour(0xFF090B10));
        g.fillRoundedRectangle(rightValRect, 3.0f);
        g.fillRect(rightValRect.getX(), rightValRect.getY(), 3.0f, rightValRect.getHeight());

        // Outer border highlight
        g.setColour(hovered ? accent : Palette::borderSubtle);
        g.drawRoundedRectangle(box, 3.0f, hovered ? 1.4f : 1.0f);

        // Label text on left
        const juce::String boxTitle = slider.getProperties().getWithDefault("boxTitle", "OCT").toString();
        g.setColour(hovered ? accent : Palette::textSecondary);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 6.8f, juce::Font::bold));
        g.drawText(boxTitle, leftLabelRect, juce::Justification::centred, false);

        // Value text on right
        const double val = slider.getValue();
        const juce::String suffix = slider.getProperties().getWithDefault("boxSuffix", "").toString();
        const int decimals = slider.getProperties().getWithDefault("boxDecimals", 0);

        juce::String valStr;
        if (decimals > 0)
            valStr = (val > 0.04 ? "+" : "") + juce::String(val, decimals) + suffix;
        else
        {
            const int iVal = static_cast<int>(std::round(val));
            valStr = (iVal > 0 ? "+" : "") + juce::String(iVal) + suffix;
        }

        const bool isNonZero = std::abs(val) > 0.001;
        g.setColour(isNonZero ? accent : Palette::textPrimary);
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.2f, juce::Font::bold));
        g.drawText(valStr, rightValRect.reduced(1.0f, 0.0f), juce::Justification::centred, false);
        return;
    }

    // -------------------------------------------------------------------------
    // 2. Realistic 3D CNC-Machined Black Anodized Knobs (Hardware Replication)
    // -------------------------------------------------------------------------
    const bool isHeaderKnob = static_cast<bool>(slider.getProperties().getWithDefault("headerKnob", false));
    const bool isMasterKnob = static_cast<bool>(slider.getProperties().getWithDefault("masterKnob", false));
    const bool isBipolar    = static_cast<bool>(slider.getProperties().getWithDefault("bipolar", false));
    const bool isHovered    = slider.isMouseOverOrDragging();

    const juce::Colour activeAccent = isMasterKnob ? Palette::goldOrange : accent;

    const float boundsW = static_cast<float>(width);
    const float boundsH = static_cast<float>(height);
    const float minDim  = std::min(boundsW, boundsH);

    // Center coordinates
    const float cx = static_cast<float>(x) + boundsW * 0.5f;
    const float cy = static_cast<float>(y) + boundsH * 0.5f;

    // Angle calculation: toAngle in radians
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Proportions & Radii scaled to knob bounds
    const float tickRadius    = minDim * 0.44f;
    const float barrelRadius  = minDim * 0.35f;
    const float barrelDrop    = std::max(2.0f, minDim * 0.08f);
    const float capRadius     = minDim * 0.32f;
    const float faceRadius    = capRadius * 0.88f;

    // =========================================================================
    // (A) ENGRAVED CHASSIS MICRO-TICKS & SUB-SURFACE LED DOTS (No Solid Arcs!)
    // =========================================================================
    // Scale of 13 laser-engraved micro-dots around the outer perimeter (1.6px diameter)
    // Ticks up to current angle illuminate as crisp micro-LEDs; unreached remain dark slots.
    constexpr int numDots = 13;
    const float midAngle = rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle);

    for (int i = 0; i < numDots; ++i)
    {
        const float tNorm  = static_cast<float>(i) / static_cast<float>(numDots - 1);
        const float tAngle = rotaryStartAngle + tNorm * (rotaryEndAngle - rotaryStartAngle);

        const float rad  = tAngle - juce::MathConstants<float>::halfPi;
        const float dotX = cx + std::cos(rad) * tickRadius;
        const float dotY = cy + std::sin(rad) * tickRadius;

        bool lit = false;
        if (!isBipolar)
        {
            lit = (tAngle <= toAngle + 0.06f);
        }
        else
        {
            if (toAngle >= midAngle)
                lit = (tAngle >= midAngle - 0.06f && tAngle <= toAngle + 0.06f);
            else
                lit = (tAngle <= midAngle + 0.06f && tAngle >= toAngle - 0.06f);
        }

        if (lit)
        {
            // Illuminated discrete micro-LED dot
            const float glowR = isHovered ? 3.6f : 2.8f;
            g.setColour(activeAccent.withAlpha(isHovered ? 0.45f : 0.28f));
            g.fillEllipse(dotX - glowR * 0.5f, dotY - glowR * 0.5f, glowR, glowR);

            const float ledR = 1.8f;
            g.setColour(activeAccent.brighter(0.35f));
            g.fillEllipse(dotX - ledR * 0.5f, dotY - ledR * 0.5f, ledR, ledR);

            g.setColour(juce::Colours::white.withAlpha(isHovered ? 0.95f : 0.80f));
            g.fillEllipse(dotX - 0.5f, dotY - 0.5f, 1.0f, 1.0f);
        }
        else
        {
            // Dark engraved slot recessed into the chassis / header
            const float slotR = 1.5f;
            if (isHeaderKnob)
            {
                g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.6f));
                g.fillEllipse(dotX - slotR * 0.5f, dotY - slotR * 0.5f + 0.5f, slotR, slotR);
                g.setColour(juce::Colour(0xFF7A8498));
                g.fillEllipse(dotX - slotR * 0.5f, dotY - slotR * 0.5f, slotR, slotR);
            }
            else
            {
                g.setColour(juce::Colour(0xFF07080A));
                g.fillEllipse(dotX - slotR * 0.5f, dotY - slotR * 0.5f + 0.5f, slotR, slotR);
                g.setColour(juce::Colour(0xFF252A36));
                g.fillEllipse(dotX - slotR * 0.5f, dotY - slotR * 0.5f, slotR, slotR);
            }
        }
    }

    // =========================================================================
    // (B) 3D PERSPECTIVE & CAST SHADOW (Multi-Layer Downward Shift)
    // =========================================================================
    g.setColour(juce::Colours::black.withAlpha(isHeaderKnob ? 0.28f : 0.55f));
    g.fillEllipse(cx - barrelRadius * 1.02f, cy - barrelRadius * 0.96f + barrelDrop * 1.35f,
                  barrelRadius * 2.04f, barrelRadius * 2.0f);

    g.setColour(juce::Colours::black.withAlpha(isHeaderKnob ? 0.42f : 0.75f));
    g.fillEllipse(cx - barrelRadius * 0.98f, cy - barrelRadius * 0.94f + barrelDrop * 0.75f,
                  barrelRadius * 1.96f, barrelRadius * 1.92f);

    // =========================================================================
    // (C) KNURLED LOWER BARREL RIM (CYLINDRICAL BASE WITH TACTILE GRIP)
    // =========================================================================
    const float barrelY = cy + barrelDrop * 0.55f;

    juce::ColourGradient barrelGrad(
        juce::Colour(0xFF1E222B), cx - barrelRadius * 0.4f, cy,
        juce::Colour(0xFF0A0B0E), cx + barrelRadius * 0.7f, barrelY + barrelRadius,
        true);
    barrelGrad.addColour(0.45, juce::Colour(0xFF13161E));
    g.setGradientFill(barrelGrad);
    g.fillEllipse(cx - barrelRadius, barrelY - barrelRadius, barrelRadius * 2.0f, barrelRadius * 2.0f);

    // Radial diamond knurling ribs visible on lower/side barrel edge
    constexpr int numKnurlRibs = 48;
    for (int k = 0; k < numKnurlRibs; ++k)
    {
        const float kAng = (static_cast<float>(k) / static_cast<float>(numKnurlRibs)) * juce::MathConstants<float>::twoPi;
        const float sinA = std::sin(kAng);
        const float cosA = std::cos(kAng);

        if (sinA > -0.32f)
        {
            const float kIn  = capRadius * 0.96f;
            const float kOut = barrelRadius * 0.99f;
            const float kx0 = cx + cosA * kIn;
            const float ky0 = barrelY + sinA * kIn;
            const float kx1 = cx + cosA * kOut;
            const float ky1 = barrelY + sinA * kOut;

            g.setColour(juce::Colour(0xFF07080A));
            g.drawLine(kx0, ky0, kx1, ky1, 1.1f);

            const float specAlpha = 0.05f + 0.10f * std::max(0.0f, -cosA * 0.7f + sinA * 0.7f);
            g.setColour(juce::Colours::white.withAlpha(specAlpha));
            g.drawLine(kx0 - 0.5f, ky0 - 0.5f, kx1 - 0.5f, ky1 - 0.5f, 0.65f);
        }
    }

    g.setColour(juce::Colour(0xFF07080B));
    g.drawEllipse(cx - barrelRadius, barrelY - barrelRadius, barrelRadius * 2.0f, barrelRadius * 2.0f, 1.0f);

    // =========================================================================
    // (D) STEPPED CHAMFERED BEVEL RING (DARK-ANODIZED OUTER BEVEL)
    // =========================================================================
    juce::ColourGradient bevelGrad(
        juce::Colour(0xFF282D3A), cx - capRadius * 0.5f, cy - capRadius * 0.5f,
        juce::Colour(0xFF0E1015), cx + capRadius * 0.6f, cy + capRadius * 0.6f,
        true);
    bevelGrad.addColour(0.35, juce::Colour(0xFF1E222C));
    bevelGrad.addColour(0.70, juce::Colour(0xFF13151B));
    g.setGradientFill(bevelGrad);
    g.fillEllipse(cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Directional specular highlight along top-left chamfer arc
    juce::Path chamferPath;
    chamferPath.addCentredArc(cx, cy, capRadius - 0.6f, capRadius - 0.6f, 0.0f,
                              -juce::MathConstants<float>::pi * 0.75f,
                              juce::MathConstants<float>::pi * 0.15f, true);
    g.setColour(juce::Colours::white.withAlpha(0.20f));
    g.strokePath(chamferPath, juce::PathStrokeType(1.1f));

    // Shadow along bottom-right chamfer arc
    juce::Path shadowPath;
    shadowPath.addCentredArc(cx, cy, capRadius - 0.6f, capRadius - 0.6f, 0.0f,
                             juce::MathConstants<float>::pi * 0.25f,
                             juce::MathConstants<float>::pi * 1.15f, true);
    g.setColour(juce::Colour(0xFF07080A));
    g.strokePath(shadowPath, juce::PathStrokeType(1.1f));

    // Deep recessed inner groove separating bevel ring from turned face
    g.setColour(juce::Colour(0xFF07080A));
    g.drawEllipse(cx - faceRadius - 0.5f, cy - faceRadius - 0.5f, (faceRadius + 0.5f) * 2.0f, (faceRadius + 0.5f) * 2.0f, 1.2f);

    // =========================================================================
    // (E) CONCENTRIC MICRO-GROOVES (SPUN ALUMINUM TEXTURE ON TOP FACE)
    // =========================================================================
    juce::ColourGradient faceGrad(
        juce::Colour(0xFF1E2128), cx - faceRadius * 0.35f, cy - faceRadius * 0.35f,
        juce::Colour(0xFF111317), cx + faceRadius * 0.55f, cy + faceRadius * 0.55f,
        true);
    faceGrad.addColour(0.40, juce::Colour(0xFF181A20));
    g.setGradientFill(faceGrad);
    g.fillEllipse(cx - faceRadius, cy - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);

    // Procedural ultra-fine concentric circular micro-grooves
    const int numGrooves = static_cast<int>(faceRadius / 1.5f);
    for (int gIdx = 1; gIdx <= numGrooves; ++gIdx)
    {
        const float grR = static_cast<float>(gIdx) * 1.5f;
        if (grR >= faceRadius) break;

        const float alpha = (gIdx % 2 == 0) ? 0.035f : 0.022f;
        const juce::Colour ringCol = (gIdx % 2 == 0)
            ? juce::Colours::white.withAlpha(alpha)
            : juce::Colours::black.withAlpha(alpha * 1.4f);

        g.setColour(ringCol);
        g.drawEllipse(cx - grR, cy - grR, grR * 2.0f, grR * 2.0f, 0.75f);
    }

    // Subtle directional metallic reflection sheen across turned face
    juce::ColourGradient sheenGrad(
        juce::Colours::white.withAlpha(0.045f), cx - faceRadius * 0.6f, cy - faceRadius * 0.4f,
        juce::Colours::transparentWhite, cx + faceRadius * 0.3f, cy + faceRadius * 0.5f,
        false);
    g.setGradientFill(sheenGrad);
    g.fillEllipse(cx - faceRadius, cy - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);

    // Center lathe origin dot
    g.setColour(juce::Colour(0xFF0D0F13));
    g.fillEllipse(cx - 1.2f, cy - 1.2f, 2.4f, 2.4f);

    // =========================================================================
    // (F) ILLUMINATED FIBER-OPTIC INDICATOR PILL (HIGH-CONTRAST POSITION SLOT)
    // =========================================================================
    const float pAngle = toAngle - juce::MathConstants<float>::halfPi;
    const float cosP = std::cos(pAngle);
    const float sinP = std::sin(pAngle);

    const float pillOuterR = faceRadius * 0.88f;
    const float pillInnerR = faceRadius * 0.28f;
    const float pillLength = pillOuterR - pillInnerR;
    const float midR       = (pillInnerR + pillOuterR) * 0.5f;
    const float midX       = cx + cosP * midR;
    const float midY       = cy + sinP * midR;

    const float pillW = std::max(2.2f, std::round(faceRadius * 0.16f));

    juce::Path pillBase;
    pillBase.addRoundedRectangle(-pillW * 0.5f, -pillLength * 0.5f, pillW, pillLength, pillW * 0.5f);

    juce::AffineTransform pillXform = juce::AffineTransform::rotation(toAngle)
                                        .translated(midX, midY);

    juce::Path pillTransformed = pillBase;
    pillTransformed.applyTransform(pillXform);

    // 1. Recessed dark slot trench (#07080A)
    g.setColour(juce::Colour(0xFF07080A));
    g.strokePath(pillTransformed, juce::PathStrokeType(pillW * 0.55f));

    // 2. High-contrast brushed silver / gunmetal core (#B0B8C4 to #E2E8F0)
    g.setColour(isHovered ? juce::Colours::white : juce::Colour(0xFFD4DAE4));
    g.fillPath(pillTransformed);

    // 3. Backlit tritium-style glow along its inner tip/core (brightens on hover/drag)
    const float glowAlpha = isHovered ? 0.65f : 0.35f;
    juce::Colour glowCol = isHovered ? activeAccent.brighter(0.2f) : juce::Colour(0xFFE8F2FF);
    g.setColour(glowCol.withAlpha(glowAlpha));
    g.strokePath(pillTransformed, juce::PathStrokeType(1.1f));

    // 4. Ultra-bright inner filament highlight
    const float innerPillW = std::max(1.0f, pillW - 1.2f);
    juce::Path innerPill;
    innerPill.addRoundedRectangle(-innerPillW * 0.5f, -pillLength * 0.42f, innerPillW, pillLength * 0.84f, innerPillW * 0.5f);
    innerPill.applyTransform(pillXform);
    g.setColour(juce::Colours::white.withAlpha(isHovered ? 0.95f : 0.82f));
    g.fillPath(innerPill);
}

// -----------------------------------------------------------------------------
// Linear Sliders & Meters: Segmented LED Indicators & Metallic Handles
// -----------------------------------------------------------------------------
void RapidLookAndFeel::drawLinearSlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float /*minSliderPos*/, float /*maxSliderPos*/,
    const juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    const juce::Colour accent = getAccentForSlider(slider);
    const bool isVertical = (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical);
    const bool isABCrossfader = static_cast<bool>(slider.getProperties().getWithDefault("abCrossfader", false));

    if (isVertical)
    {
        const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        const float topY = static_cast<float>(y) + 6.0f;
        const float botY = static_cast<float>(y + height) - 6.0f;
        const float trackW = 6.0f;

        // Recessed track
        g.setColour(Palette::panelRecessed);
        g.fillRoundedRectangle(cx - trackW * 0.5f, topY, trackW, botY - topY, 3.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(cx - trackW * 0.5f, topY, trackW, botY - topY, 3.0f, 1.0f);

        // Segmented LED ladder tick marks along both sides
        constexpr int numTicks = 14;
        for (int i = 0; i < numTicks; ++i)
        {
            const float norm = static_cast<float>(i) / static_cast<float>(numTicks - 1);
            const float segY = botY - norm * (botY - topY);
            const bool lit = (segY >= sliderPos - 1.0f);

            g.setColour(lit ? accent.withAlpha(0.85f) : Palette::borderSubtle.withAlpha(0.35f));
            g.drawHorizontalLine(static_cast<int>(segY), cx - 11.0f, cx - 6.0f);
            g.drawHorizontalLine(static_cast<int>(segY), cx + 6.0f, cx + 11.0f);
        }

        // Active fill bar
        const float fillH = std::max(0.0f, botY - sliderPos);
        g.setColour(accent.withAlpha(0.25f));
        g.fillRoundedRectangle(cx - trackW * 0.5f - 1.0f, sliderPos, trackW + 2.0f, fillH, 2.5f);
        g.setColour(accent);
        g.fillRoundedRectangle(cx - trackW * 0.5f + 1.0f, sliderPos, trackW - 2.0f, fillH, 2.0f);

        // Metallic rectangular handle with center indicator notch
        const float thumbW = std::min(22.0f, static_cast<float>(width) - 4.0f);
        const float thumbH = 9.0f;
        juce::Rectangle<float> thumbRect(cx - thumbW * 0.5f, sliderPos - thumbH * 0.5f, thumbW, thumbH);

        juce::ColourGradient thumbGrad(
            juce::Colour(0xFF2C3345), thumbRect.getX(), thumbRect.getY(),
            juce::Colour(0xFF171B26), thumbRect.getX(), thumbRect.getBottom(),
            false);
        g.setGradientFill(thumbGrad);
        g.fillRoundedRectangle(thumbRect, 2.5f);
        g.setColour(slider.isMouseOverOrDragging() ? accent : juce::Colour(0xFF373F54));
        g.drawRoundedRectangle(thumbRect, 2.5f, 1.2f);

        // Center white LED notch
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(static_cast<int>(sliderPos), cx - thumbW * 0.35f, cx + thumbW * 0.35f);
    }
    else
    {
        const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float leftX = static_cast<float>(x) + 8.0f;
        const float rightX = static_cast<float>(x + width) - 8.0f;
        const float trackH = 7.0f;

        // Recessed track
        g.setColour(Palette::panelRecessed);
        g.fillRoundedRectangle(leftX, cy - trackH * 0.5f, rightX - leftX, trackH, 3.5f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(leftX, cy - trackH * 0.5f, rightX - leftX, trackH, 3.5f, 1.0f);

        if (isABCrossfader)
        {
            // Part A: Ice Blue fill
            g.setColour(Palette::iceBlue.withAlpha(0.85f));
            g.fillRoundedRectangle(leftX + 1.0f, cy - 2.0f, std::max(0.0f, sliderPos - leftX), 4.0f, 2.0f);

            // Part B: Pink-Red / Gold fill
            g.setColour(Palette::pinkRed.withAlpha(0.85f));
            g.fillRoundedRectangle(sliderPos, cy - 2.0f, std::max(0.0f, rightX - sliderPos - 1.0f), 4.0f, 2.0f);

            // Center notch
            const float midX = leftX + (rightX - leftX) * 0.5f;
            g.setColour(Palette::textSecondary.withAlpha(0.5f));
            g.drawVerticalLine(static_cast<int>(midX), cy - 6.0f, cy + 6.0f);
        }
        else
        {
            g.setColour(accent);
            g.fillRoundedRectangle(leftX + 1.0f, cy - 2.0f, std::max(0.0f, sliderPos - leftX), 4.0f, 2.0f);
        }

        // Metallic handle with center white line
        juce::Rectangle<float> thumb(sliderPos - 5.0f, cy - 9.0f, 10.0f, 18.0f);
        juce::ColourGradient thumbGrad(
            juce::Colour(0xFF2C3345), thumb.getX(), thumb.getY(),
            juce::Colour(0xFF171B26), thumb.getX(), thumb.getBottom(),
            false);
        g.setGradientFill(thumbGrad);
        g.fillRoundedRectangle(thumb, 2.5f);
        g.setColour(slider.isMouseOverOrDragging() ? accent : juce::Colour(0xFF373F54));
        g.drawRoundedRectangle(thumb, 2.5f, 1.2f);

        g.setColour(juce::Colours::white);
        g.drawVerticalLine(static_cast<int>(sliderPos), cy - 6.0f, cy + 6.0f);
    }
}

// -----------------------------------------------------------------------------
// Buttons: Dark Hardware Panels with Glowing LED Squares (r-btn in RAPID)
// -----------------------------------------------------------------------------
void RapidLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool isDown = shouldDrawButtonAsDown || button.getToggleState();

    // Check if this is a below-header navigation tab (Tier 2)
    const bool isNavTab = static_cast<bool>(button.getProperties().getWithDefault("isNavTab", false));
    if (isNavTab)
    {
        const bool isActive = static_cast<bool>(button.getProperties().getWithDefault("tabActive", false));
        const juce::Colour tabAccent = Palette::goldOrange;

        // RAPID interlocking hardware tab: #232834 active, #151821 inactive
        const juce::Colour bgCol = isActive ? juce::Colour(0xFF232834) : juce::Colour(0xFF151821);
        g.setColour(bgCol);
        g.fillRoundedRectangle(bounds, 3.5f);

        // 1px top highlight for active hardware tab
        if (isActive)
        {
            g.setColour(juce::Colours::white.withAlpha(0.14f));
            g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
        }

        // Hardware border
        g.setColour(isActive ? tabAccent.withAlpha(0.75f) : (shouldDrawButtonAsHighlighted ? Palette::borderBright : Palette::borderSubtle));
        g.drawRoundedRectangle(bounds, 3.5f, isActive ? 1.3f : 1.0f);

        // Square LED status indicator (warm gold for active tab, muted dark for inactive)
        const float sqSize = 5.0f;
        const juce::Rectangle<float> sqRect(bounds.getX() + 9.0f, bounds.getCentreY() - sqSize * 0.5f, sqSize, sqSize);
        if (isActive)
        {
            g.setColour(tabAccent.withAlpha(0.40f));
            g.fillRoundedRectangle(sqRect.expanded(1.5f), 1.0f);
            g.setColour(tabAccent);
            g.fillRoundedRectangle(sqRect, 1.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xFF202636));
            g.fillRoundedRectangle(sqRect, 1.0f);
            g.setColour(Palette::borderSubtle);
            g.drawRoundedRectangle(sqRect, 1.0f, 0.8f);
        }
        return;
    }

    juce::Colour btnCol = backgroundColour;
    if (btnCol.isTransparent())
        btnCol = isDown ? Palette::panelElevated : Palette::panelDark;

    // Dark hardware panel gradient
    const juce::Colour topCol = isDown ? btnCol.brighter(0.08f) : btnCol.brighter(0.12f);
    const juce::Colour botCol = isDown ? btnCol.darker(0.06f) : btnCol;
    juce::ColourGradient btnGrad(topCol, bounds.getX(), bounds.getY(), botCol, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(btnGrad);
    g.fillRoundedRectangle(bounds, 3.5f);

    // Subtle 1px top highlight
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

    // Recessed hardware border
    const juce::Colour borderCol = shouldDrawButtonAsHighlighted
        ? Palette::borderBright
        : (isDown ? Palette::iceBlue.withAlpha(0.7f) : Palette::borderSubtle);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, 3.5f, isDown ? 1.4f : 1.0f);

    // Subtle glowing LED square indicator if toggled or active
    if (isDown && bounds.getWidth() > 50.0f)
    {
        const float sqSize = 5.0f;
        const juce::Rectangle<float> sqRect(bounds.getX() + 7.0f, bounds.getCentreY() - sqSize * 0.5f, sqSize, sqSize);
        g.setColour(Palette::iceBlue.withAlpha(0.35f));
        g.fillRoundedRectangle(sqRect.expanded(2.0f), 1.5f);
        g.setColour(Palette::iceBlue);
        g.fillRoundedRectangle(sqRect, 1.0f);
    }
}

void RapidLookAndFeel::drawButtonText(
    juce::Graphics& g,
    juce::TextButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    const bool isNavTab = static_cast<bool>(button.getProperties().getWithDefault("isNavTab", false));
    if (isNavTab)
    {
        const bool isActive = static_cast<bool>(button.getProperties().getWithDefault("tabActive", false));
        auto r = button.getLocalBounds().reduced(4, 1);
        r.removeFromLeft(14); // Indent past square LED
        g.setColour(isActive ? Palette::textPrimary : (shouldDrawButtonAsHighlighted ? Palette::textPrimary : Palette::textSecondary));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.2f, juce::Font::bold));
        g.drawText(button.getButtonText(), r, juce::Justification::centred, false);
        return;
    }

    auto textBounds = button.getLocalBounds().reduced(4, 1);
    const bool isDown = shouldDrawButtonAsDown || button.getToggleState();

    if (isDown && textBounds.getWidth() > 50)
        textBounds.removeFromLeft(12); // Leave room for the glowing LED square

    const juce::Colour textCol = isDown
        ? Palette::textPrimary
        : (shouldDrawButtonAsHighlighted ? Palette::textPrimary : Palette::textSecondary);

    g.setColour(textCol);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.2f, juce::Font::bold));
    g.drawText(button.getButtonText(), textBounds, juce::Justification::centred, false);
}

void RapidLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool /*shouldDrawButtonAsDown*/)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();

    juce::Colour accent = Palette::iceBlue;
    const auto prop = button.getProperties()["accent"];
    if (prop == "magenta" || prop == "pink")     accent = Palette::pinkRed;
    else if (prop == "amber" || prop == "gold")  accent = Palette::goldOrange;
    else if (prop == "emerald" || prop == "green") accent = Palette::greenLed;

    // Recessed hardware chassis
    g.setColour(on ? accent.withAlpha(0.20f) : Palette::panelRecessed);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(on ? accent : (shouldDrawButtonAsHighlighted ? Palette::borderBright : Palette::borderSubtle));
    g.drawRoundedRectangle(bounds, 3.0f, on ? 1.3f : 1.0f);

    // Signature RAPID square LED indicator (panel-indicator-sq)
    const float sqSize = 5.0f;
    const float sqX = bounds.getX() + 7.0f;
    const float sqY = bounds.getCentreY() - sqSize * 0.5f;
    const juce::Rectangle<float> sqRect(sqX, sqY, sqSize, sqSize);

    if (on)
    {
        g.setColour(accent.withAlpha(0.38f));
        g.fillRoundedRectangle(sqRect.expanded(2.0f), 1.5f);
        g.setColour(accent);
        g.fillRoundedRectangle(sqRect, 1.0f);
    }
    else
    {
        g.setColour(Palette::panelElevated);
        g.fillRoundedRectangle(sqRect, 1.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(sqRect, 1.0f, 1.0f);
    }

    g.setColour(on ? Palette::textPrimary : Palette::textDim);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.2f, juce::Font::bold));
    const juce::String txt = button.getButtonText().isNotEmpty()
        ? button.getButtonText()
        : (on ? "ON" : "BYP");
    g.drawText(txt, bounds.withTrimmedLeft(16.0f), juce::Justification::centred, false);
}

// -----------------------------------------------------------------------------
// ComboBox & Menus: Hardware Inset Selectors (r-select in RAPID)
// -----------------------------------------------------------------------------
void RapidLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width, int height,
    bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);

    // Dedicated styling for the Top Header Recessed LCD Preset Display
    if (static_cast<bool>(box.getProperties().getWithDefault("isPresetSlot", false)))
    {
        g.setColour(juce::Colour(0xFF090B10));
        g.fillRoundedRectangle(bounds, 3.0f);

        const bool hovered = box.isMouseOver();
        g.setColour(hovered ? Palette::iceBlue : juce::Colour(0xFF384052));
        g.drawRoundedRectangle(bounds, 3.0f, hovered ? 1.2f : 1.0f);

        // Downward chevron arrow
        const float arrowX = static_cast<float>(width) - 10.0f;
        const float arrowY = static_cast<float>(height) * 0.5f;
        juce::Path p;
        p.startNewSubPath(arrowX - 3.0f, arrowY - 1.8f);
        p.lineTo(arrowX, arrowY + 2.2f);
        p.lineTo(arrowX + 3.0f, arrowY - 1.8f);
        g.setColour(Palette::iceBlue);
        g.strokePath(p, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    // Gradient fill from #1f2432 to #151822
    juce::ColourGradient comboGrad(
        Palette::panelElevated, bounds.getX(), bounds.getY(),
        Palette::panelDark, bounds.getX(), bounds.getBottom(),
        false);
    g.setGradientFill(comboGrad);
    g.fillRoundedRectangle(bounds, 3.5f);

    const juce::Colour accent = (box.getProperties()["accent"] == "amber" || box.getProperties()["accent"] == "gold")
        ? Palette::goldOrange
        : ((box.getProperties()["accent"] == "magenta" || box.getProperties()["accent"] == "pink")
            ? Palette::pinkRed
            : Palette::iceBlue);

    const bool hovered = box.isMouseOver();
    g.setColour(hovered ? accent.withAlpha(0.85f) : Palette::borderBright);
    g.drawRoundedRectangle(bounds, 3.5f, hovered ? 1.3f : 1.0f);

    // Downward chevron arrow
    const float arrowX = static_cast<float>(width) - 13.0f;
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path p;
    p.startNewSubPath(arrowX - 3.5f, arrowY - 2.0f);
    p.lineTo(arrowX, arrowY + 2.5f);
    p.lineTo(arrowX + 3.5f, arrowY - 2.0f);
    g.setColour(accent);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font RapidLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/)
{
    return juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.5f, juce::Font::bold);
}

void RapidLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    g.setColour(Palette::panelDark);
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(Palette::borderBright);
    g.drawRoundedRectangle(bounds, 5.0f, 1.2f);
}

void RapidLookAndFeel::drawPopupMenuItem(
    juce::Graphics& g,
    const juce::Rectangle<int>& area,
    bool isSeparator, bool isActive, bool isHighlighted,
    bool isTicked, bool /*hasSubMenu*/,
    const juce::String& text, const juce::String& shortcutKeyText,
    const juce::Drawable* /*icon*/, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        const int midY = area.getCentreY();
        g.setColour(Palette::borderSubtle);
        g.drawHorizontalLine(midY, static_cast<float>(area.getX() + 6), static_cast<float>(area.getRight() - 6));
        return;
    }

    auto r = area.toFloat().reduced(4.0f, 1.0f);

    if (isHighlighted && isActive)
    {
        g.setColour(Palette::panelElevated);
        g.fillRoundedRectangle(r, 3.5f);
        g.setColour(Palette::iceBlue.withAlpha(0.6f));
        g.drawRoundedRectangle(r, 3.5f, 1.0f);
    }

    if (isTicked)
    {
        g.setColour(Palette::iceBlue);
        g.fillRoundedRectangle(r.getX() + 4.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f, 1.0f);
    }

    g.setColour(textColour != nullptr ? *textColour : (isActive ? Palette::textPrimary : Palette::textDim));
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, isHighlighted ? juce::Font::bold : juce::Font::plain));
    g.drawText(text, static_cast<int>(r.getX() + 18.0f), static_cast<int>(r.getY()),
               static_cast<int>(r.getWidth() - 24.0f), static_cast<int>(r.getHeight()),
               juce::Justification::centredLeft, false);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour(Palette::textSecondary);
        g.drawText(shortcutKeyText, r.reduced(8.0f, 0.0f), juce::Justification::centredRight, false);
    }
}

void RapidLookAndFeel::drawScrollbar(
    juce::Graphics& g,
    juce::ScrollBar& /*scrollbar*/,
    int x, int y, int width, int height,
    bool isScrollbarVertical,
    int thumbStartPosition, int thumbSize,
    bool isMouseOver, bool isMouseDown)
{
    g.setColour(Palette::panelRecessed);
    g.fillRect(x, y, width, height);

    const auto thumbRect = isScrollbarVertical
        ? juce::Rectangle<float>(static_cast<float>(x) + 1.0f, static_cast<float>(thumbStartPosition),
                                 static_cast<float>(width) - 2.0f, static_cast<float>(thumbSize))
        : juce::Rectangle<float>(static_cast<float>(thumbStartPosition), static_cast<float>(y) + 1.0f,
                                 static_cast<float>(thumbSize), static_cast<float>(height) - 2.0f);

    const juce::Colour col = (isMouseDown || isMouseOver)
        ? Palette::iceBlue.withAlpha(0.85f)
        : Palette::borderBright;
    g.setColour(col);
    g.fillRoundedRectangle(thumbRect, 2.5f);
}

} // namespace PrismorUI
