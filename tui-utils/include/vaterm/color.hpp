#ifndef VATERM_COLOR_HPP
#define VATERM_COLOR_HPP

// -----------------------------------------------------------------------
//  vaterm::color — ANSI escape sequences for colours and text effects
//
//  All functions return std::string containing the raw escape sequence.
//  The user is responsible for writing the string to the terminal and
//  for sending color::reset() (or a RESET effect) afterwards.
//
//  Colour overrides:
//    fg(bg)(Color4)    — 4-bit (16-colour palette)
//    fg(bg)(uint8_t)   — 8-bit (256-colour cube + greyscale 0-255)
//    fg(bg)(r, g, b)   — 24-bit true colour
//    fg(bg)(Rgb)       — convenience overload taking a struct
//
//  Text effects:
//    effect(TextEffect)           — single effect
//    effect({a, b, ...})          — multiple effects combined with ';'
//    reset()                      — reset everything
//
//  Colour-space conversion:
//    rgb_to_256(r, g, b)   — approximate 24-bit -> 256-colour index
//    _256_to_rgb(idx)      — reverse lookup (returns the canonical RGB)
//    _4_to_256(c4)         — map a 4-bit colour to its 256-index
//    blend_256(a, b)       — average two 256-colour indices
//    invert_256(idx)       — approximate inverted colour
// -----------------------------------------------------------------------

#include "enums.hpp"

#include <format>
#include <string>

