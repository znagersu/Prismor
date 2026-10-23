#pragma once
#include <JuceHeader.h>

/**
 * ==============================================================================
 * DESIGN DRAFT — PARAWAVE RAPID AESTHETIC PROTOTYPE
 * Location: /design_sandbox/RapidStyleLookAndFeel.h
 * 
 * Standalone LookAndFeel draft inspired by Parawave RAPID:
 * - Brushed silver metallic header accents
 * - Dark recessed circular knob tracks (#0d0f15)
 * - Vivid Electric Ice Blue (#4faaf7) and Warm Gold-Orange (#e5a43b) value arcs
 * - Cylindrical matte-slate knob caps with notched white pointer
 * - Square LED indicators (■) and high-contrast parameter value badges
 * ==============================================================================
 */

namespace PrismorDesignSandbox
{

struct RapidPalette
{
    static inline const juce::Colour chassisDark      { 0xff11141b };
    static inline const juce::Colour panelDark        { 0xff161922 };
    static inline const juce::Colour panelElevated    { 0xff1b1f2b };
    static inline const juce::Colour panelRecessed    { 0xff0e1017 };
    
    // Silver Metallic Header
    static inline const juce::Colour silverTop        { 0xffd8dce4 };
    static inline const juce::Colour silverBottom     { 0xffb4b8c3 };
    static inline const juce::Colour silverText       { 0xff2b303e };

    // RAPID Signature Accents
    static inline const juce::Colour iceBlue          { 0xff4faaf7 };
    static inline const juce::Colour iceBlueGlow      { 0x614faaf7 };
    static inline const juce::Colour goldOrange       { 0xffe5a43b };
    static inline const juce::Colour goldOrangeGlow   { 0x61e5a43b };
    static inline const juce::Colour pinkRed          { 0xfff43f5e };
    static inline const juce::Colour greenLed         { 0xff10b981 };

    static inline const juce::Colour textBright       { 0xffe2e8f0 };
    static inline const juce::Colour textMuted        { 0xff828ba0 };
    static inline const juce::Colour textDim          { 0xff4f576d };
    static inline const juce::Colour borderDark       { 0xff0a0c12 };
    static inline const juce::Colour borderHighlight  { 0x14ffffff };
};

class RapidStyleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RapidStyleLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, RapidPalette::chassisDark);
        setColour(juce::ComboBox::backgroundColourId,         RapidPalette::panelRecessed);
        setColour(juce::ComboBox::textColourId,             RapidPalette::textBright);
        setColour(juce::ComboBox::outlineColourId,          juce::Colour(0xff30374a));
        setColour(juce::TextButton::buttonColourId,          RapidPalette::panelElevated);
        setColour(juce::TextButton::textColourOffId,         RapidPalette::textMuted);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float centreX = bounds.getCentreX();
        const float centreY = bounds.getCentreY();

        // 1. Digital Pitch Box (RAPID Inset Badge Style)
        if (slider.getProperties()["pitchBox"])
        {
            const auto boxRect = bounds.reduced(1.0f);
            const juce::String title = slider.getProperties().getWithDefault("boxTitle", juce::String());
            const juce::String suffix = slider.getProperties().getWithDefault("boxSuffix", juce::String());
            const int decimals = slider.getProperties().getWithDefault("boxDecimals", 0);

            // Inset dark badge
            g.setColour(RapidPalette::panelRecessed);
            g.fillRoundedRectangle(boxRect, 3.0f);
            g.setColour(slider.isMouseOverOrDragging() ? RapidPalette::iceBlue : juce::Colour(0xff232938));
            g.drawRoundedRectangle(boxRect, 3.0f, 1.0f);

            // Label
            g.setColour(RapidPalette::textMuted);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(title, boxRect.withHeight(10.0f), juce::Justification::centred, false);

            // Value
            const float val = static_cast<float>(slider.getValue());
            juce::String valStr;
            if (val > 0.001f) valStr << "+" << juce::String(val, decimals) << suffix;
            else if (val < -0.001f) valStr << juce::String(val, decimals) << suffix;
            else valStr << "0" << suffix;

            g.setColour(std::abs(val) > 0.001f ? RapidPalette::iceBlue : RapidPalette::textBright);
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::bold));
            g.drawText(valStr, boxRect.withTrimmedTop(9.0f), juce::Justification::centred, false);
            return;
        }

        // 2. RAPID Iconic Rotary Knob: Inset Radial Track + Slate Cylindrical Cap + Value Arc
        const float currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float arcStroke = 3.0f;

        // Inset Outer Track Ring (Sunken look)
        const auto ringBounds = juce::Rectangle<float>(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
        juce::ColourGradient ringGrad(
            juce::Colour(0xff0d0f15), centreX, centreY,
            juce::Colour(0xff161a24), centreX, centreY + radius,
            true);
        g.setGradientFill(ringGrad);
        g.fillEllipse(ringBounds);
        g.setColour(juce::Colour(0xff202636));
        g.drawEllipse(ringBounds, 1.0f);

        // Track Arc Groove
        juce::Path bgTrack;
        bgTrack.addCentredArc(centreX, centreY, radius - arcStroke * 0.7f, radius - arcStroke * 0.7f,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff12151f));
        g.strokePath(bgTrack, juce::PathStrokeType(arcStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active Colored Value Arc (Ice Blue or Gold-Orange)
        juce::Colour accent = RapidPalette::iceBlue;
        const auto accentStr = slider.getProperties().getWithDefault("accent", "cyan").toString();
        if (accentStr == "amber" || accentStr == "gold" || accentStr == "honey")
            accent = RapidPalette::goldOrange;
        else if (accentStr == "magenta" || accentStr == "pink")
            accent = RapidPalette::pinkRed;

        if (sliderPos > 0.001f)
        {
            juce::Path activeArc;
            activeArc.addCentredArc(centreX, centreY, radius - arcStroke * 0.7f, radius - arcStroke * 0.7f,
                                    0.0f, rotaryStartAngle, currentAngle, true);
            
            // Soft glow
            g.setColour(accent.withAlpha(0.35f));
            g.strokePath(activeArc, juce::PathStrokeType(arcStroke * 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Sharp core
            g.setColour(accent);
            g.strokePath(activeArc, juce::PathStrokeType(arcStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Cylindrical Center Cap with Fine Metallic Chamfer
        const float capRadius = radius * 0.62f;
        const auto capBounds = juce::Rectangle<float>(centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        juce::ColourGradient capGrad(
            juce::Colour(0xff2c3345), centreX - capRadius * 0.4f, centreY - capRadius * 0.4f,
            juce::Colour(0xff171b26), centreX + capRadius * 0.6f, centreY + capRadius * 0.6f,
            true);
        g.setGradientFill(capGrad);
        g.fillEllipse(capBounds);

        // Cap Bevel Edge
        g.setColour(juce::Colour(0xff373f54));
        g.drawEllipse(capBounds, 1.0f);

        // High-Contrast White Needle Pointer
        const float pInner = capRadius * 0.30f;
        const float pOuter = capRadius * 0.92f;
        const float sinA = std::sin(currentAngle);
        const float cosA = -std::cos(currentAngle);

        juce::Path pointer;
        pointer.startNewSubPath(centreX + pInner * sinA, centreY + pInner * cosA);
        pointer.lineTo(centreX + pOuter * sinA, centreY + pOuter * cosA);

        g.setColour(juce::Colours::white);
        g.strokePath(pointer, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

} // namespace PrismorDesignSandbox
