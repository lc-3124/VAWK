#ifndef VATERM_CURSOR_HPP
#define VATERM_CURSOR_HPP

#include "enums.hpp"

#include <string>

namespace vaterm {

/// Build ANSI escape sequences for cursor manipulation.
///
/// All functions return the escape sequence as a string; none write to the
/// terminal directly. This makes the module stateless and thread-safe.
namespace cursor {

/// Move cursor to an absolute (row, col) position (1-based).
std::string move_to(uint16_t row, uint16_t col);

/// Move cursor by offset in a given direction.
std::string move(CursorDir dir, uint16_t steps = 1);

/// Save current cursor position.
std::string save();

/// Restore cursor to the last saved position.
std::string restore();

/// Show the cursor.
std::string show();

/// Hide the cursor.
std::string hide();

/// Set the cursor shape (DECSCUSR).
std::string shape(CursorShape s);

/// Report cursor position — returns the escape sequence that causes the
/// terminal to reply with `\033[row;colR`.
std::string report_position();

} // namespace cursor
} // namespace vaterm

#endif // VATERM_CURSOR_HPP

