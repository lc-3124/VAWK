#ifndef VATERM_ENUMS_HPP
#define VATERM_ENUMS_HPP

// -----------------------------------------------------------------------
//  vaterm::enums — shared enumerations and simple data types
//
//  Provides the common type definitions used throughout the vaterm
//  library. All values correspond to standard ANSI / ECMA-48 parameters.
// -----------------------------------------------------------------------

#include <cstdint>

namespace vaterm {

// ---- Terminal color-depth detection ------------------------------------
//  Ordered from lowest to highest so the enum values can be compared:
//    C4 < C8 < C24
enum class ColorDepth : uint8_t {
    C4  = 4,
    C8  = 8,
    C24 = 24,
};

// ---- Cursor direction -------------------------------------------------
//  Used by cursor::move() to specify which way to shift the cursor.

enum class CursorDir : uint8_t {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};

// ---- Cursor shapes (DECSCUSR) -----------------------------------------
//  VTxxx cursor-style codes. Supported in xterm, kitty, foot, and most
//  modern terminals. Values 1-6 correspond to the DECSCUSR parameter.

enum class CursorShape : uint8_t {
    BLINKING_BLOCK      = 1,
    STEADY_BLOCK        = 2,
    BLINKING_UNDERLINE  = 3,
    STEADY_UNDERLINE    = 4,
    BLINKING_BAR        = 5,
    STEADY_BAR          = 6,
};

// ---- ANSI text effects (SGR parameters) -------------------------------
//  Select Graphic Rendition parameters. Not all terminals support every
//  effect; RAPID_BLINK in particular is often ignored.

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

// ---- 4-bit ANSI color palette -----------------------------------------
//  The sixteen standard terminal colors. Values 0-7 are the "dark" set;
//  values 8-15 are the corresponding "bright" variants.

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

// ---- RGB color triplet ------------------------------------------------
//  Used by the 24-bit true-color API and the color-space conversion
//  helpers (rgb_to_256, _256_to_rgb, etc.).

struct Rgb {
    uint8_t r, g, b;
};

// ---- 8-bit (256-colour) index wrapper ----------------------------------
//  Explicit wrapper to distinguish 8-bit color from a raw uint8_t in
//  overload resolution.
struct Color8 {
    uint8_t index;
};

// ---- Transparent colour tag --------------------------------------------
//  When used as fg/bg in a Style initialiser, produces an empty SGR
//  string.  On importFrame this means "skip this plane" (no fg / no bg).
//  On output the terminal renders its default colour (or the underlying
//  destination content).
struct Trans {};
inline constexpr Trans TRANS{};

} // namespace vaterm

#endif // VATERM_ENUMS_HPP
