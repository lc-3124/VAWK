//- vatui.cpp — VaTui 实现

#include "vatui.hpp"

#include "vaterm/color.hpp"
#include "vaterm/cursor.hpp"
#include "vaterm/term.hpp"
#include "vaterm/utf.hpp"

#include <string>

//- 匿名命名空间辅助

namespace {

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

//- 颜色降位
// 把 256 色 / 真彩色按目标色深折算成 SGR 序列。
// ch 决定序列用于前景还是背景——fg() 与 bg() 唯一的差别就在这一步。
enum class SGR_Channel { FG, BG };

std::string sgr_indexed(vaterm::Color8 c, vaterm::ColorDepth depth, SGR_Channel ch) {
    if (depth >= vaterm::ColorDepth::C24) {
        auto rgb = vaterm::color::_256_to_rgb(c.index);
        return ch == SGR_Channel::FG ? vaterm::color::fg(rgb) : vaterm::color::bg(rgb);
    }
    if (depth == vaterm::ColorDepth::C4) {
        auto rgb = vaterm::color::_256_to_rgb(c.index);
        auto c4  = vaterm::color::nearest_4bit(rgb.r, rgb.g, rgb.b);
        return ch == SGR_Channel::FG ? vaterm::color::fg(c4) : vaterm::color::bg(c4);
    }
    return ch == SGR_Channel::FG ? vaterm::color::fg(c.index) : vaterm::color::bg(c.index);
}

std::string sgr_rgb(vaterm::Rgb c, vaterm::ColorDepth depth, SGR_Channel ch) {
    if (depth >= vaterm::ColorDepth::C24) {
        return ch == SGR_Channel::FG ? vaterm::color::fg(c) : vaterm::color::bg(c);
    }
    if (depth == vaterm::ColorDepth::C8) {
        auto idx = vaterm::color::rgb_to_256(c.r, c.g, c.b);
        return ch == SGR_Channel::FG ? vaterm::color::fg(idx) : vaterm::color::bg(idx);
    }
    auto c4 = vaterm::color::nearest_4bit(c.r, c.g, c.b);
    return ch == SGR_Channel::FG ? vaterm::color::fg(c4) : vaterm::color::bg(c4);
}

// 终端色深状态：g_forced 优先；否则首次探测后缓存进 g_probed。
std::optional<vaterm::ColorDepth> g_forced, g_probed;

} // 匿名命名空间

//- Framebuffer

