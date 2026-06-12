#ifndef VATERM_COLOR_HPP
#define VATERM_COLOR_HPP

#include "enums.hpp"

#include <string>
#include <string_view>

namespace vaterm {

/// Build ANSI escape sequences and manage terminal colors.
///
/// All functions return the escape sequence as a string; none write to the
/// terminal directly. This makes the module stateless and thread-safe.
namespace color {

// ── Foreground ──────────────────────────────────────────────────────────────

/// Set foreground color using 4-bit palette.
std::string fg(Color4 c);

/// Set foreground color using 8-bit (256-color) palette.
std::string fg(uint8_t c);

/// Set foreground color using 24-bit RGB.
std::string fg(uint8_t r, uint8_t g, uint8_t b);

/// Set foreground color from an Rgb struct.
inline std::string fg(Rgb rgb) { return fg(rgb.r, rgb.g, rgb.b); }

// ── Background ──────────────────────────────────────────────────────────────

/// Set background color using 4-bit palette.
std::string bg(Color4 c);

/// Set background color using 8-bit palette.
std::string bg(uint8_t c);

/// Set background color using 24-bit RGB.
std::string bg(uint8_t r, uint8_t g, uint8_t b);

/// Set background color from an Rgb struct.
inline std::string bg(Rgb rgb) { return bg(rgb.r, rgb.g, rgb.b); }

// ── Text effects ────────────────────────────────────────────────────────────

/// Apply a single text effect.
std::string effect(TextEffect e);

/// Reset all text attributes (color + effects).
std::string reset();

/// Apply multiple effects at once.
std::string effect(std::initializer_list<TextEffect> effects);

// ── Color conversion utilities ──────────────────────────────────────────────

/// Convert 24-bit RGB to the nearest 8-bit (216-color cube + grayscale) value.
uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b);

/// Convert 8-bit ANSI color back to an approximate RGB value.
Rgb     _256_to_rgb(uint8_t c);

/// Convert a 4-bit color value (0-15) to the closest 8-bit value.
uint8_t _4_to_256(Color4 c);

/// Blend two 8-bit colors by averaging their approximate RGB values.
uint8_t blend_256(uint8_t a, uint8_t b);

/// Invert an 8-bit color.
uint8_t invert_256(uint8_t c);

} // namespace color
} // namespace vaterm

#endif // VATERM_COLOR_HPP

