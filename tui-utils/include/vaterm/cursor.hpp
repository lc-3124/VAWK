#ifndef VATERM_CURSOR_HPP
#define VATERM_CURSOR_HPP

// -----------------------------------------------------------------------
//  vaterm::cursor — ANSI escape sequences for cursor manipulation
//
//  All functions return std::string containing the raw escape sequence.
//  They are stateless — no terminal state is cached.
//
//  Movement:
//    move_to(row, col)       — absolute positioning (1-based)
//    move(dir, steps)        — relative movement (UP/DOWN/LEFT/RIGHT)
//
//  Save / restore:
//    save() / restore()      — DEC private modes 7 (SCP / RCP)
//
//  Visibility & shape:
//    show() / hide()         — DECTCEM (?25h / ?25l)
//    shape(CursorShape)      — DECSCUSR (1..6, see enums.hpp)
//
//  Query:
//    report_position()       — CPR (\033[6n); terminal responds on stdin
// -----------------------------------------------------------------------

#include "enums.hpp"

#include <format>
#include <string>

namespace vaterm {
namespace cursor {

// Move cursor to an absolute (row, col) position (1-based, 1 = top-left).
inline std::string move_to(uint16_t row, uint16_t col) {
    return std::format("\033[{};{}H", row, col);
}

// Move cursor relative to its current position.
// dir   — CursorDir::UP, DOWN, LEFT, or RIGHT
// steps — number of cells to move
inline std::string move(CursorDir dir, uint16_t steps) {
    constexpr const char* seqs[] = {"A", "B", "C", "D"};
    return std::format("\033[{}{}", steps, seqs[static_cast<uint8_t>(dir)]);
}

// Save the current cursor position (DEC private mode 7, SCO sequence).
// Restore with restore().
inline std::string save() {
    return "\0337";
}

// Restore the cursor position saved by the most recent save().
inline std::string restore() {
    return "\0338";
}

// Make the cursor visible (DECTCEM).
inline std::string show() {
    return "\033[?25h";
}

// Hide the cursor (DECTCEM).
inline std::string hide() {
    return "\033[?25l";
}

// Change the cursor shape (DECSCUSR). Supported in xterm and most
// modern terminal emulators. See CursorShape in enums.hpp for the
// available shapes (blinking/steady block, underline, bar).
inline std::string shape(CursorShape s) {
    return std::format("\033[{} q", static_cast<uint8_t>(s));
}

// Request the terminal to report the cursor position (CPR).
// The terminal responds with "\033[row;colR" on stdin.
// Use terminal::read_byte() to read the response.
inline std::string report_position() {
    return "\033[6n";
}

} // namespace cursor
} // namespace vaterm

#endif // VATERM_CURSOR_HPP
