// -----------------------------------------------------------------------
//  who_is_gay.cpp — VAWK 演示：「谁最给？」问卷（浅色主题）
//
//  用 VAWK 现有的能力拼一个接近 GUI 观感的界面：
//    - vatui::Framebuffer  — 整屏重绘：黑色页面底 / 白色卡片 / 标题条 /
//                            选中行高亮 / 底部状态栏 + 按钮
//    - vaterm::input       — 输入引擎（dumper 按序取事件）
//
//  因为 vawk 的组件层（widget/window）还没实现，布局由 layout()/draw_*()
//  手工完成。配色：黑色页面底衬托白色卡片，蓝色标题条与选中行。
//
//  键盘：上下键选择，Enter 确认，Q / Esc / Ctrl+C 退出；
//        结果页 Enter 或 R 再来一次。
//
//  注：本程序是纯娱乐 demo，与人无关。
// -----------------------------------------------------------------------

#include <vatui.hpp>

#include <vaterm/enums.hpp>
#include <vaterm/input.hpp>
#include <vaterm/term.hpp>
#include <vaterm/utf.hpp>

#include <algorithm>
#include <string>
#include <string_view>

namespace {

using vaterm::Rgb;

// ── 浅色主题调色板 ─────────────────────────────────────────────────────
constexpr Rgb PAGE      {  0,   0,   0};   // 页面底（黑）
constexpr Rgb PAGE_TX   {230, 233, 240};   // 页面底上的文字
constexpr Rgb CARD      {255, 255, 255};   // 卡片
constexpr Rgb HEADER    { 76, 111, 255};   // 标题条（品牌蓝）
constexpr Rgb HEADER_TX {255, 255, 255};
constexpr Rgb TEXT      { 34,  38,  46};   // 正文
constexpr Rgb MUTED     {126, 134, 148};   // 次要文字
constexpr Rgb SEL_BG    {228, 235, 255};   // 选中行 / 结果面板底色
constexpr Rgb SEL_TX    { 40,  70, 200};
constexpr Rgb SEL_BAR   { 76, 111, 255};   // 选中行左侧色条
constexpr Rgb FOOT_BG   {246, 247, 251};   // 状态栏
constexpr Rgb OK        { 26, 150,  80};   // 成功绿
constexpr Rgb WHITE     {255, 255, 255};

// 组装一份 Style：始终带上前景与背景，避免文字覆盖掉底色的情形。
vatui::Style style(Rgb f, Rgb b, bool bold = false) {
    vatui::Style s;
    s.fg_sgr = vatui::fg(f);
    s.bg_sgr = vatui::bg(b);
    if (bold) s.effects_sgr = vatui::effects({vaterm::TextEffect::BOLD});
    return s;
}

// ── 选项 ───────────────────────────────────────────────────────────────
struct Item {
    std::string_view label;    ///< 列表里显示的文字
    std::string_view result;   ///< 选中后「恭喜！<result>」里的部分
};

constexpr Item kItems[] = {
    {"Rx is gay !",      "Rx 是给！"},
    {"Miss ko is gay !", "流畅 是给！"},
    {"Lc3124 is gay !",  "费世玉 是给！"},
    {"宋翌豪 是给！",    "宋翌豪 是给！"},
    {"All gay",          "所有人都是给！"},
};
constexpr int kCount = static_cast<int>(sizeof(kItems) / sizeof(kItems[0]));

} // namespace

// ── 应用 ───────────────────────────────────────────────────────────────

class QuizApp {
  public:
    vatui::Framebuffer& fb;
    vaterm::input       in;

    explicit QuizApp(vatui::Framebuffer& buffer) : fb(buffer) {}

    int run() {
        if (!in.start()) {
            vaterm::terminal::write("本程序需要真实终端(tty)，请在终端里运行。\r\n");
            return 1;
        }
        in.disableMouse();   // 键盘问卷：关掉鼠标追踪，保留终端正常的鼠标选择

        vaterm::input::dumper du(in);
        du.reset();

        while (running_) {
            bool resized = sync_size();
            if (need_redraw_ || resized) {
                draw();
                fb.swap();
                need_redraw_ = false;
            }

            // 150ms 轮询：有按键立即返回，没按键就顺带检查一下终端尺寸变化。
            auto ev = du.dump(150);
            if (ev.inputType == vaterm::InputData::InputType::NON) {
                if (in.getState() == vaterm::input::InputMode::OFF) break;
                continue;
            }
            if (ev.inputType == vaterm::InputData::InputType::KEYBD)
                handle(ev.value.keybd);
        }
        in.stop();
        return 0;
    }

  private:
    enum class Mode { Menu, Result };

    Mode mode_        = Mode::Menu;
    int  sel_         = 0;
    bool running_     = true;
    bool need_redraw_ = true;

    int cols_ = -1, rows_ = -1;         // 终端尺寸
    int cx_ = 0, cy_ = 0, cw_ = 0, ch_ = 0;   // 卡片在屏幕上的位置与大小

    static constexpr int CARD_MAX_W = 62;
    static constexpr int CARD_H     = 11;

    // 尺寸变化时重建帧缓冲并清屏；无变化返回 false。
    bool sync_size() {
        int c = vaterm::terminal::getColMax();
        int r = vaterm::terminal::getRowMax();
        if (c == cols_ && r == rows_) return false;
        cols_ = c;
        rows_ = r;
        if (c > 0 && r > 0) fb.setSize({c, r});
        vaterm::terminal::write(vaterm::terminal::clear_screen());
        need_redraw_ = true;
        return true;
    }

