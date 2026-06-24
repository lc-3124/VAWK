// -----------------------------------------------------------------------
//  vatui.cpp — VaTui implementation
// -----------------------------------------------------------------------

#include "vatui.hpp"

#include "vaterm/color.hpp"
#include "vaterm/cursor.hpp"
#include "vaterm/term.hpp"
#include "vaterm/utf.hpp"

#include <poll.h>
#include <string>

// ── Anonymous-namespace helpers ────────────────────────────────────────

namespace {

bool operator==(const vatui::Style& a, const vatui::Style& b) {
    return a.fg == b.fg
        && a.bg == b.bg
        && a.effects == b.effects;
}

bool operator!=(const vatui::Style& a, const vatui::Style& b) {
    return !(a == b);
}

std::string cp_to_utf8(char32_t cp) {
    if (cp < 0x80)
        return std::string(1, static_cast<char>(cp));
    if (cp < 0x800)
        return {static_cast<char>(0xC0 | (cp >> 6)),
                static_cast<char>(0x80 | (cp & 0x3F))};
    if (cp < 0x10000)
        return {static_cast<char>(0xE0 | (cp >> 12)),
                static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
                static_cast<char>(0x80 | (cp & 0x3F))};
    return {static_cast<char>(0xF0 | (cp >> 18)),
            static_cast<char>(0x80 | ((cp >> 12) & 0x3F)),
            static_cast<char>(0x80 | ((cp >> 6) & 0x3F)),
            static_cast<char>(0x80 | (cp & 0x3F))};
}

std::string style_to_seq(const vatui::Style& s) {
    std::string out;
    out += vaterm::color::fg(s.fg);
    out += vaterm::color::bg(s.bg);
    for (auto& e : s.effects)
        out += vaterm::color::effect(e);
    return out;
}

int cp_width(char32_t cp) {
    if (cp < 0x20 || cp == 0x7F) return 0;
    if (cp >= 0x1100 && cp <= 0x115F) return 2;
    if (cp == 0x2329 || cp == 0x232A) return 2;
    if (cp >= 0x2E80 && cp <= 0xA4CF) return 2;
    if (cp >= 0xA960 && cp <= 0xA97C) return 2;
    if (cp >= 0xAC00 && cp <= 0xD7AF) return 2;
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;
    if (cp >= 0xFE10 && cp <= 0xFE19) return 2;
    if (cp >= 0xFE30 && cp <= 0xFE6F) return 2;
    if (cp >= 0xFF01 && cp <= 0xFF60) return 2;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;
    if (cp >= 0x1B000 && cp <= 0x1B0FF) return 2;
    if (cp >= 0x1F004) return 2;
    if (cp >= 0x20000 && cp <= 0x3FFFF) return 2;
    return 1;
}

// Non-blocking check for pending data on stdin.
bool data_available() {
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    return poll(&pfd, 1, 0) > 0;
}

// Parse a CSI escape sequence (between \033[ and the terminator byte)
// into a Key.
vatui::Key parse_csi(std::string_view params, char term) {
    using namespace vatui;
    if (params.empty()) {
        switch (term) {
        case 'A': return {.code = KEY_UP};
        case 'B': return {.code = KEY_DOWN};
        case 'C': return {.code = KEY_RIGHT};
        case 'D': return {.code = KEY_LEFT};
        case 'H': return {.code = KEY_HOME};
        case 'F': return {.code = KEY_END};
        }
    }
    if (term == '~') {
        int n = 0;
        for (char c : params) {
            if (c >= '0' && c <= '9') n = n * 10 + (c - '0');
            else break;
        }
        switch (n) {
        case 1:  return {.code = KEY_HOME};
        case 2:  return {.code = KEY_INS};
        case 3:  return {.code = KEY_DEL};
        case 4:  return {.code = KEY_END};
        case 5:  return {.code = KEY_PGUP};
        case 6:  return {.code = KEY_PGDN};
        case 11: return {.code = KEY_F1};
        case 12: return {.code = KEY_F2};
        case 13: return {.code = KEY_F3};
        case 14: return {.code = KEY_F4};
        case 15: return {.code = KEY_F5};
        case 17: return {.code = KEY_F6};
        case 18: return {.code = KEY_F7};
        case 19: return {.code = KEY_F8};
        case 20: return {.code = KEY_F9};
        case 21: return {.code = KEY_F10};
        case 23: return {.code = KEY_F11};
        case 24: return {.code = KEY_F12};
        }
    }
    return {};
}

// Decode a UTF-8 sequence whose lead byte is already at buf[0].
// Returns the decoded Key and sets seq_len to the number of bytes
// consumed, or returns {} and seq_len = 0 on failure.
vatui::Key decode_utf8_key(std::string_view buf, int& seq_len) {
    using namespace vatui;
    seq_len = 0;
    if (buf.empty()) return {};
    auto lead = static_cast<unsigned char>(buf[0]);
    int cont;
    char32_t min_cp;
    char32_t cp;
    if (lead < 0xE0)         { cont = 1; min_cp = 0x80;    cp = lead & 0x1F; }
    else if (lead < 0xF0)    { cont = 2; min_cp = 0x800;   cp = lead & 0x0F; }
    else if (lead < 0xF8)    { cont = 3; min_cp = 0x10000; cp = lead & 0x07; }
    else return {};
    if (static_cast<int>(buf.size()) < 1 + cont) return {};
    for (int i = 1; i <= cont; ++i) {
        auto c = static_cast<unsigned char>(buf[i]);
        if ((c & 0xC0) != 0x80) return {};
        cp = (cp << 6) | (c & 0x3F);
    }
    if (cp < min_cp) return {};
    seq_len = 1 + cont;
    return {.cp = cp};
}

} // anonymous namespace

