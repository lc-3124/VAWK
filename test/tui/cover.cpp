// cover.cpp — importFrame 四象限透明叠层演示
//
// 场景设计：
//   fb  : 全屏背景，从中线分 4 色块（红 # / 蓝 @ / 绿 = / 黄 .）
//         中央 30×15 矩形挖空为全透明区域（终端原生底色透过）
//   fb2 : 30×15 浮窗，黑色不透明外框 + 十字分隔（田字形）
//         内部 4 象限各演示一种 fg/bg TRANS 组合
//
// importFrame 合并规则（按每个 Cell）：
//   src_fg==true  → 使用来源格的 fg SGR 和字符
//   src_fg==false → 保留目标格的 fg SGR 和字符（fg TRANS）
//   src_bg==true  → 使用来源格的 bg SGR
//   src_bg==false → 保留目标格的 bg SGR（bg TRANS）
//   全 TRANS (both false) → 跳过该格，目标格不变
//
// 操作：WASD 移动 fb2，Q / ESC 退出
//===========================================================

#include <vatui.hpp>
#include <vaterm/term.hpp>
#include <cstdio>

using namespace vaterm;
using namespace vatui;

int main() {
    auto cols = terminal::getColMax();
    auto rows = terminal::getRowMax();
    if (cols < 52 || rows < 24) {
        std::printf("Need at least 52x24 terminal\n");
        return 0;
    }

    auto& tui = VaTui::instance();
    tui.init();
    auto& fb = tui.buffer();

    // ── fb 参数 ──────────────────────────────────────────
    // 从中线 mx/my 分成 4 块，每块一种颜色+填字符
    int mx = cols / 2, my = (rows - 1) / 2;
    int fq_x[4] = {0, mx, 0, mx};
    int fq_y[4] = {0, 0, my, my};
    int fq_w[4] = {mx, cols - mx, mx, cols - mx};
    int fq_h[4] = {my, my, rows - 1 - my, rows - 1 - my};

    // fb 4 色块定义：
    //   Q1: 白(231)字 红(196)底  填 #
    //   Q2: 白(231)字 蓝(27)底   填 @
    //   Q3: 白(231)字 绿(46)底   填 =
    //   Q4: 黑(0)  字 黄(226)底  填 .
    struct { std::string fg; std::string bg; char fill; } fb_q[4] = {
        {fg(Color8{231}), bg(Color8{196}), '#'},
        {fg(Color8{231}), bg(Color8{27}),  '@'},
        {fg(Color8{231}), bg(Color8{46}),  '='},
        {fg(Color8{0}),   bg(Color8{226}), '.'},
    };

    // ── fb2 参数 ─────────────────────────────────────────
    // 30×15，黑色不透明边框 + 十字分隔（田字），内部 4 象限
    // 宽度分布：1(左边框)+14(左象限)+1(竖分隔)+13(右象限)+1(右边框)=30
    // 高度分布：1(上边框)+6(上象限)+1(横分隔)+6(下象限)+1(下边框)=15
    int FW = 30, FH = 15;
    int o2x[4] = {1, 16, 1, 16};   // 象限左上角 x
    int o2y[4] = {1, 1, 8, 8};     // 象限左上角 y
    int o2w[4] = {14, 13, 14, 13}; // 左象限 14w，右象限 13w（保右边框）

    // fb2 4 象限定义（fg/bg 的 4 种 TRANS 组合）：
    struct { std::string fg; std::string bg; char fill; const char* label; } ov_q[4] = {
        // Q1   fg set, bg set   → 完全不透明，完整覆盖 fb
        {fg(Color8{226}), bg(Color8{88}),  '#', "OPAQUE"},
        // Q2   fg set, bg TRANS → 文字/填符以 fg 色显示，fb 底色透出
        {fg(Color8{51}),  bg(TRANS),       '@', "BG TR"},
        // Q3   fg TRANS, bg set → fb 的字符/前景透出，使用来源格 bg 色
        {fg(TRANS),       bg(Color8{201}), ' ', 0},
        // Q4   fg TRANS, bg TRANS → 全透明，fb 的格不变（hole）
        {fg(TRANS),       bg(TRANS),       ' ', 0},
    };

    // 状态栏样式
    Style sb = {.fg_sgr = fg(Color4::BLACK), .bg_sgr = bg(Color4::BRIGHT_WHITE)};

    // ── 构建 fb（背景帧缓冲）───────────────────────────
    // fbg 是一次性构建的静态背景：4 色块 + 中央透明洞
    Framebuffer fbg;
    fbg.setSize({cols, rows - 1});

    for (int i = 0; i < 4; ++i) {
        Style s = {.fg_sgr = fb_q[i].fg, .bg_sgr = fb_q[i].bg};
        fbg.fillRegion({fq_x[i], fq_y[i], fq_w[i], fq_h[i], fb_q[i].fill, s});
    }

    // 中央透明洞 = fb2 初始位置，用全空 fg/bg 使 importFrame 跳过这些格
    int hx = (cols - FW) / 2, hy = ((rows - 1) - FH) / 2;
    fbg.fillRegion({hx, hy, FW, FH, ' ', {.fg_sgr = "", .bg_sgr = ""}});

    // ── 构建 fb2（叠层帧缓冲）─────────────────────────
    Framebuffer fgo;
    fgo.setSize({FW, FH});

    // 黑色不透明边框 + 十字分隔（has_fg=has_bg=true → 永不透明）
    Style bdr = {.fg_sgr = fg(Color8{0}), .bg_sgr = bg(Color8{0})};
    fgo.fillRegion({0, 0, FW, 1, ' ', bdr});                              // 上边
    fgo.fillRegion({0, FH - 1, FW, 1, ' ', bdr});                          // 下边
    fgo.fillRegion({0, 1, 1, FH - 2, ' ', bdr});                           // 左边
    fgo.fillRegion({FW - 1, 1, 1, FH - 2, ' ', bdr});                       // 右边
    fgo.fillRegion({15, 1, 1, FH - 2, ' ', bdr});                           // 竖分隔
    fgo.fillRegion({1, 7, FW - 2, 1, ' ', bdr});                            // 横分隔

    // 内部分隔出的 4 象限，各用不同的 TRANS 模式
    for (int i = 0; i < 4; ++i) {
        Style s = {.fg_sgr = ov_q[i].fg, .bg_sgr = ov_q[i].bg};
        fgo.fillRegion({o2x[i], o2y[i], o2w[i], 6, ov_q[i].fill, s});
        // Q1/Q2 的 label 使用同象限的 fg/bg（fg 设定了 → 可见）
        if (ov_q[i].label)
            fgo.printText({.col = o2x[i] + 1, .row = o2y[i] + 1,
                           .text = ov_q[i].label, .style = s});
    }
    // Q4（全 TRANS）用不透明标签标识，否则用户看不到"HOLE"字样
    fgo.printText({.col = o2x[3] + 4, .row = o2y[3] + 2, .text = "HOLE",
                   .style = {.fg_sgr = fg(Color4::BRIGHT_WHITE),
                             .bg_sgr = bg(Color4::BLACK)}});

    // ── 主循环 ──────────────────────────────────────────
    // ox, oy: fb2 的屏幕位置（左上角），初始居中于透明洞
    int ox = hx, oy = hy;
    bool quit = false;

    while (!quit) {
        // 每帧先 clear() 清除上一帧的残影（fb2 移动后旧位置残留）
        // 再用 fbg 恢复背景，最后将 fgo 叠到新位置
        fb.clear();
        fb.importFrame({.col = 0, .row = 0}, fbg);
        fb.importFrame({.col = ox, .row = oy}, fgo);

        // 状态栏（最底行）
        char buf[120];
        std::snprintf(buf, sizeof buf,
            "  cover  fb2@(%d,%d)  Q1=OPAQ  Q2=bgTR  Q3=fgTR  Q4=HOLE  WASD=move Q=quit",
            ox, oy);
        fb.fillRegion({0, rows - 1, cols, 1, ' ', sb});
        fb.printText({.col = 0, .row = rows - 1, .text = buf, .style = sb});

        fb.swap();

        auto ev = tui.waitInput();
        if (ev.type == InputType::KEY) {
            auto& k = ev.key;
            if (k.cp == 'q' || k.cp == 'Q' || k.code == KeyCode::ESC)
                quit = true;
            if (k.cp == 'a' || k.cp == 'A') ox = ox > 0 ? ox - 1 : 0;
            if (k.cp == 'd' || k.cp == 'D') ox = ox + FW < cols ? ox + 1 : ox;
            if (k.cp == 'w' || k.cp == 'W') oy = oy > 0 ? oy - 1 : 0;
            if (k.cp == 's' || k.cp == 'S') oy = oy + FH < rows - 1 ? oy + 1 : oy;
        }
    }
}
