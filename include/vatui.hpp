#ifndef VATUI_HPP
#define VATUI_HPP

// -----------------------------------------------------------------------
//  vatui —— 终端帧缓冲工具库
//
//  构建在 vaterm 之上，提供：
//    - Framebuffer  —— 双缓冲单元格矩阵、基于差异的交换，
//                      支持 UTF-8 / 宽字符、带样式的填充与文本。
//    - VaTui        —— 单例，持有一个 Framebuffer + 一个终端实例。
//
//  用法：
//    auto& tui = VaTui::instance();
//    tui.init();
//    auto& fb = tui.buffer();
//    fb.printText({.col = 0, .row = 0, .text = "Hi", .style = {...}});
//    fb.swap();
//
//  本库拆分为 hpp（声明）与 cpp（实现）两部分。
// -----------------------------------------------------------------------

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "vaterm/color.hpp"
#include "vaterm/enums.hpp"
#include "vaterm/term.hpp"

namespace vatui {

// ---- 颜色 SGR 辅助函数（立即转换） -------------------------------------
// 每次调用都会在首次使用时检测终端色深，
// 并自动把输入转换为终端原生色深。
// 在 Style 初始化器中使用：
//   Style{.fg_sgr = fg(Color4::GREEN), .bg_sgr = bg(Color8{196})}
//
// 转换规则（fg 与 bg 相同）：
//   Color4 → 直通（4-bit 在任何终端上都可用）
//   Color8 → C24: 经 _256_to_rgb 升位转换
//            C4:  经 _256_to_rgb + nearest_4bit 降位转换
//            C8:  直通
//   Rgb    → C24: 直通
//            C8:  经 rgb_to_256 降位转换
//            C4:  经 nearest_4bit 降位转换

// ---- 颜色 SGR 辅助函数（立即转换） -------------------------------------
// 每次调用都在首次使用时检测终端色深（此后缓存），
// 并自动把输入转换为终端原生色深。
// 在 Style 初始化器中使用：
//   Style{.fg_sgr = fg(Color4::GREEN), .bg_sgr = bg(Color8{196})}
//
// 转换规则（fg 与 bg 相同）：
//   Color4 → 直通（4-bit 在任何终端上都可用）
//   Color8 → C24: 经 _256_to_rgb 升位转换
//            C4:  经 _256_to_rgb + nearest_4bit 降位转换
//            C8:  直通
//   Rgb    → C24: 直通
//            C8:  经 rgb_to_256 降位转换
//            C4:  经 nearest_4bit 降位转换
//
// 实现见 src/vatui.cpp。

// 前景色（把输入转换为当前色深下的 SGR 序列）。
std::string fg(vaterm::Color4 c);
std::string fg(vaterm::Color8 c);
std::string fg(vaterm::Rgb c);
std::string fg(vaterm::Trans);

// 背景色（同上）。
std::string bg(vaterm::Color4 c);
std::string bg(vaterm::Color8 c);
std::string bg(vaterm::Rgb c);
std::string bg(vaterm::Trans);

// 把一组 SGR 效果合并成一条转义序列。
std::string effects(std::initializer_list<vaterm::TextEffect> list);

// ---- 终端色深访问点 ----------------------------------------------------
// 首次调用探测一次并缓存。set_color_depth() 可强制指定色深（测试 / 手动
// 降级时用），传 std::nullopt 则恢复自动探测。
//
// 这是全库唯一持有色深状态的地方：原先每个 fg/bg 重载都自带一个函数内
// static 缓存，彼此重复且无法在测试中注入。
vaterm::ColorDepth color_depth();
void set_color_depth(std::optional<vaterm::ColorDepth> depth);

// ---- 样式描述符（预计算的 SGR 串） -------------------------------------
// 每个颜色值在赋值时就会被转换成对应的 ANSI SGR 序列。
// fillRegion / printText 之后只需把三部分拼接一次，
// 并把结果存入每个 Cell::sgr_。
//
//   Style{.fg_sgr = fg(Rgb{255,100,50}),
//         .bg_sgr = bg(Color4::BLACK),
//         .effects_sgr = effects({TextEffect::BOLD})}

struct Style {
    std::string fg_sgr      = vaterm::color::fg(vaterm::Color4::WHITE);
    std::string bg_sgr      = vatui::bg(vaterm::TRANS);                        // 默认：无背景（沿用终端原生背景）
    std::string effects_sgr = "";
};

// ---- 参数结构体（支持指定初始化器语法） ------------------------------

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

// ---- Framebuffer（双缓冲单元格矩阵） ---------------------------------

class Framebuffer {
  public:
    // ── 缓冲尺寸 ────────────────────────────────────────────────────
    void setSize(SizeArgs args);
    int  getColMax() const { return max_col_; }
    int  getRowMax() const { return max_row_; }

    // ── 屏幕偏移 ────────────────────────────────────────────────────
    void setPosition(PositionArgs args);
    int  getOffsetCol() const { return off_col_; }
    int  getOffsetRow() const { return off_row_; }

