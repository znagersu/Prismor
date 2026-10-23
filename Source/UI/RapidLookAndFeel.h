#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace PrismorUI
{

namespace Palette
{
    inline int currentThemeIndex = 0;

    // -------------------------------------------------------------------------
    // Parawave RAPID Inspired Default Palette
    // -------------------------------------------------------------------------
    inline juce::Colour chassisBg        { 0xFF11141B };
    inline juce::Colour panelDark        { 0xFF161922 };
    inline juce::Colour panelElevated    { 0xFF1E2330 };
    inline juce::Colour panelRecessed    { 0xFF090B10 };
    inline juce::Colour headerSilverTop  { 0xFFD8DCE4 };
    inline juce::Colour headerSilverMid  { 0xFFC4C8D2 };
    inline juce::Colour headerSilverBot  { 0xFFB4B8C3 };
    inline juce::Colour borderDark       { 0xFF0A0C10 };
    inline juce::Colour borderSubtle     { 0xFF232838 };
    inline juce::Colour borderBright     { 0xFF363E54 };

    // Signature RAPID Accents
    inline juce::Colour iceBlue          { 0xFF4FAAF7 }; // Oscillators, Pitch, Waveforms
    inline juce::Colour iceGlow          { 0x604FAAF7 };
    inline juce::Colour goldOrange       { 0xFFE5A43B }; // Filters, Envelopes, Warm Harms
    inline juce::Colour goldGlow         { 0x60E5A43B };
    inline juce::Colour pinkRed          { 0xFFF43F5E }; // Routing, Special, Modulation
    inline juce::Colour greenLed         { 0xFF10B981 }; // Limiter, Master LED, Power

    // Theme Compatibility Aliases
    inline juce::Colour bgObsidian       { 0xFF11141B };
    inline juce::Colour bgCharcoal       { 0xFF161922 };
    inline juce::Colour cardSurface      { 0xFF161922 };
    inline juce::Colour cardHeader       { 0xFF1E2330 };
    inline juce::Colour electricCyan     { 0xFF4FAAF7 }; // Ice Blue
    inline juce::Colour deepMagenta      { 0xFFF43F5E }; // Pink / Red
    inline juce::Colour energeticAmber   { 0xFFE5A43B }; // Warm Gold
    inline juce::Colour neonEmerald      { 0xFF10B981 }; // Green LED
    inline juce::Colour textPrimary      { 0xFFE2E8F0 };
    inline juce::Colour textSecondary    { 0xFF828BA0 };
    inline juce::Colour textDim          { 0xFF4F576D };
    inline juce::Colour trackDark        { 0xFF090B10 };

    inline juce::StringArray getThemeNames()
    {
        return {
            "THEME: PARAWAVE RAPID",
            "THEME: VALHALLA AMBER",
            "THEME: OBSIDIAN CYAN",
            "THEME: DEEP CRIMSON",
            "THEME: MATRIX GREEN",
            "THEME: CLEAN MINIMAL",
            "THEME: CYBERPUNK PINK/BLUE",
            "THEME: SOLARIZED DARK",
            "THEME: MONOCHROME OLED",
            "THEME: SYNTHWAVE PURPLE",
            "THEME: NORD FROST BLUE"
        };
    }

    inline void applyTheme(int index) noexcept
    {
        currentThemeIndex = juce::jlimit(0, 10, index);
        switch (currentThemeIndex)
        {
            case 0: // Parawave RAPID (Default Signature Theme)
                chassisBg      = juce::Colour(0xFF11141B);
                panelDark      = juce::Colour(0xFF161922);
                panelElevated  = juce::Colour(0xFF1B1F2B);
                panelRecessed  = juce::Colour(0xFF0E1017);
                headerSilverTop= juce::Colour(0xFFD8DCE4);
                headerSilverMid= juce::Colour(0xFFC4C8D2);
                headerSilverBot= juce::Colour(0xFFB4B8C3);
                borderDark     = juce::Colour(0xFF0A0C12);
                borderSubtle   = juce::Colour(0xFF232838);
                borderBright   = juce::Colour(0xFF363E54);
                iceBlue        = juce::Colour(0xFF4FAAF7);
                goldOrange     = juce::Colour(0xFFE5A43B);
                pinkRed        = juce::Colour(0xFFF43F5E);
                greenLed       = juce::Colour(0xFF10B981);
                bgObsidian     = juce::Colour(0xFF11141B);
                bgCharcoal     = juce::Colour(0xFF161922);
                cardSurface    = juce::Colour(0xFF161922);
                cardHeader     = juce::Colour(0xFF1E2330);
                electricCyan   = juce::Colour(0xFF4FAAF7);
                deepMagenta    = juce::Colour(0xFFF43F5E);
                energeticAmber = juce::Colour(0xFFE5A43B);
                neonEmerald    = juce::Colour(0xFF10B981);
                textPrimary    = juce::Colour(0xFFE2E8F0);
                textSecondary  = juce::Colour(0xFF828BA0);
                textDim        = juce::Colour(0xFF4F576D);
                trackDark      = juce::Colour(0xFF0C0E14);
                break;

            case 1: // Valhalla Vintage Amber
                bgObsidian     = juce::Colour(0xFF120E0A);
                bgCharcoal     = juce::Colour(0xFF1C1510);
                cardSurface    = juce::Colour(0xFF19130E);
                cardHeader     = juce::Colour(0xFF241B14);
                borderSubtle   = juce::Colour(0xFF3A2C20);
                borderBright   = juce::Colour(0xFF543F2E);
                electricCyan   = juce::Colour(0xFFFF9E1B);
                deepMagenta    = juce::Colour(0xFFFF521B);
                energeticAmber = juce::Colour(0xFFFFD166);
                neonEmerald    = juce::Colour(0xFF8CE070);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFFFF3E0);
                textSecondary  = juce::Colour(0xFFC8A882);
                textDim        = juce::Colour(0xFF7A6248);
                trackDark      = juce::Colour(0xFF0C0906);
                break;

            case 2: // Obsidian Cyan
                bgObsidian     = juce::Colour(0xFF111111);
                bgCharcoal     = juce::Colour(0xFF1A1A1A);
                cardSurface    = juce::Colour(0xFF15171C);
                cardHeader     = juce::Colour(0xFF1C1F26);
                borderSubtle   = juce::Colour(0xFF282D38);
                borderBright   = juce::Colour(0xFF363D4D);
                electricCyan   = juce::Colour(0xFF00F0FF);
                deepMagenta    = juce::Colour(0xFFFF1E8E);
                energeticAmber = juce::Colour(0xFFFFB800);
                neonEmerald    = juce::Colour(0xFF00E599);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFF2F5F8);
                textSecondary  = juce::Colour(0xFF8C96A8);
                textDim        = juce::Colour(0xFF505868);
                trackDark      = juce::Colour(0xFF0C0D10);
                break;

            case 3: // Deep Crimson
                bgObsidian     = juce::Colour(0xFF11090C);
                bgCharcoal     = juce::Colour(0xFF1B0F14);
                cardSurface    = juce::Colour(0xFF180D12);
                cardHeader     = juce::Colour(0xFF24131B);
                borderSubtle   = juce::Colour(0xFF3A1E2B);
                borderBright   = juce::Colour(0xFF562B3E);
                electricCyan   = juce::Colour(0xFFFF1E4D);
                deepMagenta    = juce::Colour(0xFFB538FF);
                energeticAmber = juce::Colour(0xFFFF8833);
                neonEmerald    = juce::Colour(0xFF00F5B8);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFFDF0F4);
                textSecondary  = juce::Colour(0xFFB88C9A);
                textDim        = juce::Colour(0xFF6E4B58);
                trackDark      = juce::Colour(0xFF0A0507);
                break;

            case 4: // Matrix Green
                bgObsidian     = juce::Colour(0xFF08100B);
                bgCharcoal     = juce::Colour(0xFF0E1A12);
                cardSurface    = juce::Colour(0xFF0C1710);
                cardHeader     = juce::Colour(0xFF122318);
                borderSubtle   = juce::Colour(0xFF1E3A28);
                borderBright   = juce::Colour(0xFF2B543A);
                electricCyan   = juce::Colour(0xFF00FF66);
                deepMagenta    = juce::Colour(0xFF00D8FF);
                energeticAmber = juce::Colour(0xFFB8FF00);
                neonEmerald    = juce::Colour(0xFF00FFAA);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFE8FFF0);
                textSecondary  = juce::Colour(0xFF78B890);
                textDim        = juce::Colour(0xFF3E6B4E);
                trackDark      = juce::Colour(0xFF050A07);
                break;

            case 5: // Clean Light / Minimal
                bgObsidian     = juce::Colour(0xFFE4E8F0);
                bgCharcoal     = juce::Colour(0xFFEEF1F6);
                cardSurface    = juce::Colour(0xFFF8FAFC);
                cardHeader     = juce::Colour(0xFFEDF1F7);
                borderSubtle   = juce::Colour(0xFFCBD5E1);
                borderBright   = juce::Colour(0xFF94A3B8);
                electricCyan   = juce::Colour(0xFF0055FF);
                deepMagenta    = juce::Colour(0xFFE11D48);
                energeticAmber = juce::Colour(0xFFD97706);
                neonEmerald    = juce::Colour(0xFF059669);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFF0F172A);
                textSecondary  = juce::Colour(0xFF475569);
                textDim        = juce::Colour(0xFF94A3B8);
                trackDark      = juce::Colour(0xFFDCE3ED);
                break;

            case 6: // Cyberpunk Pink / Blue
                bgObsidian     = juce::Colour(0xFF0B0716);
                bgCharcoal     = juce::Colour(0xFF140D26);
                cardSurface    = juce::Colour(0xFF120B22);
                cardHeader     = juce::Colour(0xFF1C1132);
                borderSubtle   = juce::Colour(0xFF321E58);
                borderBright   = juce::Colour(0xFF4D2E86);
                electricCyan   = juce::Colour(0xFFFF00A0);
                deepMagenta    = juce::Colour(0xFF00E5FF);
                energeticAmber = juce::Colour(0xFFFFEE00);
                neonEmerald    = juce::Colour(0xFF00FF9D);
                iceBlue        = deepMagenta;
                goldOrange     = electricCyan;
                textPrimary    = juce::Colour(0xFFFFF0FA);
                textSecondary  = juce::Colour(0xFFB39DDB);
                textDim        = juce::Colour(0xFF67508C);
                trackDark      = juce::Colour(0xFF07040E);
                break;

            case 7: // Solarized Dark
                bgObsidian     = juce::Colour(0xFF001E26);
                bgCharcoal     = juce::Colour(0xFF002B36);
                cardSurface    = juce::Colour(0xFF073642);
                cardHeader     = juce::Colour(0xFF0A404E);
                borderSubtle   = juce::Colour(0xFF195463);
                borderBright   = juce::Colour(0xFF2AA198);
                electricCyan   = juce::Colour(0xFF2AA198);
                deepMagenta    = juce::Colour(0xFFD33682);
                energeticAmber = juce::Colour(0xFFB58900);
                neonEmerald    = juce::Colour(0xFF859900);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFFDF6E3);
                textSecondary  = juce::Colour(0xFF93A1A1);
                textDim        = juce::Colour(0xFF586E75);
                trackDark      = juce::Colour(0xFF00141A);
                break;

            case 8: // Monochrome OLED
                bgObsidian     = juce::Colour(0xFF000000);
                bgCharcoal     = juce::Colour(0xFF080808);
                cardSurface    = juce::Colour(0xFF0C0C0C);
                cardHeader     = juce::Colour(0xFF151515);
                borderSubtle   = juce::Colour(0xFF2A2A2A);
                borderBright   = juce::Colour(0xFF555555);
                electricCyan   = juce::Colour(0xFFFFFFFF);
                deepMagenta    = juce::Colour(0xFFB8B8B8);
                energeticAmber = juce::Colour(0xFFE0E0E0);
                neonEmerald    = juce::Colour(0xFFD0D0D0);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFFFFFFF);
                textSecondary  = juce::Colour(0xFF9E9E9E);
                textDim        = juce::Colour(0xFF525252);
                trackDark      = juce::Colour(0xFF030303);
                break;

            case 9: // Retro Synthwave Purple
                bgObsidian     = juce::Colour(0xFF120824);
                bgCharcoal     = juce::Colour(0xFF1D0E38);
                cardSurface    = juce::Colour(0xFF190C30);
                cardHeader     = juce::Colour(0xFF261347);
                borderSubtle   = juce::Colour(0xFF3E206E);
                borderBright   = juce::Colour(0xFF5E32A3);
                electricCyan   = juce::Colour(0xFFB84DFF);
                deepMagenta    = juce::Colour(0xFFFF2E93);
                energeticAmber = juce::Colour(0xFFFF9933);
                neonEmerald    = juce::Colour(0xFF00F0FF);
                iceBlue        = neonEmerald;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFF8EEFF);
                textSecondary  = juce::Colour(0xFFC299E8);
                textDim        = juce::Colour(0xFF734D99);
                trackDark      = juce::Colour(0xFF0B0417);
                break;

            case 10: // Nord Frost Blue
            default:
                bgObsidian     = juce::Colour(0xFF1D212A);
                bgCharcoal     = juce::Colour(0xFF242933);
                cardSurface    = juce::Colour(0xFF2E3440);
                cardHeader     = juce::Colour(0xFF3B4252);
                borderSubtle   = juce::Colour(0xFF434C5E);
                borderBright   = juce::Colour(0xFF4C566A);
                electricCyan   = juce::Colour(0xFF88C0D0);
                deepMagenta    = juce::Colour(0xFFB48EAD);
                energeticAmber = juce::Colour(0xFFEBCB8B);
                neonEmerald    = juce::Colour(0xFFA3BE8C);
                iceBlue        = electricCyan;
                goldOrange     = energeticAmber;
                textPrimary    = juce::Colour(0xFFECEFF4);
                textSecondary  = juce::Colour(0xFFD8DEE9);
                textDim        = juce::Colour(0xFF7B88A1);
                trackDark      = juce::Colour(0xFF181C24);
                break;
        }
    }
} // namespace Palette

class RapidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RapidLookAndFeel();
    ~RapidLookAndFeel() override = default;

    void refreshThemeColours();

    static juce::Colour getAccentForSlider(const juce::Slider& slider);

    // Rotary Knobs: Dark Inset Ring, Radial Metallic Cap, Center Needle & Colored Progress Arc
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    // Linear Faders: LED-Style Segmented Indicators & Metallic Handles
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Buttons: Dark Hardware Panels with Glowing LED Squares
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // ComboBoxes & Menus
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    juce::Font getComboBoxFont(juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;
};

// Typedef alias for backward compatibility
using PrismorLookAndFeel = RapidLookAndFeel;

} // namespace PrismorUI
