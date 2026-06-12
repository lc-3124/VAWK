#ifndef VATERM_HPP
#define VATERM_HPP

/// vaterm — Terminal UI Toolkit for Linux
///
/// A collection of stateless, thread-safe utilities for building TUI
/// applications on Linux.  Each module lives in its own header under
/// `vaterm/` and can be included independently.
///
/// Modules:
///   vaterm::color    — ANSI escape sequences for colours and effects
///   vaterm::cursor   — ANSI escape sequences for cursor manipulation
///   vaterm::Terminal — terminal mode switching, screen control, I/O
///   vaterm::sys      — system information helpers
///   vaterm::utf      — UTF-8 string utilities
///
/// Example:
///   #include <vaterm.hpp>
///   #include <cstdio>
///
///   int main() {
///       printf("%sHello, TUI!%s\n",
///              vaterm::color::fg(vaterm::Color4::RED).c_str(),
///              vaterm::color::reset().c_str());
///       return 0;
///   }

#include "vaterm/enums.hpp"
#include "vaterm/color.hpp"
#include "vaterm/cursor.hpp"
#include "vaterm/term.hpp"
#include "vaterm/system.hpp"
#include "vaterm/utf.hpp"

#endif // VATERM_HPP

