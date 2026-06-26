/**
 * color — display colour-support test for every depth (4-bit / 8-bit / 24-bit)
 *
 * Renders labelled colour blocks for:
 *   - Color4  (the 16 ANSI named colours)
 *   - Color8  (system 0-15, the 6×6×6 cube 16-231, grey ramp 232-255)
 *   - Rgb     (24-bit true-colour gradients)
 *
 * Build:
 *   make -C test/tui all
 *
 * Run:
 *   ./test/tui/color          — requires ≥ 80×24 terminal; press Q to quit
 */

#include <vatui.hpp>
#include <vaterm/color.hpp>
#include <vaterm/term.hpp>
#include <cstdio>

using namespace vaterm;
using namespace vatui;

// ── Helpers ─────────────────────────────────────────────────────────────

// Draw a single labelled colour block at (col,row).
static void color_block(Framebuffer& fb, int col, int row,
                        int w, int h, const std::string& bg_sgr,
                        const char* label, const std::string& fg_sgr) {
    fb.fillRegion({.col = col, .row = row, .w = w, .h = h,
                   .ch = ' ', .style = {.fg_sgr = fg_sgr, .bg_sgr = bg_sgr}});
    int lw = 0;
    while (label[lw]) ++lw;
    int lc = col + (w - lw) / 2;
    if (lc >= 0)
        fb.printText({.col = lc, .row = row, .text = std::string_view(label, lw),
                      .style = {.fg_sgr = fg_sgr, .bg_sgr = bg_sgr}});
}

// Foreground colour with enough contrast against a given background.
static std::string contrast_fg(Color4 bg) {
    auto v = static_cast<uint8_t>(bg);
    return (v == 0 || v == 4 || v == 8 || v == 12) ? fg(Color4::WHITE) : fg(Color4::BLACK);
}
static std::string contrast_fg(Color8 bg) {
    auto rgb = color::_256_to_rgb(bg.index);
    int lum = rgb.r * 299 + rgb.g * 587 + rgb.b * 114;
    return lum < 150000 ? fg(Color4::WHITE) : fg(Color4::BLACK);
}

// Draw a section title line.
static void title(Framebuffer& fb, int row, int cols, const char* text) {
    auto style = Style{.fg_sgr = fg(Color4::BRIGHT_WHITE), .bg_sgr = bg(Color4::BLUE),
                       .effects_sgr = effects({TextEffect::BOLD})};
    int len = 0;
    while (text[len]) ++len;
    fb.fillRegion({.col = 0, .row = row, .w = cols, .h = 1,
                   .ch = ' ', .style = style});
    fb.printText({.col = 1, .row = row, .text = std::string_view(text, len),
                  .style = style});
}

// ── Main ───────────────────────────────────────────────────────────────