// =====================================================================
//  Framebuffer
// =====================================================================

namespace vatui {

void Framebuffer::setSize(SizeArgs args) {
    max_col_ = args.col;
    max_row_ = args.row;
    buf_.resize(static_cast<size_t>(max_col_) * max_row_);
    clear();
}

void Framebuffer::setPosition(PositionArgs args) {
    off_col_ = args.col;
    off_row_ = args.row;
}

// ── Text output ────────────────────────────────────────────────────────

void Framebuffer::printText(TextArgs args) {
    auto  col   = args.col;
    auto  row   = args.row;
    auto& text  = args.text;
    auto& style = args.style;

    if (row >= max_row_) return;
    dirty_ = true;

    size_t pos = 0;
    while (pos < text.size() && col < max_col_) {
        // Protect multi-byte: check target cell's current state.
        {
            auto idx_p = static_cast<size_t>(row) * max_col_ + col;
            auto& cell_p = buf_[idx_p];
            if (!cell_p.isHead_ && cell_p.isLong_ && col > 0) {
                reset_cell_(buf_[idx_p - 1], cell_p.style_);
            }
            if (cell_p.isHead_ && cell_p.isLong_ && col + 1 < max_col_) {
                reset_cell_(buf_[idx_p + 1], cell_p.style_);
            }
        }

        // Decode the next codepoint.
        auto cb = vaterm::utf::char_bytes(text.substr(pos));
        if (cb == 0) { ++pos; continue; }
        if (pos + cb > text.size()) break;

        auto start = pos;
        char32_t cp = static_cast<unsigned char>(text[pos]);

        if (cb >= 2) {
            auto b1 = static_cast<unsigned char>(text[pos]);
            auto b2 = static_cast<unsigned char>(text[pos + 1]);
            if (cb == 2) {
                cp = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
            } else if (cb == 3) {
                auto b3 = static_cast<unsigned char>(text[pos + 2]);
                cp = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            } else {
                auto b3 = static_cast<unsigned char>(text[pos + 2]);
                auto b4 = static_cast<unsigned char>(text[pos + 3]);
                cp = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12)
                   | ((b3 & 0x3F) << 6)  | (b4 & 0x3F);
            }
        }
        pos += cb;

        int w = cp_width(cp);
        if (w < 1) w = 1;

        // Wide char doesn't fit at right edge → write a space with style.
        if (w > 1 && col + 1 >= max_col_) {
            auto& cell = buf_[static_cast<size_t>(row) * max_col_ + col];
            cell.data_   = ' ';
            cell.cp_     = 0;
            cell.isHead_ = true;
            cell.isLong_ = false;
            cell.style_  = style;
            col += 1;
            continue;
        }

        // Write head cell.
        auto idx = static_cast<size_t>(row) * max_col_ + col;
        auto& cell = buf_[idx];
        cell.data_   = text[start];
        cell.cp_     = cp;
        cell.isHead_ = true;
        cell.isLong_ = (w > 1);
        cell.style_  = style;

        // Wide char: occupy a second cell.
        if (w > 1 && col + 1 < max_col_) {
            size_t idx_n = idx + 1;
            auto& tail = buf_[idx_n];
            if (!tail.isHead_ && tail.isLong_)
                reset_cell_(buf_[idx_n - 1], tail.style_);
            if (tail.isHead_ && tail.isLong_ && col + 2 < max_col_)
                reset_cell_(buf_[idx_n + 1], tail.style_);

            tail.data_   = ' ';
            tail.cp_     = 0;
            tail.isLong_ = true;
            tail.isHead_ = false;
            tail.style_  = style;
            col += 2;
        } else {
            col += 1;
        }
    }
}

