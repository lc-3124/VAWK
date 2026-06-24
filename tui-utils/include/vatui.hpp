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

#include "vaterm/color.hpp"
#include "vaterm/enums.hpp"
#include "vaterm/mouse.hpp"
#include "vaterm/term.hpp"

namespace vatui {

// ---- colour SGR helpers (immediate conversion) -------------------------
// Each call detects the terminal's colour depth on first invocation and
// automatically converts the input to the terminal's native depth.
// Use inside Style initialisers:
//   Style{.fg_sgr = fg(Color4::GREEN), .bg_sgr = bg(Color8{196})}
//
// Conversion rules (fg&bg):
//   Color4 → passthrough (4-bit works on any terminal)
//   Color8 → C24: up-convert via _256_to_rgb
//            C4:  down-convert via _256_to_rgb + nearest_4bit
//            C8:  passthrough
//   Rgb    → C24: passthrough
//            C8:  down-convert via rgb_to_256
//            C4:  down-convert via nearest_4bit

inline std::string fg(vaterm::Color4 c) {
    return vaterm::color::fg(c);
}

inline std::string fg(vaterm::Color8 c) {
    static auto depth = vaterm::terminal::detect_color_depth();
    if (depth >= vaterm::ColorDepth::C24) {
        auto rgb = vaterm::color::_256_to_rgb(c.index);
        return vaterm::color::fg(rgb);
    }
    if (depth == vaterm::ColorDepth::C4) {
        auto rgb = vaterm::color::_256_to_rgb(c.index);
        return vaterm::color::fg(vaterm::color::nearest_4bit(rgb));
    }
    return vaterm::color::fg(c.index);
}

inline std::string fg(vaterm::Rgb c) {
    static auto depth = vaterm::terminal::detect_color_depth();
    if (depth >= vaterm::ColorDepth::C24)
        return vaterm::color::fg(c);
    if (depth == vaterm::ColorDepth::C8)
        return vaterm::color::fg(vaterm::color::rgb_to_256(c.r, c.g, c.b));
    return vaterm::color::fg(vaterm::color::nearest_4bit(c));
}

inline std::string fg(vaterm::Trans) { return std::string{}; }

inline std::string bg(vaterm::Color4 c) {
    return vaterm::color::bg(c);
}

inline std::string bg(vaterm::Color8 c) {
    static auto depth = vaterm::terminal::detect_color_depth();
    if (depth >= vaterm::ColorDepth::C24) {
        auto rgb = vaterm::color::_256_to_rgb(c.index);
        return vaterm::color::bg(rgb);
    }
    if (depth == vaterm::ColorDepth::C4) {
        auto rgb = vaterm::color::_256_to_rgb(c.index);
        return vaterm::color::bg(vaterm::color::nearest_4bit(rgb));
    }
    return vaterm::color::bg(c.index);
}

inline std::string bg(vaterm::Rgb c) {
    static auto depth = vaterm::terminal::detect_color_depth();
    if (depth >= vaterm::ColorDepth::C24)
        return vaterm::color::bg(c);
    if (depth == vaterm::ColorDepth::C8)
        return vaterm::color::bg(vaterm::color::rgb_to_256(c.r, c.g, c.b));
    return vaterm::color::bg(vaterm::color::nearest_4bit(c));
}

inline std::string bg(vaterm::Trans) { return std::string{}; }

inline std::string effects(std::initializer_list<vaterm::TextEffect> list) {
    return vaterm::color::effect(list);
}

// ---- style descriptor (pre-computed SGR strings) -----------------------
// Every colour value is converted to its ANSI SGR sequence at the point
// of assignment.  fillRegion / printText then simply concatenate the
// three pieces once and store the result in each Cell::sgr_.
//
//   Style{.fg_sgr = fg(Rgb{255,100,50}),
//         .bg_sgr = bg(Color4::BLACK),
//         .effects_sgr = effects({TextEffect::BOLD})}

struct Style {
    std::string fg_sgr      = vaterm::color::fg(vaterm::Color4::WHITE);
    std::string bg_sgr;                        // default: no bg (terminal native)
    std::string effects_sgr;
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

    // ── Import ──────────────────────────────────────────────────────
    // Copy cells from another Framebuffer onto this one at (off_col, off_row).
    // Wide-char fragments at the destination are cleaned before overwrite.
    // Source wide-chars that overflow the destination right edge are
    // replaced with a space in the source's style.
    void importFrame(PositionArgs offset, const Framebuffer& src);

    // ── Output ──────────────────────────────────────────────────────
    void swap();

  private:
    friend class VaTui;

