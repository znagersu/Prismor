#pragma once
#include <JuceHeader.h>

/**
 * ==============================================================================
 * DESIGN DRAFT — CONCEPT C: OBSIDIAN GLASS
 * Location: /design_sandbox/ObsidianGlassLookAndFeel.h
 * 
 * Standalone LookAndFeel draft implementing the soft dark "Obsidian Glass"
 * aesthetic for Prismor:
 * - Acrylic translucent card panels with 1px top specular highlights
 * - Velvet Mint (#38e1b0) and Ethereal Lavender (#a78bfa) soft glow accents
 * - Sleek floating disc rotary knobs without harsh radial tick marks
 * - Monospace digital pitch badges (OCT, SEMI, FIN, CRS)
 * - Muted inactive states to eliminate sensory clutter
 * ==============================================================================
 */

namespace PrismorDesignSandbox
{

struct ObsidianGlassPalette
{
    static inline const juce::Colour bgVoid          { 0xff090b10 };
    static inline const juce::Colour bgObsidian      { 0xff0d1017 };
    static inline const juce::Colour cardGlass        { 0xb8131824 }; // 72% opacity dark slate
    static inline const juce::Colour cardHeader       { 0xa61b2131 }; // 65% opacity
    static inline const juce::Colour trackDark        { 0xff121622 };
    static inline const juce::Colour borderGlass      { 0x14ffffff }; // ~8% white alpha
    static inline const juce::Colour borderSpecular   { 0x26ffffff }; // ~15% white alpha

    // Velvet Accents
    static inline const juce::Colour velvetMint       { 0xff38e1b0 };
    static inline const juce::Colour velvetMintGlow   { 0x5238e1b0 };
    static inline const juce::Colour etherealLavender { 0xffa78bfa };
    static inline const juce::Colour etherealLavGlow  { 0x52a78bfa };
    static inline const juce::Colour warmHoney        { 0xfffbbf24 };
    static inline const juce::Colour softRose         { 0xfff43f5e };

    static inline const juce::Colour textPrimary      { 0xfff0f4fc };
    static inline const juce::Colour textSecondary    { 0xff8c96ac };
    static inline const juce::Colour textDim          { 0xff4b5468 };
};

class ObsidianGlassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ObsidianGlassLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, ObsidianGlassPalette::bgObsidian);
        setColour(juce::ComboBox::backgroundColourId,         ObsidianGlassPalette::trackDark);
        setColour(juce::ComboBox::textColourId,             ObsidianGlassPalette::textPrimary);
        setColour(juce::ComboBox::outlineColourId,          ObsidianGlassPalette::borderGlass);
        setColour(juce::TextButton::buttonColourId,          ObsidianGlassPalette::trackDark);
        setColour(juce::TextButton::textColourOffId,         ObsidianGlassPalette::textSecondary);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float centreX = bounds.getCentreX();
        const float centreY = bounds.getCentreY();
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;

        // Custom Pitch Box Rendering
        if (slider.getProperties()["pitchBox"])
        {
            const auto boxRect = bounds.reduced(1.0f);
            const juce::String title = slider.getProperties().getWithDefault("boxTitle", juce::String());
            const juce::String suffix = slider.getProperties().getWithDefault("boxSuffix", juce::String());
            const int decimals = slider.getProperties().getWithDefault("boxDecimals", 0);
            const juce::Colour accent = slider.getProperties().getWithDefault("accent", "mint") == "lavender"
                ? ObsidianGlassPalette::etherealLavender
                : ObsidianGlassPalette::velvetMint;

            g.setColour(ObsidianGlassPalette::cardGlass);
            g.fillRoundedRectangle(boxRect, 4.0f);
            g.setColour(slider.isMouseOverOrDragging() ? accent.withAlpha(0.6f) : ObsidianGlassPalette::borderGlass);
            g.drawRoundedRectangle(boxRect, 4.0f, 1.0f);

            // Label
            g.setColour(ObsidianGlassPalette::textSecondary);
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 7.8f, juce::Font::bold));
            g.drawText(title, boxRect.withHeight(11.0f), juce::Justification::centred, false);

            // Value
            const float val = static_cast<float>(slider.getValue());
            juce::String valStr;
            if (val > 0.001f) valStr << "+" << juce::String(val, decimals) << suffix;
            else if (val < -0.001f) valStr << juce::String(val, decimals) << suffix;
            else valStr << "0" << suffix;

            g.setColour(std::abs(val) > 0.001f ? accent : ObsidianGlassPalette::textPrimary);
            g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::bold));
            g.drawText(valStr, boxRect.withTrimmedTop(10.0f), juce::Justification::centred, false);
            return;
        }

        // Clean Obsidian Glass Rotary Knob (No busy tick clusters!)
        const float currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float arcStroke = 2.4f;

        // Background Track Arc
        juce::Path bgTrack;
        bgTrack.addCentredArc(centreX, centreY, radius - arcStroke, radius - arcStroke,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(ObsidianGlassPalette::trackDark);
        g.strokePath(bgTrack, juce::PathStrokeType(arcStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active Value Arc with Soft Diffused Glow
        juce::Colour accent = ObsidianGlassPalette::velvetMint;
        const auto accentStr = slider.getProperties().getWithDefault("accent", "mint").toString();
        if (accentStr == "lavender") accent = ObsidianGlassPalette::etherealLavender;
        else if (accentStr == "honey") accent = ObsidianGlassPalette::warmHoney;
        else if (accentStr == "rose")  accent = ObsidianGlassPalette::softRose;

        if (sliderPos > 0.001f)
        {
            juce::Path activeArc;
            activeArc.addCentredArc(centreX, centreY, radius - arcStroke, radius - arcStroke,
                                    0.0f, rotaryStartAngle, currentAngle, true);

            // Soft bloom glow
            g.setColour(accent.withAlpha(0.25f));
            g.strokePath(activeArc, juce::PathStrokeType(arcStroke * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Sharp core arc
            g.setColour(accent);
            g.strokePath(activeArc, juce::PathStrokeType(arcStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Floating Center Disc Cap
        const float capRadius = radius * 0.65f;
        const auto capBounds = juce::Rectangle<float>(centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        juce::ColourGradient capGrad(
            juce::Colour(0xff22293a), centreX - capRadius * 0.4f, centreY - capRadius * 0.4f,
            juce::Colour(0xff121622), centreX + capRadius * 0.7f, centreY + capRadius * 0.7f,
            true);
        g.setGradientFill(capGrad);
        g.fillEllipse(capBounds);

        // Cap subtle rim light
        g.setColour(ObsidianGlassPalette::borderSpecular);
        g.drawEllipse(capBounds, 1.0f);

        // Radial Sleek Line Pointer
        const float pInner = capRadius * 0.35f;
        const float pOuter = capRadius * 0.90f;
        const float sinA = std::sin(currentAngle);
        const float cosA = -std::cos(currentAngle);

        juce::Path pointer;
        pointer.startNewSubPath(centreX + pInner * sinA, centreY + pInner * cosA);
        pointer.lineTo(centreX + pOuter * sinA, centreY + pOuter * cosA);

        g.setColour(juce::Colours::white);
        g.strokePath(pointer, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

} // namespace PrismorDesignSandbox
