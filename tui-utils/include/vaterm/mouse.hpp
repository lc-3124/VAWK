#ifndef VATERM_MOUSE_HPP
#define VATERM_MOUSE_HPP

// -----------------------------------------------------------------------
//  vaterm::mouse — SGR-encoded mouse event parsing and tracking
//
//  The terminal (xterm, foot, kitty, etc.) can be told to send encoded
//  mouse events on stdin by writing the enable() sequence. Each event
//  arrives as \033[<Cb;Cx;RyM (press/drag) or \033[<Cb;Cx;Rym (release)
//  where Cb encodes button+modifiers+action-type, and (Cx,Ry) are the
//  1-based column and row.
//
//  API overview:
//    mouse::enable() / disable()   — send/release the mouse-trapping
//                                     escape sequences (?1000, ?1002,
//                                     ?1006 — SGR extended mode).
//    mouse::available()            — check whether stdin/stdout are ttys.
//    mouse::parse(seq)             — static parser: raw string -> MouseState.
//    mouse::capture()              — instance method: block and parse
//                                     the next complete mouse event.
//
//  The instance (mouse) maintains an internal buffer for partial reads so
//  that capture() can accumulate raw bytes until a full SGR sequence is
//  recognised.
//
//  For applications that also need to handle keyboard input alongside
//  mouse events, use poll() or select() on STDIN_FILENO and feed bytes
//  into mouse::parse() manually — see mouse_demo.cpp for an example.
// -----------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>

namespace vaterm {

// Structured representation of a parsed SGR mouse event.
struct MouseState {
    int row = 0;   // 1-based row
    int col = 0;   // 1-based column

    // Which physical button (or none for scroll events).
    enum class Button : uint8_t { NONE, LEFT, MIDDLE, RIGHT };
    // What kind of action occurred.
    enum class Action : uint8_t { RELEASE, PRESS, DRAG, SCROLL_UP, SCROLL_DOWN };
    Button button = Button::NONE;
    Action action = Action::RELEASE;
};

// SGR mouse event tracker.
//
// Use an instance when reading events with capture(). The static
// methods (enable, disable, available, parse) can be used without
// creating an instance.
class mouse {
  public:
    mouse() = default;

    // Non-copyable — the internal buffer buf_ holds partial state.
    mouse(const mouse&) = delete;
    mouse& operator=(const mouse&) = delete;

    // Return the escape sequence to enable SGR mouse tracking.
    // Combines ?1000 (button events), ?1002 (button-event tracking
    // for drag), and ?1006 (SGR extended coordinates).
    static std::string enable();

    // Return the escape sequence to disable mouse tracking.
    static std::string disable();

    // Check whether both stdin and stdout are connected to a terminal.
    static bool available();

    // Parse a complete SGR mouse escape sequence into a MouseState.
    // Returns std::nullopt if the sequence is malformed or incomplete.
    //
    // Expected format:
    //   \033[<Cb;Cx;RyM   — press or drag
    //   \033[<Cb;Cx;Rym   — release
    static std::optional<MouseState> parse(std::string_view seq);

    // Block until a complete mouse event arrives on STDIN_FILENO,
    // then parse and return it.
    //
    // This method reads one byte at a time, accumulates them in an
    // internal buffer, and calls find_and_parse() each iteration.
    // The buffer is cleared after a successful parse.
    MouseState capture();