namespace vaterm {
namespace color {

// Foreground — 4-bit palette (Color4).
// Values 0-7 use the 30-37 range, 8-15 use the 90-97 "bright" range.
inline std::string fg(Color4 c) {
    auto v = static_cast<uint8_t>(c);
    if (v < 8)
        return std::format("\033[{}m", v + 30);
    return std::format("\033[{}m", v + 82);
}

// Foreground — 8-bit (256-colour). Parameter is the colour index (0-255).
inline std::string fg(uint8_t c) {
    return std::format("\033[38;5;{}m", c);
}

// Foreground — 24-bit true colour using separate R, G, B components.
inline std::string fg(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("\033[38;2;{};{};{}m", r, g, b);
}

// Foreground — convenience overload accepting an Rgb struct.
inline std::string fg(Rgb rgb) { return fg(rgb.r, rgb.g, rgb.b); }

// Background — 4-bit palette (Color4).
// Values 0-7 use the 40-47 range, 8-15 use the 100-107 "bright" range.
inline std::string bg(Color4 c) {
    auto v = static_cast<uint8_t>(c);
    if (v < 8)
        return std::format("\033[{}m", v + 40);
    return std::format("\033[{}m", v + 92);
}

// Background — 8-bit (256-colour). Parameter is the colour index (0-255).
inline std::string bg(uint8_t c) {
    return std::format("\033[48;5;{}m", c);
}

// Background — 24-bit true colour using separate R, G, B components.
inline std::string bg(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("\033[48;2;{};{};{}m", r, g, b);
}

// Background — convenience overload accepting an Rgb struct.
inline std::string bg(Rgb rgb) { return bg(rgb.r, rgb.g, rgb.b); }

// Single SGR text effect (e.g. BOLD, UNDERLINE).
inline std::string effect(TextEffect e) {
    return std::format("\033[{}m", static_cast<uint8_t>(e));
}

// Reset all colours and effects to terminal defaults.
inline std::string reset() {
    return "\033[0m";
}

// Multiple SGR effects combined into one escape sequence.
// Example: effect({TextEffect::BOLD, TextEffect::UNDERLINE})
// produces "\033[1;4m".
inline std::string effect(std::initializer_list<TextEffect> effects) {
    std::string out = "\033[";
    bool first = true;
    for (auto e : effects) {
        if (!first) out += ';';
        out += std::to_string(static_cast<uint8_t>(e));
        first = false;
    }
    out += 'm';
    return out;
}

// ---- Colour-space conversion helpers -----------------------------------

// Approximate a 24-bit (R,G,B) colour to the nearest 256-colour index.
// Uses the standard 6×6×6 cube (indices 16-231) plus the grey ramp
// (indices 232-255) and the 16 base ANSI colours (indices 0-15).
inline uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b) {
    // Greyscale shortcut: if R=G=B, map straight into the greyscale ramp.
    if (r == g && g == b) {
        if (r < 8) return 16;
        if (r > 238) return 231;
        return static_cast<uint8_t>(232 + (r - 8) / 10);
    }
    // Map each component from 0-255 to 0-5.
    auto ir = static_cast<uint8_t>((r * 5) / 255);
    auto ig = static_cast<uint8_t>((g * 5) / 255);
    auto ib = static_cast<uint8_t>((b * 5) / 255);
    return static_cast<uint8_t>(16 + ir * 36 + ig * 6 + ib);
}

// Reverse lookup: return the canonical RGB colour for a 256-colour index.
// For indices 0-15 the ANSI named colours are used; for 16-231 the
// 6×6×6 cube is unpacked; for 232-255 the grey ramp values are computed.
inline Rgb _256_to_rgb(uint8_t c) {
    // Indices 0-15: standard ANSI colours (approximate sRGB values).
    if (c < 16) {
        static constexpr uint8_t table[16][3] = {
            {0, 0, 0}, {128, 0, 0}, {0, 128, 0}, {128, 128, 0},
            {0, 0, 128}, {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
            {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
            {0, 0, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255},
        };
        return {table[c][0], table[c][1], table[c][2]};
    }
    // Indices 232-255: 24-step greyscale ramp.
    if (c >= 232) {
        auto v = static_cast<uint8_t>(8 + (c - 232) * 10);
        return {v, v, v};
    }
    // Indices 16-231: 6×6×6 colour cube.
    auto idx = c - 16;
    auto r = static_cast<uint8_t>((idx / 36) * 51);
    auto g = static_cast<uint8_t>(((idx % 36) / 6) * 51);
    auto b = static_cast<uint8_t>((idx % 6) * 51);
    return {r, g, b};
}

// Map a 4-bit Color4 to its equivalent 256-colour index (identity for
// indices 0-15).
inline uint8_t _4_to_256(Color4 c) {
    static constexpr uint8_t table[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
    };
    return table[static_cast<uint8_t>(c)];
}

// Blend two 256-colour indices by averaging their RGB values.
inline uint8_t blend_256(uint8_t a, uint8_t b) {
    auto ra = _256_to_rgb(a);
    auto rb = _256_to_rgb(b);
    return rgb_to_256(
        static_cast<uint8_t>((static_cast<uint16_t>(ra.r) + rb.r) / 2),
        static_cast<uint8_t>((static_cast<uint16_t>(ra.g) + rb.g) / 2),
        static_cast<uint8_t>((static_cast<uint16_t>(ra.b) + rb.b) / 2)
    );
}

// Approximate the inverted colour of a 256-colour index.
inline uint8_t invert_256(uint8_t c) {
    auto rgb = _256_to_rgb(c);
    return rgb_to_256(
        static_cast<uint8_t>(255 - rgb.r),
        static_cast<uint8_t>(255 - rgb.g),
        static_cast<uint8_t>(255 - rgb.b)
    );
}

// Find the nearest 4-bit ANSI colour for a 24-bit RGB value.
inline Color4 nearest_4bit(uint8_t r, uint8_t g, uint8_t b) {
    static constexpr uint8_t table[16][3] = {
        {0, 0, 0}, {128, 0, 0}, {0, 128, 0}, {128, 128, 0},
        {0, 0, 128}, {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
        {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
        {0, 0, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255},
    };
    int best = 0;
    int best_dist = INT32_MAX;
    for (int i = 0; i < 16; ++i) {
        int dr = r - table[i][0];
        int dg = g - table[i][1];
        int db = b - table[i][2];
        int dist = dr * dr + dg * dg + db * db;
        if (dist < best_dist) { best_dist = dist; best = i; }
    }
    return static_cast<Color4>(best);
}

inline Color4 nearest_4bit(Rgb rgb) {
    return nearest_4bit(rgb.r, rgb.g, rgb.b);
}

} // namespace color
} // namespace vaterm

#endif // VATERM_COLOR_HPP