    // 在 [x0, x0+w) 区间内水平居中打印一段文字。
    void center_in(int row, int x0, int w, std::string_view text, vatui::Style s) {
        int tw = vaterm::utf::width(text);
        int c  = x0 + (w - tw) / 2;
        if (c < x0) c = x0;
        fb.printText({.col = c, .row = row, .text = text, .style = s});
    }

    void layout() {
        cw_ = std::min(CARD_MAX_W, cols_ - 4);
        ch_ = CARD_H;
        cx_ = (cols_ - cw_) / 2;
        cy_ = (rows_ - ch_) / 2;
    }

    void draw() {
        fb.fillRegion({.col = 0, .row = 0, .w = cols_, .h = rows_, .ch = ' ',
                       .style = style(PAGE, PAGE)});

        if (cols_ < 40 || rows_ < 13) {
            center_in((rows_ - 1) / 2, 0, cols_, "窗口太小，请放大终端",
                      style(PAGE_TX, PAGE, true));
            return;
        }

        layout();

        // 白色卡片
        fb.fillRegion({.col = cx_, .row = cy_, .w = cw_, .h = ch_,
                       .ch = ' ', .style = style(CARD, CARD)});

        // 标题条 + 标题
        fb.fillRegion({.col = cx_, .row = cy_, .w = cw_, .h = 1,
                       .ch = ' ', .style = style(HEADER, HEADER)});
        fb.printText({.col = cx_ + 3, .row = cy_, .text = "VAWK - 谁最给？",
                      .style = style(HEADER_TX, HEADER, true)});

        if (mode_ == Mode::Menu) draw_menu();
        else                     draw_result();
        draw_footer();
    }

    void draw_menu() {
        center_in(cy_ + 2, cx_, cw_, "请选出最「给」的一位：", style(TEXT, CARD, true));
        center_in(cy_ + 3, cx_, cw_, "上下键移动，Enter 确认", style(MUTED, CARD));

        for (int i = 0; i < kCount; ++i) {
            int  row = cy_ + 5 + i;
            bool on  = (i == sel_);
            Rgb  f   = on ? SEL_TX : TEXT;
            Rgb  b   = on ? SEL_BG : CARD;

            if (on) {   // 选中行：整行浅蓝底 + 左侧蓝色色条
                fb.fillRegion({.col = cx_ + 1, .row = row, .w = cw_ - 2, .h = 1,
                               .ch = ' ', .style = style(SEL_BG, SEL_BG)});
                fb.fillRegion({.col = cx_ + 1, .row = row, .w = 1, .h = 1,
                               .ch = ' ', .style = style(SEL_BAR, SEL_BAR)});
            }
            fb.printText({.col = cx_ + 3, .row = row, .text = on ? "[>]" : "[ ]",
                          .style = style(f, b, on)});
            fb.printText({.col = cx_ + 8, .row = row, .text = kItems[i].label,
                          .style = style(f, b, on)});
        }
    }

    void draw_result() {
        // 结果面板：一块浅蓝底，居中放两行大字。
        fb.fillRegion({.col = cx_ + 4, .row = cy_ + 3, .w = cw_ - 8, .h = 5,
                       .ch = ' ', .style = style(SEL_BG, SEL_BG)});
        center_in(cy_ + 4, cx_, cw_, "恭喜！", style(OK, SEL_BG, true));
        center_in(cy_ + 5, cx_, cw_, kItems[sel_].result, style(SEL_TX, SEL_BG, true));
        center_in(cy_ + 8, cx_, cw_, "本问卷纯属娱乐，无任何科学依据。",
                  style(MUTED, CARD));
    }

    void draw_footer() {
        int fr = cy_ + ch_ - 1;
        fb.fillRegion({.col = cx_, .row = fr, .w = cw_, .h = 1,
                       .ch = ' ', .style = style(FOOT_BG, FOOT_BG)});

        const char* hint = (mode_ == Mode::Menu)
                         ? "上下键选择 | Enter 确认 | Q 退出"
                         : "Enter / R 再玩一次 | Q 退出";
        fb.printText({.col = cx_ + 3, .row = fr, .text = hint,
                      .style = style(MUTED, FOOT_BG)});

        // 右侧一个实心按钮
        const char* btn = (mode_ == Mode::Menu) ? " 确认 " : " 重来 ";
        int bw = vaterm::utf::width(btn);
        int bx = cx_ + cw_ - bw - 2;
        fb.fillRegion({.col = bx, .row = fr, .w = bw, .h = 1,
                       .ch = ' ', .style = style(HEADER, HEADER)});
        fb.printText({.col = bx, .row = fr, .text = btn,
                      .style = style(WHITE, HEADER, true)});
    }

    void handle(const vaterm::InputData::KeybdData& k) {
        using KC = vaterm::InputData::KeyCode;

        // 退出：Ctrl+C / Esc / Q
        if ((k.isCtrl && (k.cp == U'c' || k.cp == U'C'))
            || k.code == KC::ESC || k.cp == U'q' || k.cp == U'Q') {
            running_ = false;
            return;
        }

        if (mode_ == Mode::Menu) {
            if (k.code == KC::UP)         sel_ = (sel_ + kCount - 1) % kCount;
            else if (k.code == KC::DOWN)  sel_ = (sel_ + 1) % kCount;
            else if (k.code == KC::ENTER) mode_ = Mode::Result;
        } else {
            if (k.code == KC::ENTER || k.cp == U'r' || k.cp == U'R')
                mode_ = Mode::Menu;
        }
        need_redraw_ = true;
    }
};

int main() {
    auto& tui = vatui::VaTui::instance();
    tui.init();

    QuizApp app(tui.buffer());
    return app.run();
}