  private:
    // Scan a buffer (which may contain non-mouse data) for a trailing
    // SGR mouse sequence, starting from the last \033 character.
    std::optional<MouseState> find_and_parse(std::string_view buf);
    std::string buf_;   // accumulator for partial reads
};

inline std::string mouse::enable() {
    return "\033[?1000h\033[?1002h\033[?1006h";
}

inline std::string mouse::disable() {
    return "\033[?1006l\033[?1002l\033[?1000l";
}

inline bool mouse::available() {
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

// Parser for SGR mouse sequences.
//
// Wire format (terminator M = press/drag, m = release):
//   \033[<button+mods;col;rowM
//   \033[<button+mods;col;rowm
//
// The button+mods field (cb) is a bitmask:
//   bits 0-1 : button (0=left, 1=middle, 2=right, 3=none/release)
//   bits 2-4 : modifiers (shift, meta, ctrl)
//   bits 5-6 : action type (0=press/release, 1=drag, 2=scroll-up, 3=scroll-down)
inline std::optional<MouseState> mouse::parse(std::string_view seq) {
    // Minimum valid SGR sequence: \033[<;M (7 chars).
    if (seq.size() < 7 || seq[0] != '\033' || seq[1] != '[' || seq[2] != '<')
        return std::nullopt;
    auto term = seq.back();
    if (term != 'M' && term != 'm') return std::nullopt;

    // Locate semi-colon separators between cb, col, row.
    auto p1 = seq.find(';', 3);
    auto p2 = seq.find(';', p1 + 1);
    if (p1 == std::string_view::npos || p2 == std::string_view::npos)
        return std::nullopt;

    // Simple string-to-integer parser (std::from_chars would require
    // <charconv> and is not materially better here).
    auto to_int = [](std::string_view sv) -> int {
        int v = 0;
        for (auto ch : sv) {
            if (ch < '0' || ch > '9') break;
            v = v * 10 + (ch - '0');
        }
        return v;
    };

    // Extract the three numeric fields.
    auto cb  = to_int(seq.substr(3, p1 - 3));
    auto col = to_int(seq.substr(p1 + 1, p2 - p1 - 1));
    auto row = to_int(seq.substr(p2 + 1, seq.size() - p2 - 2));

    // Decode action type (bits 5-6) and button (bits 0-1).
    auto atype = (cb >> 5) & 3;
    auto btn   = cb & 3;

    MouseState ms;
    // Terminal reports 1-based coordinates; convert to 0-based.
    ms.col = col - 1;
    ms.row = row - 1;

    // Map the two-button bitfield to our Button enum.
    auto to_btn = [](uint8_t b) -> MouseState::Button {
        return b == 0 ? MouseState::Button::LEFT
             : b == 1 ? MouseState::Button::MIDDLE
             : b == 2 ? MouseState::Button::RIGHT
             : MouseState::Button::NONE;
    };

    switch (atype) {
    case 0:
        // Normal press / release.
        // Terminator 'm' or button=3 implies release; otherwise press.
        ms.action = (term == 'm' || btn == 3) ? MouseState::Action::RELEASE : MouseState::Action::PRESS;
        ms.button = to_btn(btn);
        break;
    case 1:
        // Drag with a button held.
        ms.action = MouseState::Action::DRAG;
        ms.button = to_btn(btn);
        break;
    case 2: ms.action = MouseState::Action::SCROLL_UP;   ms.button = MouseState::Button::NONE; break;
    case 3: ms.action = MouseState::Action::SCROLL_DOWN; ms.button = MouseState::Button::NONE; break;
    }
    return ms;
}

// Scan the buffer for the last \033 character and try to parse the
// trailing substring. Returns nullopt if no complete SGR sequence
// is found.
inline std::optional<MouseState> mouse::find_and_parse(std::string_view buf) {
    auto esc = buf.rfind('\033');
    if (esc == std::string_view::npos) return std::nullopt;
    auto seq = buf.substr(esc);
    if (seq.back() != 'M' && seq.back() != 'm') return std::nullopt;
    return parse(seq);
}

// Blocking read: accumulate bytes until a complete SGR mouse sequence
// is recognised, then return the parsed MouseState.
inline MouseState mouse::capture() {
    buf_.clear();
    while (true) {
        char c;
        // Busy-loop on read (VMIN=0, VTIME=1 in raw mode) until a byte
        // arrives.
        while (::read(STDIN_FILENO, &c, 1) <= 0)
            ;
        buf_ += c;
        if (auto m = find_and_parse(buf_)) {
            buf_.clear();
            return *m;
        }
        // Safety bound: if the buffer grows beyond 128 bytes without
        // producing a valid sequence, reset it to avoid unbounded
        // memory consumption (e.g. from stray terminal data).
        if (buf_.size() > 128) buf_.clear();
    }
}

} // namespace vaterm

#endif // VATERM_MOUSE_HPP
