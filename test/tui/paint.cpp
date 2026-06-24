/**
 * paint — mouse-driven drawing board with VaTui
 *
 * Uses a full-screen VaTui framebuffer and unified input (getInput) to
 * implement a minimal paint program:
 *
 *   Left mouse  – draw in bright blue
 *   Right mouse – draw in bright red
 *   Middle      – erase (black)
 *   Scroll up   – clear canvas
 *   Q / Escape  – quit
 *
 * A status bar on the last line shows the most recent mouse position,
 * button, action, and last keyboard event.
 *
 * Build:
 *   make -C test/tui all
 *
 * Run:
 *   ./test/tui/paint          — requires a ≥ 50×15 terminal
 */

#include <vatui.hpp>
#include <vaterm/term.hpp>
#include <cstdio>

using namespace vaterm;
using namespace vatui;

int main() {
    // ── Terminal-size guard ──────────────────────────────────────────
    auto cols = terminal::getColMax();
    auto rows = terminal::getRowMax();
    if (cols < 50 || rows < 15) {
        std::printf("Need at least 50×15 terminal\n");
        return 0;
    }

    // ── Initialise ───────────────────────────────────────────────────
    auto& tui = VaTui::instance();
    tui.init();
    tui.enableMouse();
    auto& fb = tui.buffer();

    // Drawing styles and helper.
    auto paint_cell = [&](int col, int row, Style s) {
        fb.fillRegion({.col = col, .row = row, .w = 1, .h = 1,
                       .ch = ' ', .style = s});
    };

    Style style_blue  = {.fg_sgr = fg(Rgb{255, 255, 255}),     .bg_sgr = bg(Rgb{60, 120, 255})};
    Style style_red   = {.fg_sgr = fg(Color4::WHITE),          .bg_sgr = bg(Color8{196})};
    Style style_erase = {.fg_sgr = fg(Color4::WHITE),          .bg_sgr = bg(Color4::BLACK)};
    Style style_draw  = style_blue;

    // Status area (bottom two rows).
    auto status_row = rows - 2;
    std::string status_text = "Ready";
    std::string last_key_str;

    bool quit = false;

    // ── Main event loop ──────────────────────────────────────────────
    while (!quit) {
        auto inp = tui.waitInput();

        if (inp.type == INPUT_MOUSE) {
            auto& m = inp.mouse;

            // Select style based on button.
            if (m.button == MouseState::LEFT)   style_draw = style_blue;
            if (m.button == MouseState::RIGHT)  style_draw = style_red;
            if (m.button == MouseState::MIDDLE) style_draw = style_erase;

            // Paint on press or drag, skip status area.
            if ((m.action == MouseState::PRESS || m.action == MouseState::DRAG) &&
                m.row >= 0 && m.row < status_row && m.col >= 0 && m.col < cols) {
                paint_cell(m.col, m.row, style_draw);
            }

            if (m.action == MouseState::SCROLL_UP) {
                fb.clear();
            }

            // Build status bar text.
            auto btn_name = m.button == MouseState::LEFT   ? "L"
                          : m.button == MouseState::RIGHT  ? "R"
                          : m.button == MouseState::MIDDLE ? "M" : "-";
            auto act_name = m.action == MouseState::PRESS       ? "Press"
                          : m.action == MouseState::RELEASE     ? "Release"
                          : m.action == MouseState::DRAG        ? "Drag"
                          : m.action == MouseState::SCROLL_UP   ? "ScrollUp"
                          : m.action == MouseState::SCROLL_DOWN ? "ScrollDn" : "";
            status_text = "Mouse: (" + std::to_string(m.col) + "," +
                          std::to_string(m.row) + ") " + btn_name + " " +
                          act_name;
        }

        if (inp.type == INPUT_KEY) {
            auto& k = inp.key;
            std::string mod;
            if (k.ctrl) mod += "C-";
            if (k.alt)  mod += "M-";

            if (k.cp != 0) {
                char seq[16] = {};
                if (k.cp < 0x80) {
                    seq[0] = static_cast<char>(k.cp);
                    last_key_str = mod + seq;
                } else {
                    std::snprintf(seq, sizeof seq, "U+%04X",
                                  static_cast<unsigned>(k.cp));
                    last_key_str = mod + seq;
                }
            } else {
                static const char* names[] = {
                    "UP","DOWN","LEFT","RIGHT",
                    "HOME","END","PGUP","PGDN","INS","DEL",
                    "F1","F2","F3","F4","F5","F6",
                    "F7","F8","F9","F10","F11","F12",
                    "ESC","TAB","ENTER","BACKSPACE",
                };
                int idx = k.code - KEY_UP;
                if (idx >= 0 && static_cast<size_t>(idx) < sizeof names / sizeof names[0])
                    last_key_str = mod + names[idx];
            }

            if (k.code == KEY_BACKSPACE) {
                style_draw = style_erase;
            } else if (k.code == KEY_ENTER) {
                style_draw = style_blue;
                last_key_str += " (ok)";
            }

            if (k.code == KEY_NONE && k.cp == 'u' && k.ctrl) {
                fb.clear();
            }

            if ((k.code == KEY_NONE && (k.cp == 'q' || k.cp == 'Q'))
                || k.code == KEY_ESC) {
                quit = true;
            }
        }

        // ── Render status bar ────────────────────────────────────────
        std::string key_info;
        if (!last_key_str.empty())
            key_info = "Key: " + last_key_str;
        std::string bar = status_text;
        if (!key_info.empty()) bar += "  |  " + key_info;
        if (bar.size() > static_cast<size_t>(cols)) bar.resize(cols);

        fb.fillRegion({.col = 0, .row = status_row, .w = cols, .h = 2,
                       .ch = ' ', .style = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BLACK)}});
        fb.printText({.col = 0, .row = status_row, .text = bar,
                      .style = {.fg_sgr = fg(Color4::BRIGHT_GREEN), .bg_sgr = bg(Color4::BLACK)}});
        fb.printText({.col = 0, .row = status_row + 1,
                       .text = "Q: quit  |  L: blue  R: red  M: erase  BSpc: erase  Ent: ok  ^U: clear",
                      .style = {.fg_sgr = fg(Color4::BRIGHT_BLACK), .bg_sgr = bg(Color4::BLACK)}});

        fb.swap();
    }

    tui.disableMouse();
}