// ── Clear ──────────────────────────────────────────────────────────────

void Framebuffer::clear() {
    dirty_ = true;
    for (auto& cell : buf_)
        reset_cell_(cell);
}

void Framebuffer::clearRegion(RegionArgs args) {
    fillRegion({.col = args.col, .row = args.row,
                .w = args.w, .h = args.h,
                .ch = ' ', .style = {}});
}

// ── Shapes ─────────────────────────────────────────────────────────────

void Framebuffer::fillRegion(FillArgs args) {
    auto  col   = args.col;
    auto  row   = args.row;
    auto  w     = args.w;
    auto  h     = args.h;
    auto  ch    = args.ch;
    auto& style = args.style;

    dirty_ = true;
    for (int r = row; r < row + h && r < max_row_; ++r) {
        for (int c = col; c < col + w && c < max_col_; ++c) {
            {
                auto idx_f = static_cast<size_t>(r) * max_col_ + c;
                auto& cell_f = buf_[idx_f];
                if (!cell_f.isHead_ && cell_f.isLong_ && c > 0)
                    reset_cell_(buf_[idx_f - 1], cell_f.style_);
                if (cell_f.isHead_ && cell_f.isLong_ && c + 1 < max_col_)
                    reset_cell_(buf_[idx_f + 1], cell_f.style_);
            }
            auto& cell = cell_at_(c, r);
            cell.data_   = ch;
            cell.cp_     = static_cast<char32_t>(static_cast<unsigned char>(ch));
            cell.isHead_ = true;
            cell.isLong_ = false;
            cell.style_  = style;
        }
    }
}

// ── Output ─────────────────────────────────────────────────────────────

void Framebuffer::swap() {
    if (!dirty_) return;
    if (max_col_ == 0 || max_row_ == 0) return;

    std::string out;
    Style prev_style;
    int   prev_r = -1, prev_c = -1;

    for (int r = 0; r < max_row_; ++r) {
        for (int c = 0; c < max_col_; ++c) {
            auto& cell = cell_at_(c, r);
            if (!cell.isHead_) continue;

            bool seq = (r == prev_r && c == prev_c + 1);

            if (!seq || cell.style_ != prev_style) {
                if (prev_r >= 0)
                    out += vaterm::color::reset();
                if (!seq)
                    out += vaterm::cursor::move_to(
                        r + off_row_ + 1, c + off_col_ + 1);
                out += style_to_seq(cell.style_);
                prev_style = cell.style_;
            }

            if (cell.cp_ != 0)
                out += cp_to_utf8(cell.cp_);
            else
                out += cell.data_;

            prev_r = r;
            prev_c = c;
        }
    }

    if (prev_r >= 0)
        out += vaterm::color::reset();

    vaterm::terminal::write(out);
    vaterm::terminal::flush();
    dirty_ = false;
}

