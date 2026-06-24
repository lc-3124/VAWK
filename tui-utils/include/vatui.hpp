#ifndef VATUI_HPP
#define VATUI_HPP

// -----------------------------------------------------------------------
//  vatui — terminal framebuffer and input toolkit
//
//  Builds on vaterm with:
//    - Framebuffer  — double-buffered cell matrix, diff-based swap,
//                     UTF-8 / wide-character support, styled fill & text.
//    - VaTui        — singleton that owns a Framebuffer + a terminal
//                     instance and provides unified input (key / mouse).
//
//  Usage:
//    auto& tui = VaTui::instance();
//    tui.init();
//    tui.enableMouse();
//    auto& fb = tui.buffer();
//    fb.printText({.col = 0, .row = 0, .text = "Hi", .style = {...}});
//    fb.swap();
//    auto ev = tui.waitInput();
//
//  The library is split into hpp (declaration) and cpp (implementation).
// -----------------------------------------------------------------------

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "vaterm/enums.hpp"
#include "vaterm/mouse.hpp"
#include "vaterm/term.hpp"

namespace vatui {

// ---- style descriptor ------------------------------------------------

struct Style {
    vaterm::Color4 fg   = vaterm::Color4::WHITE;
    vaterm::Color4 bg   = vaterm::Color4::BLACK;
    std::vector<vaterm::TextEffect> effects;
};

// ---- parameter structs (enables designated-initialiser syntax) -------

struct SizeArgs {
    int col;
    int row;
};

struct PositionArgs {
    int col;
    int row;
};

struct TextArgs {
    int         col;
    int         row;
    std::string_view text;
    Style            style = {};
};

struct RegionArgs {
    int col;
    int row;
    int w;
    int h;
};

struct FillArgs {
    int  col;
    int  row;
    int  w;
    int  h;
    char ch;
    Style style = {};
};

// ---- Key-code constants ----------------------------------------------

enum KeyCode : int {
    KEY_NONE = 0,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, KEY_INS, KEY_DEL,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_BACKSPACE,
};

// ---- Key event -------------------------------------------------------

struct Key {
    char32_t cp   = 0;       // Unicode codepoint for printable keys
    KeyCode  code = KEY_NONE; // special-key code (KEY_NONE for printable)
    bool     alt  = false;   // whether the Alt / Meta modifier was detected
    bool     ctrl = false;   // whether the Ctrl modifier was detected
};

// ---- Unified input ---------------------------------------------------

enum InputType : int {
    INPUT_KEY   = 0,
    INPUT_MOUSE = 1,
};

struct Input {
    InputType          type = INPUT_KEY;
    Key                key;
    vaterm::MouseState mouse;
};

// ---- Framebuffer (double-buffered cell matrix) -----------------------

class Framebuffer {
  public:
    // ── Buffer sizing ───────────────────────────────────────────────
    void setSize(SizeArgs args);
    int  getColMax() const { return max_col_; }
    int  getRowMax() const { return max_row_; }

    // ── Screen offset ───────────────────────────────────────────────
    void setPosition(PositionArgs args);
    int  getOffsetCol() const { return off_col_; }
    int  getOffsetRow() const { return off_row_; }

    // ── Drawing ─────────────────────────────────────────────────────
    void printText(TextArgs args);
    void clear();
    void clearRegion(RegionArgs args);
    void fillRegion(FillArgs args);

    // ── Output ──────────────────────────────────────────────────────
    void swap();

  private:
    friend class VaTui;

    struct Cell {
        bool     isLong_ = false;
        bool     isHead_ = false;
        char32_t cp_     = 0;
        char     data_   = ' ';
        Style    style_;
    };

    Cell& cell_at_(int col, int row) {
        return buf_[row * max_col_ + col];
    }

    static void reset_cell_(Cell& cell, Style style = {}) {
        cell.data_   = ' ';
        cell.cp_     = 0;
        cell.isLong_ = false;
        cell.isHead_ = true;
        cell.style_  = style;
    }

    int              max_col_ = 0;
    int              max_row_ = 0;
    int              off_col_ = 0;
    int              off_row_ = 0;
    std::vector<Cell> buf_;
    bool             dirty_  = false;
};

// ---- VaTui singleton -------------------------------------------------

class VaTui {
  public:
    static VaTui& instance();

    // Non-copyable.
    VaTui(const VaTui&) = delete;
    VaTui& operator=(const VaTui&) = delete;

    // ── Lifecycle ───────────────────────────────────────────────────
    // Enter raw mode, hide cursor, clear screen.
    // Safe to call multiple times (idempotent).
    void init();

    // ── Framebuffer access ──────────────────────────────────────────
    Framebuffer& buffer() { return fb_; }

    // ── Unified input ───────────────────────────────────────────────
    std::optional<Input> getInput();
    Input                waitInput();

    // ── Mouse enable / disable ──────────────────────────────────────
    void enableMouse();
    void disableMouse();

  private:
    VaTui();
    ~VaTui();

    Framebuffer          fb_;
    vaterm::terminal     term_;
    bool                 initialized_ = false;
    std::string          input_buf_;
};

} // namespace vatui

#endif // VATUI_HPP
