#ifndef VATERM_UTF_HPP
#define VATERM_UTF_HPP

//- vaterm::utf — UTF-8 字符串工具
//
// 提供 TUI 应用中处理 UTF-8 编码文本时常用的辅助函数：由首字节推断一个
// 码点的字节长度、统计码点个数、按索引取出码点，以及计算终端显示宽度
// （基于 East Asian Width 与 Unicode 字符属性，兼顾 CJK / emoji /
// 全角字符）。
//
// 大多数函数接受 std::string_view 且从不分配内存；pop_back 是例外
// ——它就地修改 std::string。
//
// 参考：
//   RFC 3629 (UTF-8)
//   Unicode TR#11 (East Asian Width)
//   ECMA-48 / POSIX 控制字符的终端语义
//
// 实现见 src/vaterm/utf.cpp。

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace vaterm {
namespace utf {

/**
 * @brief 返回首字节位于 s 起始处的 UTF-8 码点所占的字节数。
 *  若 s[0] 不是合法 UTF-8 首字节（续字节 0x80-0xBF 或过长前缀），返回 0。
 *
 *   0x00-0x7F  → 1 (ASCII)
 *   0xC2-0xDF  → 2
 *   0xE0-0xEF  → 3
 *   0xF0-0xF7  → 4
 *
 * @param s UTF-8 字节视图，至少含一个字节。
 * @return 字节数 1-4；非法首字节返回 0。
 */
size_t char_bytes(std::string_view s);

/**
 * @brief 返回 s 中 Unicode 码点的个数（不解析字形簇，统计的是标量值）。
 * @note 非法续字节会被跳过，且不计入个数。
 * @param s UTF-8 字节视图。
 * @return 码点个数。
 */
size_t count(std::string_view s);

/**
 * @brief 返回 s 中第 i 个码点（从 0 开始）的原始字节视图（1-4 字节）。
 * @note i 越界返回空视图；非法续字节会被跳过，且不递增索引。
 * @param s   UTF-8 字节视图。
 * @param i   码点索引，从 0 开始。
 * @return 该码点的字节子视图。
 */
std::string_view at(std::string_view s, size_t i);

/**
 * @brief 就地删除 s 的最后一个码点。
 * @note 若末尾残留孤立的续字节，会先回退到最近的首字节。
 *        典型用途：Backspace 删除一个字符。
 * @param s 待修改的字符串。
 */
void pop_back(std::string& s);

/**
 * @brief 返回首字节位于 s 起始处的单个码点的终端显示宽度。
 *  宽度规则：
 *    - 控制字符（U+0000-U+001F、U+007F）→ 0
 *    - ASCII 可打印字符 → 1
 *    - East Asian Wide / Fullwidth / CJK / Emoji → 2
 *    - 其它 → 1
 *
 *  宽字符范围检测覆盖 Hangul Jamo、CJK Unified Ideographs、
 *  CJK Compatibility、Fullwidth forms、Kana Supplement、
 *  Emoticons（从 U+1F004 起）以及 CJK Extension B-H。
 *
 * @param s 单个码点的字节视图。
 * @return 列数 0、1 或 2。
 */
int char_width(std::string_view s);

/**
 * @brief 返回 s 的终端显示宽度，即对每个码点累加 char_width()。
 * @param s UTF-8 字节视图。
 * @return 总占列数。
 */
int width(std::string_view s);

} // namespace utf
} // namespace vaterm

#endif // VATERM_UTF_HPP
