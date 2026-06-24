/**
 * demo — interactive VaTui text rendering test
 *
 * Launches a 40×20 framebuffer on a colour-capable terminal and repeatedly
 * draws several lines of mixed ASCII / CJK text with different foreground,
 * background and text-effect styles.  A movable cursor cross-hair ("< 按
 * wasd移动我") can be steered with WASD to observe how VaTui handles
 * overlay writes and wide-character fragment cleanup in real time.
 *
 * Key VaTui patterns exercised here:
 *
 *   1. Singleton lifecycle
 *      ────────────────────
 *      VaTui::instance().init() enters raw mode and clears the screen.
 *      The destructor (at program exit) restores the terminal.
 *
 *   2. Parameter-struct call syntax
 *      ─────────────────────────────
 *      Every drawing call uses a designated-initialiser struct so the
 *      argument role is clear at the call site:
 *        fb.printText({ .col = x, .row = y, .text = s, .style = {...} })
 *
 *   3. Style descriptors
 *      ──────────────────
 *      Style stores pre-computed SGR strings for fg, bg, and effects.
 *      Colours are converted when the Style is constructed; swap() simply
 *      outputs the cached SGR — zero runtime colour conversion.
 *
 *   4. Frame loop
 *      ────────────
 *      Each iteration: clear() the buffer, print text lines, print the
 *      cursor overlay, then swap().  swap() flushes the minimal set of
 *      ANSI sequences needed to bring the physical terminal in sync
 *      with the logical buffer.
 *
 *   5. Wide-character handling
 *      ────────────────────────
 *      The test string (" hello 世界 ， 你好😃  VAWK！!") contains
 *      CJK ideographs (width 2) and an emoji (width 2).  When the
 *      overlay text moves across these characters VaTui's protection
 *      code clears stale wide-character half-cells and replaces them
 *      with styled spaces that retain the original colour scheme.
 *
 * Build:
 *   make -C test/tui all
 *
 * Run:
 *   ./test/tui/print          — requires ≥ 40×20 terminal
 *   Press w/a/s/d to move the overlay; q to quit.
 */

#include <vatui.hpp>
#include <vaterm/term.hpp>
#include <iostream>

using namespace vaterm;
using namespace vatui;

int main() {
    // ── Terminal-size guard ──────────────────────────────────────────
    if (vaterm::terminal::getColMax() < 40 || vaterm::terminal::getRowMax() < 20) {
        std::cout << "col: " << vaterm::terminal::getColMax() << std::endl;
        std::cout << "row: " << vaterm::terminal::getRowMax() << std::endl;
        std::cout << "Make sure your term size in at least row 30 col 60\n";
        return 0;
    }

    // ── Initialisation ───────────────────────────────────────────────
    auto& tui = VaTui::instance();
    tui.init();
    auto& fb = tui.buffer();
    fb.setSize({40, 20});

    // ── Test strings ─────────────────────────────────────────────────
    int x = 3, y = 10;

    std::string outputb = " hello 世界 ， 你好😃  VAWK！!";
    std::string outputf = "< 按wasd移动我";

    // ── Main loop ────────────────────────────────────────────────────
    char ch = 0;
    while (true) {
        // --- WASD movement (clamped to buffer bounds) ----------------
        if (ch == 'w' && y != 0)                    y--;
        if (ch == 'a' && x != 0)                    x--;
        if (ch == 's' && y != fb.getRowMax())       y++;
        if (ch == 'd' && x != fb.getColMax())       x++;
        if (ch == 'q') break;

        // --- Frame draw ----------------------------------------------
        fb.clear();

        // Line 1 — bright red on blue at (0, 1)
        fb.printText({0, 1, outputb,
            {.fg_sgr = fg(Color4::BRIGHT_RED), .bg_sgr = bg(Color4::BLUE),
             .effects_sgr = effects({TextEffect::BOLD, TextEffect::ITALIC})}});

        // Line 2 — bright blue on red at (1, 5)
        fb.printText({1, 5, outputb,
            {.fg_sgr = fg(Color4::BRIGHT_BLUE), .bg_sgr = bg(Color4::RED),
             .effects_sgr = effects({TextEffect::BOLD, TextEffect::ITALIC})}});

        // Line 3 — bright red on red at (3, 10) (foreground blends)
        fb.printText({3, 10, outputb,
            {.fg_sgr = fg(Color4::BRIGHT_RED), .bg_sgr = bg(Color4::RED),
             .effects_sgr = effects({TextEffect::BOLD, TextEffect::ITALIC})}});

        // Line 4 — black on white at (4, 15)
        fb.printText({4, 15, outputb,
            {.fg_sgr = fg(Color4::BLACK), .bg_sgr = bg(Color4::WHITE),
             .effects_sgr = effects({TextEffect::BOLD, TextEffect::ITALIC})}});

        // Overlay — user-movable cross-hair
        fb.printText({x, y, outputf,
            {.fg_sgr = fg(Color4::BRIGHT_MAGENTA), .bg_sgr = bg(Color4::BRIGHT_YELLOW),
             .effects_sgr = effects({TextEffect::BOLD, TextEffect::ITALIC})}});

        // --- Swap / flush -------------------------------------------
        fb.swap();

        // Wait for one keypress (blocking).
        ch = vaterm::terminal::getch();
    }
}