    struct Cell {
        bool        isLong_    = false;
        bool        isHead_    = false;
        char32_t    cp_        = 0;
        char        data_      = ' ';
        uint32_t    sgr_id_    = 0;
    };

    Cell& cell_at_(int col, int row) {
        return buf_[row * max_col_ + col];
    }

    static void reset_cell_(Cell& cell) {
        cell.data_   = ' ';
        cell.cp_     = 0;
        cell.isLong_ = false;
        cell.isHead_ = true;
        // sgr_id_ not touched — caller sets it
    }

    // sgr_id encoding:
    //   bit 31: has_fg (1 = foreground colour set, 0 = TRANS)
    //   bit 30: has_bg (1 = background colour set, 0 = TRANS)
    //   bits 0-29: index into sgr_pool_
    static uint32_t make_sgr_id_(uint32_t index, bool has_fg, bool has_bg) {
        return (has_fg ? 0x80000000u : 0) | (has_bg ? 0x40000000u : 0) | (index & 0x3FFFFFFF);
    }

    // Intern an SGR string: return its index in the pool (deduplicates
    // across the pool) encoded with has_fg/has_bg flags in the high bits.
    uint32_t intern_sgr_(const std::string& s, bool has_fg, bool has_bg) {
        for (size_t i = 0; i < sgr_pool_.size(); ++i)
            if (sgr_pool_[i] == s)
                return make_sgr_id_(static_cast<uint32_t>(i), has_fg, has_bg);
        sgr_pool_.push_back(s);
        return make_sgr_id_(static_cast<uint32_t>(sgr_pool_.size() - 1), has_fg, has_bg);
    }

    // Extract the foreground SGR sequence from a combined SGR string.
    // Returns e.g. "\033[31m" or "" if none.
    static std::string extract_fg_sgr_(const std::string& s) {
        size_t last = s.rfind("\033[38;");           // 8/24-bit
        for (auto p : {"\033[3", "\033[9"}) {       // 4-bit 30-37 / 90-97
            auto pos = s.rfind(p);
            if (pos != std::string::npos && pos + 3 < s.size()) {
                char d = s[pos + 3];
                if (d >= '0' && d <= '7' && pos > last) last = pos;
            }
        }
        if (last == std::string::npos) return {};
        auto end = s.find('m', last);
        return s.substr(last, end + 1 - last);
    }

    // Extract the background SGR sequence from a combined SGR string.
    static std::string extract_bg_sgr_(const std::string& s) {
        size_t last = s.rfind("\033[48;");           // 8/24-bit
        for (auto p : {"\033[4", "\033[10"}) {      // 4-bit 40-47 / 100-107
            auto pos = s.rfind(p);
            if (pos != std::string::npos && pos + 3 < s.size()) {
                char d = s[pos + 3];
                if (d >= '0' && d <= '7' && pos > last) last = pos;
            }
        }
        // Exclude false match with \033[38
        if (last != std::string::npos && last + 2 < s.size() && s[last + 2] == '3')
            last = std::string::npos;  // 033[38 matched, not bg
        if (last == std::string::npos) return {};
        auto end = s.find('m', last);
        return s.substr(last, end + 1 - last);
    }

    // Extract all non-colour SGR sequences (effects) from a combined SGR.
    static std::string extract_effects_sgr_(const std::string& s) {
        std::string r;
        size_t pos = 0;
        while ((pos = s.find("\033[", pos)) != std::string::npos) {
            if (pos + 3 >= s.size()) break;
            auto end = s.find('m', pos);
            if (end == std::string::npos) break;
            auto seq = s.substr(pos, end + 1 - pos);
            bool is_color = false;
            if (seq.size() > 4 && seq[3] == '8' && s[pos + 4] == ';')
                is_color = true;                     // 38; or 48;
            else if (seq.size() >= 4) {
                char p1 = seq[2];                    // digit after '[' in "\033["
                if ((p1 == '3' || p1 == '4') && seq[3] >= '0' && seq[3] <= '7')
                    is_color = true;                 // 30-37 or 40-47
                if (seq.size() >= 5 && p1 == '1' && seq[3] == '0' && seq[4] >= '0' && seq[4] <= '7')
                    is_color = true;                 // 100-107
                if (p1 == '9' && seq[3] >= '0' && seq[3] <= '7')
                    is_color = true;                 // 90-97
            }
            if (!is_color) r += seq;
            pos = end + 1;
        }
        return r;
    }

    int                   max_col_ = 0;
    int                   max_row_ = 0;
    int                   off_col_ = 0;
    int                   off_row_ = 0;
    std::vector<Cell>     buf_;
    bool                  dirty_   = false;
    std::string           out_buf_;         // reusable swap output buffer
    std::vector<std::string> sgr_pool_;     // SGR deduplication pool
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
