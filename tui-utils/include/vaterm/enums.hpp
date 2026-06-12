#ifndef VATERM_ENUMS_HPP
#define VATERM_ENUMS_HPP

#include <cstdint>

namespace vaterm {

/// Cursor movement direction
enum class CursorDir : uint8_t {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};

/// Cursor shape (DECSCUSR)
enum class CursorShape : uint8_t {
    BLINKING_BLOCK      = 1,
    STEADY_BLOCK        = 2,
    BLINKING_UNDERLINE  = 3,
    STEADY_UNDERLINE    = 4,
    BLINKING_BAR        = 5,
    STEADY_BAR          = 6,
};

/// ANSI text effects (SGR parameters)
enum class TextEffect : uint8_t {
    RESET           = 0,
    BOLD            = 1,
    DIM             = 2,
    ITALIC          = 3,
    UNDERLINE       = 4,
    SLOW_BLINK      = 5,
    RAPID_BLINK     = 6,
    REVERSE         = 7,
    CONCEAL         = 8,
    STRIKETHROUGH   = 9,
};

/// 4-bit color palette (standard ANSI colors)
enum class Color4 : uint8_t {
    BLACK           = 0,
    RED             = 1,
    GREEN           = 2,
    YELLOW          = 3,
    BLUE            = 4,
    MAGENTA         = 5,
    CYAN            = 6,
    WHITE           = 7,
    BRIGHT_BLACK    = 8,
    BRIGHT_RED      = 9,
    BRIGHT_GREEN    = 10,
    BRIGHT_YELLOW   = 11,
    BRIGHT_BLUE     = 12,
    BRIGHT_MAGENTA  = 13,
    BRIGHT_CYAN     = 14,
    BRIGHT_WHITE    = 15,
};

/// An RGB color triplet
struct Rgb {
    uint8_t r, g, b;
};

} // namespace vaterm

#endif // VATERM_ENUMS_HPP

