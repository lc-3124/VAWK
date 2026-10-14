#ifndef VATERM_COLOR_HPP
#define VATERM_COLOR_HPP

//- vaterm::color —— 颜色与文本效果的 ANSI 转义序列
//
// 所有函数都返回包含原始转义序列的 std::string。
// 用户需自行把字符串写到终端，并在之后
// 发送 color::reset()（或 RESET 效果）。
//
// 颜色重载：
//   fg(bg)(Color4)    —— 4-bit（16 色调色板）
//   fg(bg)(uint8_t)   —— 8-bit（256 色立方 + 0-255 灰阶）
//   fg(bg)(r, g, b)   —— 24-bit 真彩色
//   fg(bg)(Rgb)       —— 接受结构体的便捷重载
//
// 文本效果：
//   effect(TextEffect)           —— 单个效果
//   effect({a, b, ...})          —— 用 ';' 组合的多个效果
//   reset()                      —— 重置一切
//
// 颜色空间转换：
//   rgb_to_256(r, g, b)   —— 把 24-bit 近似为 256 色下标
//   _256_to_rgb(idx)      —— 反查（返回规范 RGB 值）
//   _4_to_256(c4)         —— 把 4-bit 颜色映射为 256 色下标
//   blend_256(a, b)       —— 取两个 256 色下标 RGB 的平均
//   invert_256(idx)       —— 近似反色
//
// 实现见 src/vaterm/color.cpp；下面标注 inline 的便捷重载
// 是纯转发，留在头内。

#include "enums.hpp"

#include <format>
#include <initializer_list>
#include <string>

namespace vaterm {
namespace color {

//- 前景色

/**
 * @brief 前景色：4-bit 调色板。值 0-7 用 30-37，8-15 用 90-97 的"明亮"区间。
 */
std::string fg(Color4 c);

/**
 * @brief 前景色：8-bit（256 色），参数为颜色下标 0-255。
 */
std::string fg(uint8_t c);

/**
 * @brief 前景色：24-bit 真彩色，使用独立的 R、G、B 分量。
 */
std::string fg(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 前景色：接受 Rgb 结构体的便捷重载。
 */
inline std::string fg(Rgb rgb) { return fg(rgb.r, rgb.g, rgb.b); }

//- 背景色

/**
 * @brief 背景色：4-bit 调色板。值 0-7 用 40-47，8-15 用 100-107 的"明亮"区间。
 */
std::string bg(Color4 c);

/**
 * @brief 背景色：8-bit（256 色），参数为颜色下标 0-255。
 */
std::string bg(uint8_t c);

/**
 * @brief 背景色：24-bit 真彩色，使用独立的 R、G、B 分量。
 */
std::string bg(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 背景色：接受 Rgb 结构体的便捷重载。
 */
inline std::string bg(Rgb rgb) { return bg(rgb.r, rgb.g, rgb.b); }

//- 文本效果

/**
 * @brief 单个 SGR 文本效果（例如 BOLD、UNDERLINE）。
 */
std::string effect(TextEffect e);

/**
 * @brief 把颜色与效果全部重置为终端默认值。
 */
inline std::string reset() { return "\033[0m"; }

/**
 * @brief 把多个 SGR 效果合并成一条转义序列。
 *  例如 effect({TextEffect::BOLD, TextEffect::UNDERLINE}) 会得到 "\033[1;4m"。
 */
std::string effect(std::initializer_list<TextEffect> effects);

//- 颜色空间转换辅助函数

/**
 * @brief 把 24-bit (R,G,B) 近似到最近的 256 色下标。
 *  使用标准 6×6×6 立方（下标 16-231）加上灰阶渐变（下标 232-255）
 *  以及 16 个基础 ANSI 颜色（下标 0-15）。
 * @return 256 色下标 0-255。
 */
uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 反查：返回某个 256 色下标对应的规范 RGB 颜色。
 *  下标 0-15 用 ANSI 命名颜色；16-231 解包 6×6×6 立方；
 *  232-255 计算出灰阶渐变值。
 * @return 规范 RGB 值。
 */
Rgb _256_to_rgb(uint8_t c);

/**
 * @brief 把 4-bit 的 Color4 映射为其等价的 256 色下标（0-15 恒等）。
 * @return 256 色下标。
 */
uint8_t _4_to_256(Color4 c);

/**
 * @brief 取两个 256 色下标 RGB 值的平均来混合它们。
 * @return 混合后的 256 色下标。
 */
uint8_t blend_256(uint8_t a, uint8_t b);

/**
 * @brief 近似求某个 256 色下标的反色。
 * @return 反色的 256 色下标。
 */
uint8_t invert_256(uint8_t c);

/**
 * @brief 为 24-bit RGB 值寻找最接近的 4-bit ANSI 颜色。
 * @return 最接近的 Color4。
 */
Color4 nearest_4bit(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief  接受 Rgb 结构体的便捷重载。
 * @return 最接近的 Color4。
 */
inline Color4 nearest_4bit(Rgb rgb) { return nearest_4bit(rgb.r, rgb.g, rgb.b); }

} // namespace color
} // namespace vaterm

#endif // VATERM_COLOR_HPP
