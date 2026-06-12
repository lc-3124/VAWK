#ifndef VATERM_TERM_HPP
#define VATERM_TERM_HPP

#include <cstdint>
#include <string>
#include <termios.h>

namespace vaterm {

/// Terminal-level operations: mode switching, screen control, I/O.
///
/// Unlike the other modules, term has state (the saved terminal attributes)
/// and therefore exposes a class rather than a namespace of free functions.
class Terminal {
  public:
    Terminal() = default;
    ~Terminal();

    Terminal(const Terminal&)            = delete;
    Terminal& operator=(const Terminal&) = delete;

    // ── Screen control ────────────────────────────────────────────────────

    /// Clear the entire screen and home the cursor.
    static std::string clear_screen();

    /// Clear from cursor to end of line.
    static std::string clear_to_eol();

    /// Clear from cursor to end of screen.
    static std::string clear_to_eos();

    /// Clear the entire line the cursor is on.
    static std::string clear_line();

    // ── Terminal info ─────────────────────────────────────────────────────

    /// Get terminal size in rows and columns.
    static bool size(uint16_t& rows, uint16_t& cols);

    /// Get the value of $TERM.
    static std::string type();

    // ── Raw mode (RAII) ───────────────────────────────────────────────────

    /// Enter raw mode (non-canonical input, no echo, etc.).
    /// Returns true on success.
    bool enter_raw();

    /// Restore the original terminal settings.
    void exit_raw();

    /// Check whether raw mode is currently active.
    bool is_raw() const { return raw_; }

    // ── Low-level I/O helpers ─────────────────────────────────────────────

    /// Write a string_view to stdout via write(2).
    static void write(std::string_view s);

    /// Flush stdout.
    static void flush();

    /// Read a single byte from stdin without buffering.
    /// Returns -1 if no data is available (non-blocking).
    static int read_byte();

  private:
    bool        raw_ = false;
    struct termios original_;
};

} // namespace vaterm

#endif // VATERM_TERM_HPP

