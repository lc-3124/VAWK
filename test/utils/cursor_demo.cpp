// -----------------------------------------------------------------------
//  cursor_demo — interactive cursor manipulation demo
//
//  Keyboard controls (inside the demo):
//    W/S    — move cursor up/down
//    A/D    — move cursor left/right
//    J      — save cursor position (SCP)
//    K      — restore cursor position (RCP)
//    H      — hide cursor (DECTCEM)
//    L      — show cursor (DECTCEM)
//    1-6    — switch cursor shape (DECSCUSR)
//    Q      — quit
//
//  The terminal cursor itself is the visual object being manipulated.
//  A status line at the bottom shows current shape, visibility, and
//  position.  A help panel describes the available keys.
//
//  Run:  ./cursor_demo   (requires a real terminal)
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace vaterm;

// Convenience: move cursor and write a string at (r, c).
static void write_at(uint16_t r, uint16_t c, const std::string& s) {
    terminal::write(cursor::move_to(r, c));
    terminal::write(s);
}

// Print the status line at the bottom of the screen.
static void status_line(uint16_t rows, uint16_t cy, uint16_t cx,
                        const std::string& shape, bool hidden,
                        const std::string& msg) {
    auto r = static_cast<uint16_t>(rows - 1);
    terminal::write(cursor::move_to(r, 1));
    terminal::write(terminal::clear_line());
    terminal::write(color::fg(Color4::BRIGHT_BLACK));
    terminal::write(" [形状:" + shape + "] [光标:" + (hidden ? "隐藏" : "可见") +
                    "] [位置:(" + std::to_string(cy) + "," + std::to_string(cx) + ")]");
    if (!msg.empty())
        terminal::write("  " + msg);
    terminal::write(color::reset());
}

// Draw the help panel with control descriptions.
static void help_panel(uint16_t rows) {
    auto r = static_cast<uint16_t>(rows - 11);
    terminal::write(cursor::move_to(r, 2));
    terminal::write(color::effect(TextEffect::BOLD));
    write_at(r,     2, "+-- Controls --+");
    write_at(r + 1, 2, "| W/S  Up/Dn  |");
    write_at(r + 2, 2, "| A/D  L/R    |");
    write_at(r + 3, 2, "| J    Save   |");
    write_at(r + 4, 2, "| K    Restore|");
    write_at(r + 5, 2, "| H    Hide   |");
    write_at(r + 6, 2, "| L    Show   |");
    write_at(r + 7, 2, "| 1-6  Shape  |");
    write_at(r + 8, 2, "| Q    Quit   |");
    write_at(r + 9, 2, "+-------------+");
    terminal::write(color::reset());
}

// Print the boundary hint for the movable area.
static void play_area_hint(uint16_t rows) {
    auto bot = static_cast<uint16_t>(rows - 12);
    write_at(bot, 2, color::fg(Color4::BRIGHT_BLACK) +
             "--- Movement area (terminal cursor moves here) ---" + color::reset());
}

int main() {
    terminal term;
    if (!term.enter_raw()) {
        fprintf(stderr, "failed to enter raw mode\n");
        return 1;
    }

    uint16_t rows = 0, cols = 0;
    terminal::size(rows, cols);
    if (rows < 18 || cols < 40) {
        terminal::write("Terminal too small, please enlarge\n");
        term.exit_raw();
        return 1;
    }

    auto hint_row = static_cast<uint16_t>(rows - 12);
    auto status_row = static_cast<uint16_t>(rows - 1);

    terminal::write(terminal::clear_screen());

    // Title bar.
    write_at(1, 2, color::effect(TextEffect::BOLD) + color::fg(Color4::CYAN) +
             "vaterm cursor Interactive Demo" + color::reset());
    write_at(2, 2, color::fg(Color4::BRIGHT_BLACK) +
             "The terminal cursor is the object. Move it with WASD inside the area." + color::reset());

    help_panel(rows);
    play_area_hint(rows);

    // Initial cursor position — centre of the play area.
    uint16_t cx = static_cast<uint16_t>(cols / 2);
    uint16_t cy = static_cast<uint16_t>((3 + hint_row) / 2);

    // Cursor shape name tracker.
    std::string shape_name = "Steady Block";
    auto set_shape = [&](CursorShape s, const std::string& name) {
        terminal::write(cursor::shape(s));
        shape_name = name;
    };

    status_line(rows, cy, cx, shape_name, false, "Ready — press Q to quit");
    write_at(cy, cx, "");
    terminal::flush();

    bool hidden = false;
    std::string msg;
    bool running = true;
    terminal::flush();

    while (running) {
        cursor::move_to(cy ,cx);
        int ch = terminal::read_byte();
        if (ch < 0) continue;

        switch (ch) {
        case 'w': case 'W': if (cy > 3) { cy--; msg = ""; } break;
        case 's': case 'S': if (cy < hint_row - 1) { cy++; msg = ""; } break;
        case 'a': case 'A': if (cx > 1) { cx--; msg = ""; } break;
        case 'd': case 'D': if (cx < cols) { cx++; msg = ""; } break;
        case 'j': case 'J':
            terminal::write(cursor::save());
            write_at(2, 2, terminal::clear_line() + color::fg(Color4::BRIGHT_BLACK) +
                     "Position saved. Press K to restore" + color::reset());
            msg = "Saved";
            break;
        case 'k': case 'K':
            terminal::write(cursor::restore());
            write_at(2, 2, terminal::clear_line() + color::fg(Color4::BRIGHT_BLACK) +
                     "Restored saved position (internal coords may be approximate)" + color::reset());
            msg = "Restored";
            break;
        case 'h': case 'H':
            terminal::write(cursor::hide());
            hidden = true;
            msg = "";
            break;
        case 'l': case 'L':
            terminal::write(cursor::show());
            hidden = false;
            msg = "";
            break;
        case '1': set_shape(CursorShape::BLINKING_BLOCK,   "Blinking Block");    msg = ""; break;
        case '2': set_shape(CursorShape::STEADY_BLOCK,     "Steady Block");      msg = ""; break;
        case '3': set_shape(CursorShape::BLINKING_UNDERLINE, "Blinking Underline"); msg = ""; break;
        case '4': set_shape(CursorShape::STEADY_UNDERLINE, "Steady Underline");    msg = ""; break;
        case '5': set_shape(CursorShape::BLINKING_BAR,     "Blinking Bar");      msg = ""; break;
        case '6': set_shape(CursorShape::STEADY_BAR,       "Steady Bar");        msg = ""; break;
        case 'q': case 'Q': running = false; break;
        }

        status_line(rows, cy, cx, shape_name, hidden, msg);
        write_at(cy, cx, "");
        terminal::flush();
    }

    // Clean up terminal state before exit.
    terminal::write(cursor::show());
    terminal::write(cursor::shape(CursorShape::STEADY_BLOCK));
    write_at(status_row, 1, terminal::clear_line() +
             color::fg(Color4::GREEN) + "Demo finished" + color::reset() + "\n");
    terminal::flush();
    return 0;
}
