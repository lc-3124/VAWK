#ifndef VATERM_COLOR_HPP
#define VATERM_COLOR_HPP

// -----------------------------------------------------------------------
//  vaterm::color —— 颜色与文本效果的 ANSI 转义序列
//
//  所有函数都返回包含原始转义序列的 std::string。
//  用户需自行把字符串写到终端，并在之后
//  发送 color::reset()（或 RESET 效果）。
//
//  颜色重载：
//    fg(bg)(Color4)    —— 4-bit（16 色调色板）
//    fg(bg)(uint8_t)   —— 8-bit（256 色立方 + 0-255 灰阶）
//    fg(bg)(r, g, b)   —— 24-bit 真彩色
//    fg(bg)(Rgb)       —— 接受结构体的便捷重载
//
//  文本效果：
//    effect(TextEffect)           —— 单个效果
//    effect({a, b, ...})          —— 用 ';' 组合的多个效果
//    reset()                      —— 重置一切
//
//  颜色空间转换：
//    rgb_to_256(r, g, b)   —— 把 24-bit 近似为 256 色下标
//    _256_to_rgb(idx)      —— 反查（返回规范 RGB 值）
//    _4_to_256(c4)         —— 把 4-bit 颜色映射为 256 色下标
//    blend_256(a, b)       —— 取两个 256 色下标 RGB 的平均
//    invert_256(idx)       —— 近似反色
// -----------------------------------------------------------------------

#include "enums.hpp"

#include <format>
#include <string>

