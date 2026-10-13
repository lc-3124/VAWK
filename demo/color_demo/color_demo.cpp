// -----------------------------------------------------------------------
//  color_demo — 演示 vaterm::color 中的每一个颜色 API 函数
//
//  本演示覆盖：
//    - 4 位前景色（Color4 调色板，16 色）
//    - 8 位前景色（256 色立方体）
//    - 24 位真彩色前景色
//    - 背景色（4 位、8 位、24 位）
//    - 单一与组合文本效果
//    - 色彩空间转换（rgb_to_256、_256_to_rgb 等）
//    - 灰阶渐变与往返转换
//
//  运行：  ./color_demo | less -R
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <string>
#include <string_view>

using namespace vaterm;

// 打印一个粗体+下划线的分节标题。
static void section(const std::string& title) {
    printf("\n%s═══ %s ═══%s\n",
           color::effect({TextEffect::BOLD, TextEffect::UNDERLINE}).c_str(),
           title.c_str(),
           color::reset().c_str());
}

// 显示全部 16 种 4 位前景色。
static void demo_fg_4bit() {
    section("4-bit Foreground");
    for (int i = 0; i < 16; ++i) {
        printf("%s %02d %s", color::fg(static_cast<Color4>(i)).c_str(), i, color::reset().c_str());
        if ((i + 1) % 8 == 0) printf("\n");
    }
}

// 显示完整的 256 色前景立方体。
static void demo_fg_8bit() {
    section("8-bit (256-color) Foreground");
    for (int i = 0; i < 256; ++i) {
        printf("%s%3d%s", color::fg(static_cast<uint8_t>(i)).c_str(), i, color::reset().c_str());
        if ((i + 1) % 16 == 0) printf("\n");
    }
}

// 显示真彩色渐变：R 与 G 以 32 为步长变化，
// B 固定为 128。
static void demo_fg_24bit() {
    section("24-bit True Color Foreground");
    for (int r = 0; r < 256; r += 32) {
        for (int g = 0; g < 256; g += 32) {
            printf("%s██%s", color::fg(r, g, 128).c_str(), color::reset().c_str());
        }
        printf("\n");
    }
}

// 演示背景色（4 位、8 位、24 位）。
static void demo_bg() {
    section("Background Colors");
    for (int i = 0; i < 16; ++i) {
        printf("%s %02d %s", color::bg(static_cast<Color4>(i)).c_str(), i, color::reset().c_str());
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("\n");
    for (int i = 0; i < 256; i += 16) {
        printf("%s %3d %s", color::bg(static_cast<uint8_t>(i)).c_str(), i, color::reset().c_str());
    }
    printf("\n");
    printf("%s  TrueColor BG  %s\n", color::bg(100, 150, 200).c_str(), color::reset().c_str());
}

// 展示每一种单独的 SGR 文本效果。
static void demo_effects() {
    section("Text Effects");
    auto effects = {
        TextEffect::BOLD, TextEffect::DIM, TextEffect::ITALIC,
        TextEffect::UNDERLINE, TextEffect::SLOW_BLINK,
        TextEffect::REVERSE, TextEffect::CONCEAL, TextEffect::STRIKETHROUGH,
    };
    for (auto e : effects) {
        printf("%s%-15s%s\n", color::effect(e).c_str(),
               color::effect(e).c_str(), color::reset().c_str());
    }
}

// 显示组合效果（例如粗体 + 下划线）。
static void demo_multi_effects() {
    section("Multiple Effects Combined");
    printf("%sCombined Bold+Underline%s\n",
           color::effect({TextEffect::BOLD, TextEffect::UNDERLINE}).c_str(),
           color::reset().c_str());
    printf("%sBold+Italic+Reverse%s\n",
           color::effect({TextEffect::BOLD, TextEffect::ITALIC, TextEffect::REVERSE}).c_str(),
           color::reset().c_str());
}

// 测试色彩空间转换辅助函数。
static void demo_convert() {
    section("Color Space Conversion");

    // 往返转换：RGB → 256 → RGB。
    auto rgb = Rgb{200, 100, 50};
    auto idx = color::rgb_to_256(rgb.r, rgb.g, rgb.b);
    auto back = color::_256_to_rgb(idx);
    printf("rgb_to_256(%d,%d,%d)  → %3d  → rgb(%d,%d,%d)\n",
           rgb.r, rgb.g, rgb.b, idx, back.r, back.g, back.b);

    // 4 位 → 256。
    auto c4 = Color4::YELLOW;
    auto c256 = color::_4_to_256(c4);
    printf("_4_to_256(YELLOW) → %d\n", c256);

    // 混合两个 256 色索引。
    auto blended = color::blend_256(196, 21);
    printf("blend_256(RED=%d, BLUE=%d) → %d\n", 196, 21, blended);

    // 反转颜色。
    auto inverted = color::invert_256(color::_4_to_256(Color4::RED));
    printf("invert_256(RED=%d) → %d\n", color::_4_to_256(Color4::RED), inverted);
    auto invRgb = color::_256_to_rgb(inverted);
    printf("  (approx rgb(%d,%d,%d))\n", invRgb.r, invRgb.g, invRgb.b);
}

// 显示灰阶渐变（256 色索引 232-255）及其转换。
static void demo_gray_ramp() {
    section("Gray Ramp (232–255)");
    for (int i = 232; i <= 255; ++i) {
        printf("%s %3d %s", color::bg(static_cast<uint8_t>(i)).c_str(), i, color::reset().c_str());
    }
    printf("\n");
    for (int v = 0; v <= 255; v += 16) {
        auto idx = color::rgb_to_256(v, v, v);
        printf("rgb(%d,%d,%d) → 256(%d)\n", v, v, v, idx);
    }
}

int main() {
    printf("%s=== vaterm color API Demo ===%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());
    printf("(header-only: vaterm/color.hpp, vaterm/enums.hpp)\n");

    demo_fg_4bit();
    demo_fg_8bit();
    demo_fg_24bit();
    demo_bg();
    demo_effects();
    demo_multi_effects();
    demo_convert();
    demo_gray_ramp();

    printf("\n%sAll color API tests completed.%s\n",
           color::fg(Color4::GREEN).c_str(), color::reset().c_str());
    return 0;
}
