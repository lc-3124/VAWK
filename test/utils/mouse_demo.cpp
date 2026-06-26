// -----------------------------------------------------------------------
//  mouse_demo — SGR mouse tracking drawing board
//
//  Demonstrates vaterm::mouse SGR-encoded mouse event support.  The
//  program puts the terminal into raw mode, enables SGR mouse tracking,
//  and enters a poll-based event loop:
//
//    - Left mouse button (press/drag) draws blue '*' glyphs.
//    - Right mouse button (press/drag) draws red '*' glyphs.
//    - Scroll wheel UP clears the screen.
//    - Press 'Q' to quit.
//
//  Unlike the blocking mouse::capture() approach, this demo uses
//  poll() + mouse::parse() so that keyboard input ('Q') can be
//  handled in the same main-loop without a separate thread.
//
//  Run:  ./mouse_demo   (requires a real terminal with mouse support)
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <poll.h>
#include <string>

using namespace vaterm;

int main() {
    terminal term;
    if (!term.enter_raw()) {
        printf("Not a tty — skipping interactive mouse demo.\n");
        printf("Run in a terminal: ./mouse_demo\n");
        return 0;
    }

    uint16_t rows = 0, cols = 0;
    terminal::size(rows, cols);
    if (rows < 10 || cols < 30) {
        terminal::write("Terminal too small\n");
        term.exit_raw();
        return 0;
    }

    terminal::write(terminal::clear_screen());
    terminal::write(cursor::hide());
    terminal::write(mouse::enable());

    terminal::write(cursor::move_to(1, 1));
    terminal::write(color::effect(TextEffect::BOLD) +
                    "vaterm Drawing Board  (Q to quit)" + color::reset());

    auto info_r = static_cast<uint16_t>(rows - 1);

    std::string buf;       // raw-byte accumulator
    bool quit = false;

    while (!quit) {
        // Poll stdin with 50 ms timeout so keyboard input is responsive.
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        int ret = poll(&pfd, 1, 50);
        if (ret <= 0) continue;

        char c;
        if (::read(STDIN_FILENO, &c, 1) <= 0) continue;

        // Single-character 'q'/'Q' → quit immediately.
        if (c == 'q' || c == 'Q') { quit = true; break; }

        buf += c;

        // Try to find a trailing SGR mouse sequence in the accumulated
        // buffer.  mouse::parse() handles the actual decoding.
        auto esc = buf.rfind('\033');
        if (esc == std::string::npos) {
            if (buf.size() > 128) buf.clear();
            continue;
        }
        auto tail = buf.substr(esc);
        if (tail.back() != 'M' && tail.back() != 'm') {
            if (buf.size() > 128) buf.clear();
            continue;
        }

        auto state = mouse::parse(tail);
        if (!state) { buf.clear(); continue; }
        buf.clear();

        auto& s = *state;

        // Draw at the reported position.
        if (s.action == MouseState::Action::DRAG || s.action == MouseState::Action::PRESS) {
            auto fg = (s.button == MouseState::Button::LEFT)  ? color::fg(0, 100, 255)
                    : (s.button == MouseState::Button::RIGHT) ? color::fg(255, 50, 50)
                    : color::fg(Color4::BRIGHT_BLACK);
            terminal::write(cursor::move_to(s.row, s.col));
            terminal::write(fg + "*" + color::reset());
        }

        if (s.action == MouseState::Action::SCROLL_UP)
            terminal::write(terminal::clear_screen());

        // Info line at the bottom.
        terminal::write(cursor::move_to(info_r, 1) + terminal::clear_line() +
                        color::fg(Color4::BRIGHT_BLACK) +
                        "(" + std::to_string(s.col) + "," +
                        std::to_string(s.row) + ")  " +
                        ((s.button == MouseState::Button::LEFT)  ? "Left" :
                         (s.button == MouseState::Button::RIGHT) ? "Right" :
                         (s.button == MouseState::Button::MIDDLE) ? "Middle" : "-") +
                        " " +
                        ((s.action == MouseState::Action::PRESS)      ? "Press" :
                         (s.action == MouseState::Action::RELEASE)    ? "Release" :
                         (s.action == MouseState::Action::DRAG)       ? "Drag" :
                         (s.action == MouseState::Action::SCROLL_UP)  ? "Scroll Up" :
                         (s.action == MouseState::Action::SCROLL_DOWN) ? "Scroll Down" : "") +
                        color::reset());
        terminal::flush();
    }

    // Clean up terminal state.
    terminal::write(mouse::disable());
    terminal::write(cursor::show());
    terminal::write(cursor::move_to(rows, 1) + terminal::clear_line() +
                    color::fg(Color4::GREEN) + "Demo finished" +
                    color::reset() + "\n");
    terminal::flush();
    return 0;
}
