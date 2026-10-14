// vaterm::color —— 声明与实现的分离版：实现集中在此处。
// 头文件 include/vaterm/color.hpp 只保留函数声明与纯转发的便捷重载。

#include "vaterm/color.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace vaterm {
namespace color {

std::string fg(Color4 c) {
    auto v = static_cast<uint8_t>(c);
    if (v < 8)
        return std::format("\033[{}m", v + 30);
    return std::format("\033[{}m", v + 82);
}

std::string fg(uint8_t c) {
    return std::format("\033[38;5;{}m", c);
}

std::string fg(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("\033[38;2;{};{};{}m", r, g, b);
}

std::string bg(Color4 c) {
    auto v = static_cast<uint8_t>(c);
    if (v < 8)
        return std::format("\033[{}m", v + 40);
    return std::format("\033[{}m", v + 92);
}

std::string bg(uint8_t c) {
    return std::format("\033[48;5;{}m", c);
}

std::string bg(uint8_t r, uint8_t g, uint8_t b) {
    return std::format("\033[48;2;{};{};{}m", r, g, b);
}

std::string effect(TextEffect e) {
    return std::format("\033[{}m", static_cast<uint8_t>(e));
}

std::string effect(std::initializer_list<TextEffect> effects) {
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

uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b) {
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

Rgb _256_to_rgb(uint8_t c) {
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

uint8_t _4_to_256(Color4 c) {
    static constexpr uint8_t table[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
    };
    return table[static_cast<uint8_t>(c)];
}

uint8_t blend_256(uint8_t a, uint8_t b) {
    auto ra = _256_to_rgb(a);
    auto rb = _256_to_rgb(b);
    return rgb_to_256(
        static_cast<uint8_t>((static_cast<uint16_t>(ra.r) + rb.r) / 2),
        static_cast<uint8_t>((static_cast<uint16_t>(ra.g) + rb.g) / 2),
        static_cast<uint8_t>((static_cast<uint16_t>(ra.b) + rb.b) / 2)
    );
}

uint8_t invert_256(uint8_t c) {
    auto rgb = _256_to_rgb(c);
    return rgb_to_256(
        static_cast<uint8_t>(255 - rgb.r),
        static_cast<uint8_t>(255 - rgb.g),
        static_cast<uint8_t>(255 - rgb.b)
    );
}

Color4 nearest_4bit(uint8_t r, uint8_t g, uint8_t b) {
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

} // namespace color
} // namespace vaterm
