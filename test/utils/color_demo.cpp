// -----------------------------------------------------------------------
//  color_demo — demonstrates every colour API function in vaterm::color
//
//  This demo exercises:
//    - 4-bit foreground (Color4 palette, 16 colours)
//    - 8-bit foreground (256-colour cube)
//    - 24-bit true-color foreground
//    - Background colours (4-bit, 8-bit, 24-bit)
//    - Single and combined text effects
//    - Colour-space conversion (rgb_to_256, _256_to_rgb, etc.)
//    - Greyscale ramp and round-trip conversion
//
//  Run:  ./color_demo | less -R
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <string>
#include <string_view>

using namespace vaterm;

// Print a section header with bold+underline.
static void section(const std::string& title) {
    printf("\n%s═══ %s ═══%s\n",
           color::effect({TextEffect::BOLD, TextEffect::UNDERLINE}).c_str(),
           title.c_str(),
           color::reset().c_str());
}

// Show all 16 4-bit foreground colours.
static void demo_fg_4bit() {
    section("4-bit Foreground");
    for (int i = 0; i < 16; ++i) {
        printf("%s %02d %s", color::fg(static_cast<Color4>(i)).c_str(), i, color::reset().c_str());
        if ((i + 1) % 8 == 0) printf("\n");
    }
}

// Show the full 256-colour foreground cube.
static void demo_fg_8bit() {
    section("8-bit (256-color) Foreground");
    for (int i = 0; i < 256; ++i) {
        printf("%s%3d%s", color::fg(static_cast<uint8_t>(i)).c_str(), i, color::reset().c_str());
        if ((i + 1) % 16 == 0) printf("\n");
    }
}

// Show a true-color gradient: vary R and G at 32-step intervals,
// keeping B fixed at 128.
static void demo_fg_24bit() {
    section("24-bit True Color Foreground");
    for (int r = 0; r < 256; r += 32) {
        for (int g = 0; g < 256; g += 32) {
            printf("%s██%s", color::fg(r, g, 128).c_str(), color::reset().c_str());
        }
        printf("\n");
    }
}

// Demonstrate background colours (4-bit, 8-bit, 24-bit).
static void demo_bg() {
    section("Background Colors");
    for (int i = 0; i < 16; ++i) {
        printf("%s %02d %s", color::bg(static_cast<Color4>(i)).c_str(), i, color::reset().c_str());
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("\n");
    for (int i = 0; i < 256; i += 16) {
        printf("%s %3d %s", color::bg(static_cast<uint8_t>(i)).c_str(), i, color::reset().c_str());
    }
    printf("\n");
    printf("%s  TrueColor BG  %s\n", color::bg(100, 150, 200).c_str(), color::reset().c_str());
}

// Display the effect of each single SGR text effect.
static void demo_effects() {
    section("Text Effects");
    auto effects = {
        TextEffect::BOLD, TextEffect::DIM, TextEffect::ITALIC,
        TextEffect::UNDERLINE, TextEffect::SLOW_BLINK,
        TextEffect::REVERSE, TextEffect::CONCEAL, TextEffect::STRIKETHROUGH,
    };
    for (auto e : effects) {
        printf("%s%-15s%s\n", color::effect(e).c_str(),
               color::effect(e).c_str(), color::reset().c_str());
    }
}

// Show combined effects (e.g. bold + underline).
static void demo_multi_effects() {
    section("Multiple Effects Combined");
    printf("%sCombined Bold+Underline%s\n",
           color::effect({TextEffect::BOLD, TextEffect::UNDERLINE}).c_str(),
           color::reset().c_str());
    printf("%sBold+Italic+Reverse%s\n",
           color::effect({TextEffect::BOLD, TextEffect::ITALIC, TextEffect::REVERSE}).c_str(),
           color::reset().c_str());
}

// Exercise the colour-space conversion helpers.
static void demo_convert() {
    section("Color Space Conversion");

    // Round-trip: RGB → 256 → RGB.
    auto rgb = Rgb{200, 100, 50};
    auto idx = color::rgb_to_256(rgb.r, rgb.g, rgb.b);
    auto back = color::_256_to_rgb(idx);
    printf("rgb_to_256(%d,%d,%d)  → %3d  → rgb(%d,%d,%d)\n",
           rgb.r, rgb.g, rgb.b, idx, back.r, back.g, back.b);

    // 4-bit → 256.
    auto c4 = Color4::YELLOW;
    auto c256 = color::_4_to_256(c4);
    printf("_4_to_256(YELLOW) → %d\n", c256);

    // Blend two 256-colour indices.
    auto blended = color::blend_256(196, 21);
    printf("blend_256(RED=%d, BLUE=%d) → %d\n", 196, 21, blended);

    // Invert a colour.
    auto inverted = color::invert_256(color::_4_to_256(Color4::RED));
    printf("invert_256(RED=%d) → %d\n", color::_4_to_256(Color4::RED), inverted);
    auto invRgb = color::_256_to_rgb(inverted);
    printf("  (approx rgb(%d,%d,%d))\n", invRgb.r, invRgb.g, invRgb.b);
}

// Show the greyscale ramp (256-colour indices 232-255) and conversion.
static void demo_gray_ramp() {
    section("Gray Ramp (232–255)");
    for (int i = 232; i <= 255; ++i) {
        printf("%s %3d %s", color::bg(static_cast<uint8_t>(i)).c_str(), i, color::reset().c_str());
    }
    printf("\n");
    for (int v = 0; v <= 255; v += 16) {
        auto idx = color::rgb_to_256(v, v, v);
        printf("rgb(%d,%d,%d) → 256(%d)\n", v, v, v, idx);
    }
}

int main() {
    printf("%s=== vaterm color API Demo ===%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());
    printf("(header-only: vaterm/color.hpp, vaterm/enums.hpp)\n");

    demo_fg_4bit();
    demo_fg_8bit();
    demo_fg_24bit();
    demo_bg();
    demo_effects();
    demo_multi_effects();
    demo_convert();
    demo_gray_ramp();

    printf("\n%sAll color API tests completed.%s\n",
           color::fg(Color4::GREEN).c_str(), color::reset().c_str());
    return 0;
}