    // ── 绘制 ────────────────────────────────────────────────────────
    void printText(TextArgs args);
    void clear();
    void clearRegion(RegionArgs args);
    void fillRegion(FillArgs args);

    // ── 导入 ────────────────────────────────────────────────────────
    // 从另一个 Framebuffer 拷贝单元格到本对象的 (off_col, off_row) 处。
    // 目的地上的宽字符碎片会在覆盖前被清理。
    // 源中越过目的地右边缘的宽字符，
    // 会用源自身样式的一个空格替换。
    void importFrame(PositionArgs offset, const Framebuffer& src);

    // ── 输出 ────────────────────────────────────────────────────────
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
        // 不动 sgr_id_——由调用方设置
    }

    // sgr_id 编码：
    //   第 31 位: has_fg（1 = 设置了前景色，0 = TRANS）
    //   第 30 位: has_bg（1 = 设置了背景色，0 = TRANS）
    //   第 0-29 位: sgr_pool_ 中的下标
    static uint32_t make_sgr_id_(uint32_t index, bool has_fg, bool has_bg) {
        return (has_fg ? 0x80000000u : 0) | (has_bg ? 0x40000000u : 0) | (index & 0x3FFFFFFF);
    }

    // 把一个 SGR 串登记进池中：返回它在池里的下标（在池内去重），
    // 并把 has_fg/has_bg 标志编码进高位。
    uint32_t intern_sgr_(const std::string& s, bool has_fg, bool has_bg) {
        for (size_t i = 0; i < sgr_pool_.size(); ++i)
            if (sgr_pool_[i] == s)
                return make_sgr_id_(static_cast<uint32_t>(i), has_fg, has_bg);
        sgr_pool_.push_back(s);
        return make_sgr_id_(static_cast<uint32_t>(sgr_pool_.size() - 1), has_fg, has_bg);
    }

    // 从合成的 SGR 串中提取前景 SGR 序列。
    // 返回形如 "\033[31m" 的串；没有则返回 ""。
    static std::string extract_fg_sgr_(const std::string& s) {
        size_t best = std::string::npos;
        // 8/24-bit：\033[38;5;N m 或 \033[38;2;R;G;B m
        {
            auto pos = s.rfind("\033[38;");
            if (pos != std::string::npos) best = pos;
        }
        // 4-bit：\033[30-37m / \033[90-97m —— 取最右匹配
        for (auto p : {"\033[3", "\033[9"}) {
            auto pos = s.rfind(p);
            if (pos != std::string::npos && pos + 3 < s.size()) {
                char d = s[pos + 3];
                if (d >= '0' && d <= '7' && (best == std::string::npos || pos > best))
                    best = pos;
            }
        }
        if (best == std::string::npos) return {};
        auto end = s.find('m', best);
        return s.substr(best, end + 1 - best);
    }

    // 从合成的 SGR 串中提取背景 SGR 序列。
    static std::string extract_bg_sgr_(const std::string& s) {
        size_t best = std::string::npos;
        // 8/24-bit：\033[48;5;N m 或 \033[48;2;R;G;B m
        {
            auto pos = s.rfind("\033[48;");
            if (pos != std::string::npos) best = pos;
        }
        // 4-bit：\033[40-47m / \033[100-107m —— 取最右匹配
        // （\033[48; 会被下面的数字判定自然排除）
        for (auto p : {"\033[4", "\033[10"}) {
            auto pos = s.rfind(p);
            if (pos != std::string::npos && pos + 3 < s.size()) {
                char d = s[pos + 3];
                if (d >= '0' && d <= '7' && (best == std::string::npos || pos > best))
                    best = pos;
            }
        }
        if (best == std::string::npos) return {};
        auto end = s.find('m', best);
        return s.substr(best, end + 1 - best);
    }

    // 从合成的 SGR 串中提取所有非颜色 SGR 序列（即效果）。
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
                is_color = true;                     // 38; 或 48;
            else if (seq.size() >= 4) {
                char p1 = seq[2];                    // "\033[" 里 '[' 之后的数字
                if ((p1 == '3' || p1 == '4') && seq[3] >= '0' && seq[3] <= '7')
                    is_color = true;                 // 30-37 或 40-47
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
    std::string           out_buf_;         // 可复用的 swap 输出缓冲
    std::vector<std::string> sgr_pool_;     // SGR 去重池
};

// ---- VaTui 单例 ------------------------------------------------------

class VaTui {
  public:
    static VaTui& instance();

    // 不可拷贝。
    VaTui(const VaTui&) = delete;
    VaTui& operator=(const VaTui&) = delete;

    // ── 生命周期 ────────────────────────────────────────────────────
    // 进入 raw 模式、隐藏光标、清屏。
    // 可安全地多次调用（幂等）。
    void init();

    // ── Framebuffer 访问 ────────────────────────────────────────────
    Framebuffer& buffer() { return fb_; }

  private:
    VaTui();
    ~VaTui();

    Framebuffer          fb_;
    vaterm::terminal     term_;
    bool                 initialized_ = false;
};

} // namespace vatui

#endif // VATUI_HPP