int main() {
    auto cols = terminal::getColMax();
    auto rows = terminal::getRowMax();
    if (cols < 80 || rows < 24) {
        std::printf("Need at least 80×24 terminal (have %d×%d)\n", cols, rows);
        return 0;
    }

    auto& tui = VaTui::instance();
    tui.init();
    auto& fb = tui.buffer();
    fb.setSize({cols, rows});

    int r = 0;

    // ── Section 1: 4-bit ANSI colours (Color4) ───────────────────────
    title(fb, r++, cols, "4-bit ANSI (Color4)");
    static const char* c4_names[] = {
        "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN", "WHITE",
        "BR_BLK","BR_RED","BR_GRN","BR_YEL","BR_BLU","BR_MAG","BR_CYN","BR_WHT",
    };
    for (int i = 0; i < 16; ++i) {
        int c = (i % 8) * 10;
        int rw = r + (i / 8);
        auto c4 = static_cast<Color4>(i);
        color_block(fb, c, rw, 9, 1, bg(c4), c4_names[i], contrast_fg(c4));
    }
    r += 3;

    // ── Section 2: 8-bit system colours (Color8 0-15) ───────────────
    title(fb, r++, cols, "8-bit system colours (Color8 0-15)");
    for (int i = 0; i < 16; ++i) {
        int c = (i % 8) * 10;
        int rw = r + (i / 8);
        auto c8 = Color8{static_cast<uint8_t>(i)};
        char label[16];
        std::snprintf(label, sizeof label, "C8 %3d", i);
        color_block(fb, c, rw, 9, 1, bg(c8), label, contrast_fg(c8));
    }
    r += 3;

    // ── Section 3: 8-bit 6×6×6 cube (Color8 16-231) ─────────────────
    title(fb, r++, cols, "8-bit 6x6x6 cube (Color8 16-231)  —  R →, G ↓, plane B");
    for (int b = 0; b < 6; ++b) {
        for (int g = 0; g < 6; ++g) {
            for (int rr = 0; rr < 6; ++rr) {
                int idx = 16 + rr * 36 + g * 6 + b;
                int cx = 4 + rr * 6 + g;
                int ry = r + b;
                auto c8 = Color8{static_cast<uint8_t>(idx)};
                Style s = {.fg_sgr = fg(c8), .bg_sgr = bg(c8)};
                fb.fillRegion({.col = cx, .row = ry, .w = 1, .h = 1,
                               .ch = ' ', .style = s});
            }
        }
    }
    fb.printText({.col = 0, .row = r, .text = "B0",
                  .style = {.fg_sgr = fg(Color4::BRIGHT_BLACK)}});
    fb.printText({.col = 0, .row = r + 5, .text = "B5",
                  .style = {.fg_sgr = fg(Color4::BRIGHT_BLACK)}});
    r += 7;

    // ── Section 4: 8-bit grayscale (Color8 232-255) ──────────────────
    title(fb, r++, cols, "8-bit grayscale (Color8 232-255)");
    for (int i = 0; i < 24; ++i) {
        int idx = 232 + i;
        int cx = i * 3;
        auto c8 = Color8{static_cast<uint8_t>(idx)};
        fb.fillRegion({.col = cx, .row = r, .w = 3, .h = 2,
                       .ch = ' ', .style = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(c8)}});
        char lbl[4];
        std::snprintf(lbl, sizeof lbl, "%d", idx);
        fb.printText({.col = cx, .row = r, .text = std::string_view(lbl),
                      .style = {.fg_sgr = contrast_fg(c8), .bg_sgr = bg(c8)}});
    }
    r += 3;

    // ── Section 5: 24-bit true-colour gradients (Rgb) ────────────────
    title(fb, r++, cols, "24-bit true-colour gradients (Rgb)");

    // Red → Green gradient.
    for (int x = 0; x < cols; ++x) {
        uint8_t red   = static_cast<uint8_t>(255 - x * 255 / (cols - 1));
        uint8_t green = static_cast<uint8_t>(x * 255 / (cols - 1));
        auto rgb = Rgb{red, green, 0};
        Style s = {.fg_sgr = fg(rgb), .bg_sgr = bg(rgb)};
        fb.fillRegion({.col = x, .row = r, .w = 1, .h = 1,
                       .ch = ' ', .style = s});
    }
    fb.printText({.col = 0, .row = r, .text = "R->G gradient",
                  .style = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BLACK)}});
    ++r;

    // Green → Blue gradient.
    for (int x = 0; x < cols; ++x) {
        uint8_t green = static_cast<uint8_t>(255 - x * 255 / (cols - 1));
        uint8_t blue  = static_cast<uint8_t>(x * 255 / (cols - 1));
        auto rgb = Rgb{0, green, blue};
        Style s = {.fg_sgr = fg(rgb), .bg_sgr = bg(rgb)};
        fb.fillRegion({.col = x, .row = r, .w = 1, .h = 1,
                       .ch = ' ', .style = s});
    }
    fb.printText({.col = 0, .row = r, .text = "G->B gradient",
                  .style = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BLACK)}});
    ++r;

    // Blue → Red gradient.
    for (int x = 0; x < cols; ++x) {
        uint8_t blue = static_cast<uint8_t>(255 - x * 255 / (cols - 1));
        uint8_t red  = static_cast<uint8_t>(x * 255 / (cols - 1));
        auto rgb = Rgb{red, 0, blue};
        Style s = {.fg_sgr = fg(rgb), .bg_sgr = bg(rgb)};
        fb.fillRegion({.col = x, .row = r, .w = 1, .h = 1,
                       .ch = ' ', .style = s});
    }
    fb.printText({.col = 0, .row = r, .text = "B->R gradient",
                  .style = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BLACK)}});
    ++r;

    // ── Info line ─────────────────────────────────────────────────────
    auto depth = terminal::detect_color_depth();
    const char* depth_name = depth == ColorDepth::C24 ? "24-bit (truecolor)"
                           : depth == ColorDepth::C8  ? "8-bit (256-colour)"
                           :                            "4-bit (16-colour)";
    char info[80];
    std::snprintf(info, sizeof info, "Terminal: %s  |  %dx%d  |  Press Q to quit",
                  depth_name, cols, rows);
    fb.fillRegion({.col = 0, .row = rows - 1, .w = cols, .h = 1,
                   .ch = ' ', .style = {.fg_sgr = fg(Color4::BLACK), .bg_sgr = bg(Color4::BRIGHT_WHITE)}});
    fb.printText({.col = 0, .row = rows - 1, .text = info,
                  .style = {.fg_sgr = fg(Color4::BLACK), .bg_sgr = bg(Color4::BRIGHT_WHITE)}});

    fb.swap();

    // ── Wait for Q / Escape ───────────────────────────────────────────
    while (true) {
        auto inp = tui.waitInput();
        if (inp.type == InputType::KEY) {
            auto& k = inp.key;
            if ((k.code == KeyCode::NONE && (k.cp == 'q' || k.cp == 'Q'))
                || k.code == KeyCode::ESC)
                break;
        }
    }
}