namespace vaterm {
namespace color {

// 前景色 —— 4-bit 调色板（Color4）。
// 值 0-7 使用 30-37 区间，8-15 使用 90-97 的“明亮”区间。
inline std::string fg(Color4 c) {
    auto v = static_cast<uint8_t>(c);
    if (v < 8)
        return std::format("\033[{}m", v + 30);
    return std::format("\033[{}m", v + 82);
}

// 前景色 —— 8-bit（256 色）。参数为颜色下标（0-255）。
inline std::string fg(uint8_t c) {
    return std::format("\033[38;5;{}m", c);
}

// 前景色 —— 24-bit 真彩色，使用独立的 R、G、B 分量。
inline std::string fg(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("\033[38;2;{};{};{}m", r, g, b);
}

// 前景色 —— 接受 Rgb 结构体的便捷重载。
inline std::string fg(Rgb rgb) { return fg(rgb.r, rgb.g, rgb.b); }

// 背景色 —— 4-bit 调色板（Color4）。
// 值 0-7 使用 40-47 区间，8-15 使用 100-107 的“明亮”区间。
inline std::string bg(Color4 c) {
    auto v = static_cast<uint8_t>(c);
    if (v < 8)
        return std::format("\033[{}m", v + 40);
    return std::format("\033[{}m", v + 92);
}

// 背景色 —— 8-bit（256 色）。参数为颜色下标（0-255）。
inline std::string bg(uint8_t c) {
    return std::format("\033[48;5;{}m", c);
}

// 背景色 —— 24-bit 真彩色，使用独立的 R、G、B 分量。
inline std::string bg(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("\033[48;2;{};{};{}m", r, g, b);
}

// 背景色 —— 接受 Rgb 结构体的便捷重载。
inline std::string bg(Rgb rgb) { return bg(rgb.r, rgb.g, rgb.b); }

// 单个 SGR 文本效果（例如 BOLD、UNDERLINE）。
inline std::string effect(TextEffect e) {
    return std::format("\033[{}m", static_cast<uint8_t>(e));
}

// 把颜色与效果全部重置为终端默认值。
inline std::string reset() {
    return "\033[0m";
}

// 把多个 SGR 效果合并成一条转义序列。
// 例如：effect({TextEffect::BOLD, TextEffect::UNDERLINE})
// 会得到 "\033[1;4m"。
inline std::string effect(std::initializer_list<TextEffect> effects) {
    std::string out = "\033[";
    bool first = true;
    for (auto e : effects) {
        if (!first) out += ';';
        out += std::to_string(static_cast<uint8_t>(e));
        first = false;
    }
    out += 'm';
    return out;
}

// ---- 颜色空间转换辅助函数 ----------------------------------------------

// 把 24-bit (R,G,B) 颜色近似到最近的 256 色下标。
// 使用标准的 6×6×6 立方（下标 16-231）加上灰阶渐变
// （下标 232-255）以及 16 个基础 ANSI 颜色（下标 0-15）。
inline uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b) {
    // 灰阶快捷路径：若 R=G=B，直接映射到灰阶渐变。
    if (r == g && g == b) {
        if (r < 8) return 16;
        if (r > 238) return 231;
        return static_cast<uint8_t>(232 + (r - 8) / 10);
    }
    // 把每个分量从 0-255 映射到 0-5。
    auto ir = static_cast<uint8_t>((r * 5) / 255);
    auto ig = static_cast<uint8_t>((g * 5) / 255);
    auto ib = static_cast<uint8_t>((b * 5) / 255);
    return static_cast<uint8_t>(16 + ir * 36 + ig * 6 + ib);
}

// 反查：返回某个 256 色下标对应的规范 RGB 颜色。
// 下标 0-15 使用 ANSI 命名颜色；16-231 解包
// 6×6×6 立方；232-255 则计算出灰阶渐变值。
inline Rgb _256_to_rgb(uint8_t c) {
    // 下标 0-15：标准 ANSI 颜色（近似的 sRGB 值）。
    if (c < 16) {
        static constexpr uint8_t table[16][3] = {
            {0, 0, 0}, {128, 0, 0}, {0, 128, 0}, {128, 128, 0},
            {0, 0, 128}, {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
            {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
            {0, 0, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255},
        };
        return {table[c][0], table[c][1], table[c][2]};
    }
    // 下标 232-255：24 级灰阶渐变。
    if (c >= 232) {
        auto v = static_cast<uint8_t>(8 + (c - 232) * 10);
        return {v, v, v};
    }
    // 下标 16-231：6×6×6 颜色立方。
    auto idx = c - 16;
    auto r = static_cast<uint8_t>((idx / 36) * 51);
    auto g = static_cast<uint8_t>(((idx % 36) / 6) * 51);
    auto b = static_cast<uint8_t>((idx % 6) * 51);
    return {r, g, b};
}

// 把 4-bit 的 Color4 映射为其等价的 256 色下标
// （下标 0-15 为恒等映射）。
inline uint8_t _4_to_256(Color4 c) {
    static constexpr uint8_t table[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
    };
    return table[static_cast<uint8_t>(c)];
}

// 取两个 256 色下标 RGB 值的平均来混合它们。
inline uint8_t blend_256(uint8_t a, uint8_t b) {
    auto ra = _256_to_rgb(a);
    auto rb = _256_to_rgb(b);
    return rgb_to_256(
        static_cast<uint8_t>((static_cast<uint16_t>(ra.r) + rb.r) / 2),
        static_cast<uint8_t>((static_cast<uint16_t>(ra.g) + rb.g) / 2),
        static_cast<uint8_t>((static_cast<uint16_t>(ra.b) + rb.b) / 2)
    );
}

// 近似求某个 256 色下标的反色。
inline uint8_t invert_256(uint8_t c) {
    auto rgb = _256_to_rgb(c);
    return rgb_to_256(
        static_cast<uint8_t>(255 - rgb.r),
        static_cast<uint8_t>(255 - rgb.g),
        static_cast<uint8_t>(255 - rgb.b)
    );
}

// 为 24-bit RGB 值寻找最接近的 4-bit ANSI 颜色。
inline Color4 nearest_4bit(uint8_t r, uint8_t g, uint8_t b) {
    static constexpr uint8_t table[16][3] = {
        {0, 0, 0}, {128, 0, 0}, {0, 128, 0}, {128, 128, 0},
        {0, 0, 128}, {128, 0, 128}, {0, 128, 128}, {192, 192, 192},
        {128, 128, 128}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
        {0, 0, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255},
    };
    int best = 0;
    int best_dist = INT32_MAX;
    for (int i = 0; i < 16; ++i) {
        int dr = r - table[i][0];
        int dg = g - table[i][1];
        int db = b - table[i][2];
        int dist = dr * dr + dg * dg + db * db;
        if (dist < best_dist) { best_dist = dist; best = i; }
    }
    return static_cast<Color4>(best);
}

inline Color4 nearest_4bit(Rgb rgb) {
    return nearest_4bit(rgb.r, rgb.g, rgb.b);
}

} // namespace color
} // namespace vaterm

#endif // VATERM_COLOR_HPP