// =====================================================================
//  VaTui
// =====================================================================

VaTui::VaTui() {
    auto cols = vaterm::terminal::getColMax();
    auto rows = vaterm::terminal::getRowMax();
    if (cols > 0 && rows > 0)
        fb_.setSize({cols, rows});
}

VaTui::~VaTui() {
    // vaterm::terminal's destructor handles cursor show,
    // screen clear, and termios restore automatically.
}

VaTui& VaTui::instance() {
    static VaTui inst;
    return inst;
}

void VaTui::init() {
    if (initialized_) return;
    if (!term_.enter_raw()) return;
    vaterm::terminal::write(vaterm::cursor::hide());
    vaterm::terminal::write(vaterm::terminal::clear_screen());
    vaterm::terminal::flush();
    initialized_ = true;
}

// ── Unified input methods ──────────────────────────────────────────────

std::optional<Input> VaTui::getInput() {
    // Drain all available bytes into the input buffer.
    while (data_available()) {
        auto b = vaterm::terminal::read_byte();
        if (b < 0) break;
        input_buf_ += static_cast<char>(b);
    }
    if (input_buf_.empty()) return std::nullopt;

    unsigned char b0 = static_cast<unsigned char>(input_buf_[0]);

    // ── SGR mouse: \033[< ... M/m ───────────────────────────────────
    if (b0 == 0x1B && input_buf_.size() >= 3 &&
        input_buf_[1] == '[' && input_buf_[2] == '<') {
        for (size_t i = 3; i < input_buf_.size(); ++i) {
            auto c = static_cast<unsigned char>(input_buf_[i]);
            if (c == 'M' || c == 'm') {
                auto ms = vaterm::mouse::parse(
                    std::string_view(input_buf_).substr(0, i + 1));
                if (ms) {
                    input_buf_.erase(0, i + 1);
                    return Input{.type = INPUT_MOUSE, .key = {}, .mouse = *ms};
                }
                break;
            }
        }
        if (input_buf_.size() > 128) input_buf_.clear();
        return std::nullopt;
    }

    // ── CSI escape: \033[ ... (but NOT \033[<) ──────────────────────
    if (b0 == 0x1B && input_buf_.size() >= 2 && input_buf_[1] == '[') {
        for (size_t i = 2; i < input_buf_.size(); ++i) {
            auto c = static_cast<unsigned char>(input_buf_[i]);
            if (c >= 0x40 && c <= 0x7E) {
                std::string_view sv(input_buf_);
                sv.remove_prefix(2);
                char term = sv.back();
                sv.remove_suffix(1);
                auto key = parse_csi(sv, term);
                input_buf_.erase(0, i + 1);
                return Input{.type = INPUT_KEY, .key = key, .mouse = {}};
            }
        }
        return std::nullopt;
    }

    // ── SS3 sequence: \033O<c> ──────────────────────────────────────
    if (b0 == 0x1B && input_buf_.size() >= 2 && input_buf_[1] == 'O') {
        if (input_buf_.size() >= 3) {
            Key k;
            switch (input_buf_[2]) {
            case 'P': k = {.code = KEY_F1}; break;
            case 'Q': k = {.code = KEY_F2}; break;
            case 'R': k = {.code = KEY_F3}; break;
            case 'S': k = {.code = KEY_F4}; break;
            }
            input_buf_.erase(0, 3);
            return Input{.type = INPUT_KEY, .key = k, .mouse = {}};
        }
        return std::nullopt;
    }

    // ── Alt+key: \033<c> (0x20 ≤ c ≤ 0x7E) ─────────────────────────
    if (b0 == 0x1B && input_buf_.size() >= 2) {
        auto c = static_cast<unsigned char>(input_buf_[1]);
        if (c >= 0x20 && c <= 0x7E) {
            input_buf_.erase(0, 2);
            return Input{.type = INPUT_KEY,
                         .key = {.cp = static_cast<char32_t>(c), .alt = true},
                         .mouse = {}};
        }
        input_buf_.erase(0, 1);
        return std::nullopt;
    }

    // ── Bare Escape key ─────────────────────────────────────────────
    if (b0 == 0x1B) {
        input_buf_.erase(0, 1);
        return Input{.type = INPUT_KEY, .key = {.code = KEY_ESC}, .mouse = {}};
    }

    // ── Single-byte control characters ──────────────────────────────
    if (b0 == 0x09) { input_buf_.erase(0, 1); return Input{.type = INPUT_KEY, .key = {.code = KEY_TAB}, .mouse = {}}; }
    if (b0 == 0x0D) { input_buf_.erase(0, 1); return Input{.type = INPUT_KEY, .key = {.code = KEY_ENTER}, .mouse = {}}; }
    if (b0 == 0x7F) { input_buf_.erase(0, 1); return Input{.type = INPUT_KEY, .key = {.code = KEY_BACKSPACE}, .mouse = {}}; }

    // ── Printable ASCII ─────────────────────────────────────────────
    if (b0 >= 0x20 && b0 <= 0x7E) {
        input_buf_.erase(0, 1);
        return Input{.type = INPUT_KEY,
                     .key = {.cp = static_cast<char32_t>(b0)},
                     .mouse = {}};
    }

    // ── UTF-8 multi-byte ────────────────────────────────────────────
    if (b0 >= 0xC2 && b0 <= 0xF7) {
        int seq_len = 0;
        auto k = decode_utf8_key(input_buf_, seq_len);
        if (seq_len > 0) {
            input_buf_.erase(0, seq_len);
            return Input{.type = INPUT_KEY, .key = k, .mouse = {}};
        }
        return std::nullopt;
    }

    // ── Ctrl+@ / Ctrl+Space (NUL, 0x00) ────────────────────────────
    if (b0 == 0x00) {
        input_buf_.erase(0, 1);
        return Input{.type = INPUT_KEY,
                     .key = {.cp = U' ', .ctrl = true},
                     .mouse = {}};
    }

    // ── Ctrl+a … Ctrl+z (0x01–0x1A) ────────────────────────────────
    if (b0 >= 0x01 && b0 <= 0x1A) {
        input_buf_.erase(0, 1);
        return Input{.type = INPUT_KEY,
                     .key = {.cp = static_cast<char32_t>('a' + b0 - 1),
                             .ctrl = true},
                     .mouse = {}};
    }

    // ── Ctrl+\ … Ctrl+_ (0x1C–0x1F) ────────────────────────────────
    if (b0 >= 0x1C && b0 <= 0x1F) {
        input_buf_.erase(0, 1);
        auto cp = b0 == 0x1C ? U'\\'
                : b0 == 0x1D ? U']'
                : b0 == 0x1E ? U'^' : U'_';
        return Input{.type = INPUT_KEY,
                     .key = {.cp = cp, .ctrl = true},
                     .mouse = {}};
    }

    // Unrecognised — discard one byte.
    input_buf_.erase(0, 1);
    return std::nullopt;
}

Input VaTui::waitInput() {
    while (true) {
        if (auto inp = getInput()) return *inp;
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        poll(&pfd, 1, 50);
    }
}

// ── Mouse enable / disable ─────────────────────────────────────────────

void VaTui::enableMouse() {
    if (!initialized_) return;
    vaterm::terminal::write(vaterm::mouse::enable());
    vaterm::terminal::flush();
}

void VaTui::disableMouse() {
    if (!initialized_) return;
    vaterm::terminal::write(vaterm::mouse::disable());
    vaterm::terminal::flush();
}

} // namespace vatui
