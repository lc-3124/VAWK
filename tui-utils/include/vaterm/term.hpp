#ifndef VATERM_TERM_HPP
#define VATERM_TERM_HPP

// -----------------------------------------------------------------------
//  vaterm::terminal — terminal mode switching, screen control, I/O
//
//  The terminal class provides:
//
//    1. RAII raw mode:      enter_raw() / ~terminal()  automatically
//                           restores the original termios on destruction.
//    2. Screen clear:        clear_screen(), clear_to_eol(),
//                            clear_to_eos(), clear_line().
//    3. Terminal query:      size(rows, cols), type().
//    4. I/O:                 write(), flush(), read_byte(), getch().
//
//  All static methods (clear_*, size, type, write, flush, read_byte,
//  getch) are thread-safe and require no instance.  The instance is
//  only needed for raw-mode management.
//
//  Example:
//    terminal t;
//    t.enter_raw();
//    terminal::write(terminal::clear_screen());
//    terminal::write("Hello under raw mode!\n");
//    // ~terminal() restores the original settings automatically.
// -----------------------------------------------------------------------

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "cursor.hpp"

namespace vaterm {

class terminal {
  public:
    terminal() = default;
    ~terminal();

    // Non-copyable — owns the raw-mode state.
    terminal(const terminal&)            = delete;
    terminal& operator=(const terminal&) = delete;

    // ---- Screen-clear sequences (static) --------------------------------

    // Clear the entire screen and move the cursor to (1,1).
    static std::string clear_screen() {
        return "\033[2J\033[H";
    }

    // Clear from cursor to end of line (EL 0).
    static std::string clear_to_eol() {
        return "\033[K";
    }

    // Clear from cursor to end of screen (ED 0).
    static std::string clear_to_eos() {
        return "\033[J";
    }

    // Clear the entire current line and return the cursor to column 1.
    static std::string clear_line() {
        return "\033[2K\r";
    }

    // ---- Terminal queries (static) --------------------------------------

    // Query terminal size via the TIOCGWINSZ ioctl.
    // Returns true on success, false on failure (e.g. not a tty).
    // On success rows and cols are filled with the detected dimensions.
    static bool size(uint16_t& rows, uint16_t& cols) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
            return false;
        rows = ws.ws_row;
        cols = ws.ws_col;
        return true;
    }

    // Convenience wrappers around size() that return the value directly.
    // Returns 0 on error (e.g. not a tty).
    static int getColMax() {
        uint16_t r = 0, c = 0;
        return size(r, c) ? static_cast<int>(c) : 0;
    }
    static int getRowMax() {
        uint16_t r = 0, c = 0;
        return size(r, c) ? static_cast<int>(r) : 0;
    }

    // Return the value of the $TERM environment variable,
    // or an empty string if it is unset.
    static std::string type() {
        auto* t = getenv("TERM");
        return t ? std::string(t) : std::string();
    }

    // Detect the terminal's maximum supported colour depth by inspecting
    // $COLORTERM and $TERM.  The result is cached after the first call.
    //
    // Returns:
    //   C24 — truecolor / 24-bit (e.g. $COLORTERM=truecolor)
    //   C8  — 256-colour (e.g. $TERM=xterm-256color)
    //   C4  — 16-colour / otherwise unknown
    static ColorDepth detect_color_depth() {
        static ColorDepth depth = [] {
            auto* ct = getenv("COLORTERM");
            if (ct) {
                std::string_view sv(ct);
                if (sv == "truecolor" || sv == "24bit")
                    return ColorDepth::C24;
            }
            auto* term = getenv("TERM");
            if (term) {
                std::string_view sv(term);
                if (sv.ends_with("-256color"))
                    return ColorDepth::C8;
                // Some modern terminals that always support truecolor
                if (sv == "xterm-kitty" || sv == "alacritty" || sv == "foot" || sv == "foot-extra")
                    return ColorDepth::C24;
            }
            return ColorDepth::C4;
        }();
        return depth;
    }

    // ---- Raw mode (instance methods) ------------------------------------

    // Switch stdin to raw mode using cfmakeraw(3).
    //
    // Raw mode disables line buffering, signal keys (^C, ^Z),
    // and echo. After enter_raw(), every key-press is immediately
    // available via read().
    //
    // VMIN = 0, VTIME = 1 gives a non-blocking read with a 100 ms
    // timeout — read_byte() can return -1 if no data is pending.
    //
    // Returns true on success, false if stdin is not a tty or if
    // tcgetattr/tcsetattr fails.
    bool enter_raw() {
        if (raw_) return true;
        if (!isatty(STDIN_FILENO)) return false;
        if (tcgetattr(STDIN_FILENO, &original_) == -1) return false;
        struct termios raw = original_;
        cfmakeraw(&raw);
        raw.c_oflag &= ~(OPOST);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 1;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return false;
        raw_ = true;
        return true;
    }

    // Restore the original terminal settings captured by enter_raw().
    // Called automatically by the destructor.
    void exit_raw() {
        if (!raw_) return;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
        raw_ = false;
    }

    // Check whether raw mode is currently active.
    bool is_raw() const { return raw_; }

    // ---- I/O (static) ---------------------------------------------------

    // Write a string to stdout.
    static void write(std::string_view s) {
        ::write(STDOUT_FILENO, s.data(), s.size());
    }

    // Flush stdout (fsync). Use after write() to ensure the terminal
    // receives the data immediately.
    static void flush() {
        fsync(STDOUT_FILENO);
    }

    // Read one byte from stdin without blocking.
    // In raw mode (VMIN=0, VTIME=1) this returns nullopt after 100 ms if
    // no data is available.
    static std::optional<unsigned char> read_byte() {
        unsigned char c;
        auto n = ::read(STDIN_FILENO, &c, 1);
        if (n <= 0) return std::nullopt;
        return c;
    }

    // Blocking version of read_byte().
    // Loops until a byte arrives, then returns it as a char.
    static char getch() {
        char c;
        while (::read(STDIN_FILENO, &c, 1) <= 0)
            ;
        return c;
    }

  private:
    bool        raw_ = false;       // whether raw mode is active
    struct termios original_;       // saved termios before raw mode
};

inline terminal::~terminal() {
    if (raw_) {
        write(cursor::show());
        write(clear_screen());
        flush();
    }
    exit_raw();
}

} // namespace vaterm

#endif // VATERM_TERM_HPP