namespace vatui {

// 当前终端色深：被 set_color_depth() 强制指定时直接返回，
// 否则首次调用探测一次并缓存。
vaterm::ColorDepth color_depth() {
    if (g_forced) return *g_forced;
    if (!g_probed) g_probed = vaterm::terminal::detect_color_depth();
    return *g_probed;
}

// 强制指定色深（测试 / 手动降级）；传 nullopt 恢复自动探测。
void set_color_depth(std::optional<vaterm::ColorDepth> depth) {
    g_forced = depth;
    if (!depth) g_probed.reset();
}

std::string fg(vaterm::Color4 c) { return vaterm::color::fg(c); }

std::string fg(vaterm::Color8 c) {
    return sgr_indexed(c, color_depth(), SGR_Channel::FG);
}

std::string fg(vaterm::Rgb c) {
    return sgr_rgb(c, color_depth(), SGR_Channel::FG);
}

std::string fg(vaterm::Trans) { return std::string{}; }

std::string bg(vaterm::Color4 c) { return vaterm::color::bg(c); }

std::string bg(vaterm::Color8 c) {
    return sgr_indexed(c, color_depth(), SGR_Channel::BG);
}

std::string bg(vaterm::Rgb c) {
    return sgr_rgb(c, color_depth(), SGR_Channel::BG);
}

std::string bg(vaterm::Trans) { return std::string{}; }

std::string effects(std::initializer_list<vaterm::TextEffect> list) {
    return vaterm::color::effect(list);
}

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

//- 文本输出

void Framebuffer::printText(TextArgs args) {
    auto  col   = args.col;
    auto  row   = args.row;
    auto& text  = args.text;
    auto& style = args.style;

    if (row >= max_row_) return;
    dirty_ = true;

    uint32_t sid = intern_sgr_(style.fg_sgr + style.bg_sgr + style.effects_sgr,
                               !style.fg_sgr.empty(), !style.bg_sgr.empty());

    size_t pos = 0;
    while (pos < text.size() && col < max_col_) {
        // 保护多字节字符：检查目标单元格的当前状态。
        {
            auto idx_p = static_cast<size_t>(row) * max_col_ + col;
            auto& cell_p = buf_[idx_p];
            if (!cell_p.isHead_ && cell_p.isLong_ && col > 0) {
                auto old = buf_[idx_p - 1].sgr_id_;
                reset_cell_(buf_[idx_p - 1]);
                buf_[idx_p - 1].sgr_id_ = old;
            }
            if (cell_p.isHead_ && cell_p.isLong_ && col + 1 < max_col_) {
                auto old = buf_[idx_p + 1].sgr_id_;
                reset_cell_(buf_[idx_p + 1]);
                buf_[idx_p + 1].sgr_id_ = old;
            }
        }

        // 解码下一个码点。
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

        // 宽字符在右边缘放不下 → 用带样式的空格占位。
        if (w > 1 && col + 1 >= max_col_) {
            auto& cell = buf_[static_cast<size_t>(row) * max_col_ + col];
            cell.data_   = ' ';
            cell.cp_     = 0;
            cell.isHead_ = true;
            cell.isLong_ = false;
            cell.sgr_id_ = sid;
            col += 1;
            continue;
        }

        // 写入头单元格。
        auto idx = static_cast<size_t>(row) * max_col_ + col;
        auto& cell = buf_[idx];
        cell.data_   = text[start];
        cell.cp_     = cp;
        cell.isHead_ = true;
        cell.isLong_ = (w > 1);
        cell.sgr_id_ = sid;

        // 宽字符：再占用一个单元格。
        if (w > 1 && col + 1 < max_col_) {
            size_t idx_n = idx + 1;
            auto& tail = buf_[idx_n];
            if (!tail.isHead_ && tail.isLong_) {
                auto old = buf_[idx_n - 1].sgr_id_;
                reset_cell_(buf_[idx_n - 1]);
                buf_[idx_n - 1].sgr_id_ = old;
            }
            if (tail.isHead_ && tail.isLong_ && col + 2 < max_col_) {
                auto old = buf_[idx_n + 1].sgr_id_;
                reset_cell_(buf_[idx_n + 1]);
                buf_[idx_n + 1].sgr_id_ = old;
            }

            tail.data_   = ' ';
            tail.cp_     = 0;
            tail.isLong_ = true;
            tail.isHead_ = false;
            tail.sgr_id_ = sid;
            col += 2;
        } else {
            col += 1;
        }
    }
}

//- 清屏

void Framebuffer::clear() {
    dirty_ = true;
    sgr_pool_.clear();
    uint32_t sid = intern_sgr_(vaterm::color::fg(vaterm::Color4::WHITE),
                               true, false);   // 有前景色，无背景色
    for (auto& cell : buf_) {
        reset_cell_(cell);
        cell.sgr_id_ = sid;
    }
}

void Framebuffer::clearRegion(RegionArgs args) {
    fillRegion({.col = args.col, .row = args.row,
                .w = args.w, .h = args.h,
                .ch = ' ', .style = {}});
}

//- 图形

void Framebuffer::fillRegion(FillArgs args) {
    auto  col   = args.col;
    auto  row   = args.row;
    auto  w     = args.w;
    auto  h     = args.h;
    auto  ch    = args.ch;
    auto& style = args.style;

    dirty_ = true;
    uint32_t sid = intern_sgr_(style.fg_sgr + style.bg_sgr + style.effects_sgr,
                               !style.fg_sgr.empty(), !style.bg_sgr.empty());

    for (int r = row; r < row + h && r < max_row_; ++r) {
        for (int c = col; c < col + w && c < max_col_; ++c) {
            {
                auto idx_f = static_cast<size_t>(r) * max_col_ + c;
                auto& cell_f = buf_[idx_f];
                if (!cell_f.isHead_ && cell_f.isLong_ && c > 0) {
                    auto old = buf_[idx_f - 1].sgr_id_;
                    reset_cell_(buf_[idx_f - 1]);
                    buf_[idx_f - 1].sgr_id_ = old;
                }
                if (cell_f.isHead_ && cell_f.isLong_ && c + 1 < max_col_) {
                    auto old = buf_[idx_f + 1].sgr_id_;
                    reset_cell_(buf_[idx_f + 1]);
                    buf_[idx_f + 1].sgr_id_ = old;
                }
            }
            auto& cell = cell_at_(c, r);
            cell.data_   = ch;
            cell.cp_     = static_cast<char32_t>(static_cast<unsigned char>(ch));
            cell.isHead_ = true;
            cell.isLong_ = false;
            cell.sgr_id_ = sid;
        }
    }
}

//- 帧导入

void Framebuffer::importFrame(PositionArgs offset, const Framebuffer& src) {
    if (&src == this) return;

    for (int sr = 0; sr < src.max_row_; ++sr) {
        int dr = offset.row + sr;
        if (dr < 0 || dr >= max_row_) continue;

        for (int sc = 0; sc < src.max_col_; ++sc) {
            int dc = offset.col + sc;
            if (dc < 0 || dc >= max_col_) continue;

            const auto& src_cell = src.buf_[sr * src.max_col_ + sc];
            if (!src_cell.isHead_) continue;

            bool src_fg = (src_cell.sgr_id_ >> 31) != 0;
            bool src_bg = (src_cell.sgr_id_ >> 30) & 1;

            // 完全透明：完整保留目标单元格。
            if (!src_fg && !src_bg) continue;

            auto& dst_cell = cell_at_(dc, dr);

            //-- 头部位置处的碎片清理
            if (!dst_cell.isHead_ && dst_cell.isLong_ && dc > 0) {
                auto old = cell_at_(dc - 1, dr).sgr_id_;
                reset_cell_(cell_at_(dc - 1, dr));
                cell_at_(dc - 1, dr).sgr_id_ = old;
            }
            if (dst_cell.isHead_ && dst_cell.isLong_ && dc + 1 < max_col_) {
                auto old = cell_at_(dc + 1, dr).sgr_id_;
                reset_cell_(cell_at_(dc + 1, dr));
                cell_at_(dc + 1, dr).sgr_id_ = old;
            }

            const auto& src_sgr = src.sgr_pool_[src_cell.sgr_id_ & 0x3FFFFFFF];
            const auto& dst_sgr = sgr_pool_[dst_cell.sgr_id_ & 0x3FFFFFFF];

            // 构建合并后的 SGR：按 TRANS 决定 fg/bg 取自 src 还是 dst。
            std::string merged;
            if (src_fg) merged += extract_fg_sgr_(src_sgr);
            else        merged += extract_fg_sgr_(dst_sgr);
            if (src_bg) merged += extract_bg_sgr_(src_sgr);
            else        merged += extract_bg_sgr_(dst_sgr);
            merged += extract_effects_sgr_(src_sgr);

            // 右边缘溢出：用合并后的样式写一个空格。
            int w = src_cell.isLong_ ? 2 : 1;
            if (dc + w > max_col_) {
                uint32_t sid = intern_sgr_(merged, src_fg, src_bg);
                reset_cell_(dst_cell);
                dst_cell.sgr_id_ = sid;
                dirty_ = true;
                continue;
            }

            uint32_t sid = intern_sgr_(merged, src_fg, src_bg);
            reset_cell_(dst_cell);
            dst_cell.sgr_id_ = sid;
            dst_cell.data_   = src_fg ? src_cell.data_ : dst_cell.data_;
            dst_cell.cp_     = src_fg ? src_cell.cp_   : dst_cell.cp_;
            dst_cell.isLong_ = src_cell.isLong_;
            dst_cell.isHead_ = true;
            dirty_ = true;

            if (src_cell.isLong_ && dc + 1 < max_col_) {
                auto& dst_tail = cell_at_(dc + 1, dr);
                if (dst_tail.isHead_ && dst_tail.isLong_ && dc + 2 < max_col_) {
                    auto old = cell_at_(dc + 2, dr).sgr_id_;
                    reset_cell_(cell_at_(dc + 2, dr));
                    cell_at_(dc + 2, dr).sgr_id_ = old;
                }
                const auto& src_tail = src.buf_[sr * src.max_col_ + sc + 1];
                reset_cell_(dst_tail);
                dst_tail.sgr_id_ = sid;
                dst_tail.data_   = src_fg ? src_tail.data_ : ' ';
                dst_tail.cp_     = src_fg ? src_tail.cp_   : 0;
                dst_tail.isLong_ = true;
                dst_tail.isHead_ = false;
            }
        }
    }
}

//- 输出

void Framebuffer::swap() {
    if (!dirty_) return;
    if (max_col_ == 0 || max_row_ == 0) return;

    out_buf_.clear();
    uint32_t prev_sid = static_cast<uint32_t>(-1);
    int   prev_r = -1, prev_c = -1;

    for (int r = 0; r < max_row_; ++r) {
        for (int c = 0; c < max_col_; ++c) {
            auto& cell = cell_at_(c, r);
            if (!cell.isHead_) continue;

            bool seq = (r == prev_r && c == prev_c + 1);

            if (!seq || cell.sgr_id_ != prev_sid) {
                if (prev_r >= 0)
                    out_buf_ += vaterm::color::reset();
                if (!seq)
                    out_buf_ += vaterm::cursor::move_to(
                        r + off_row_ + 1, c + off_col_ + 1);
                out_buf_ += sgr_pool_[cell.sgr_id_ & 0x3FFFFFFF];
                prev_sid = cell.sgr_id_;
            }

            if (cell.cp_ != 0)
                out_buf_ += cp_to_utf8(cell.cp_);
            else
                out_buf_ += cell.data_;

            prev_r = r;
            prev_c = c;
        }
    }

    if (prev_r >= 0)
        out_buf_ += vaterm::color::reset();

    vaterm::terminal::write(out_buf_);
    vaterm::terminal::flush();
    dirty_ = false;
}

//- VaTui

VaTui::VaTui() {
    auto cols = vaterm::terminal::getColMax();
    auto rows = vaterm::terminal::getRowMax();
    if (cols > 0 && rows > 0)
        fb_.setSize({cols, rows});
}

VaTui::~VaTui() {
    // vaterm::terminal 的析构函数会自动处理光标显示、
    // 清屏以及 termios 还原。
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

} // namespace vatui
