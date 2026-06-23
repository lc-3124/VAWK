#ifndef VATERM_HPP
#define VATERM_HPP

// -----------------------------------------------------------------------
//  vaterm — header-only terminal UI toolkit for Linux (C++23)
//
//  A collection of stateless, thread-safe utilities for building TUI
//  applications. Each module lives in its own header under vaterm/ and
//  can be included independently.
//
//  Modules:
//    vaterm::color    — ANSI escape sequences for foreground / background
//                       colors and text effects (4-bit, 8-bit, 24-bit).
//    vaterm::cursor   — ANSI escape sequences for cursor movement,
//                       shape switching, save/restore, show/hide.
//    vaterm::mouse    — SGR-encoded mouse event tracking: enable/disable
//                       the terminal protocol, parse raw sequences into
//                       a structured MouseState (button, action, position).
//    vaterm::terminal — RAII terminal raw mode, clear-screen sequences,
//                       terminal-size query, blocking / non-blocking I/O.
//    vaterm::sys      — System-information helpers (user name, host name,
//                       environment variables, current working directory,
//                       command execution via popen).
//    vaterm::utf      — UTF-8 string utilities (byte length of leading
//                       byte, codepoint count, substring by codepoint
//                       index, display width with CJK/emoji awareness).
//
//  Example:
//    #include <vaterm.hpp>
//    #include <cstdio>
//
//    int main() {
//        printf("%sHello, TUI!%s\n",
//               vaterm::color::fg(vaterm::Color4::RED).c_str(),
//               vaterm::color::reset().c_str());
//        return 0;
//    }
// -----------------------------------------------------------------------

#include "vaterm/enums.hpp"
#include "vaterm/color.hpp"
#include "vaterm/cursor.hpp"
#include "vaterm/mouse.hpp"
#include "vaterm/term.hpp"
#include "vaterm/system.hpp"
#include "vaterm/utf.hpp"

#endif // VATERM_HPP
