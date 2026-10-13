#ifndef VATERM_UTF_HPP
#define VATERM_UTF_HPP

// -----------------------------------------------------------------------
//  vaterm::utf — UTF-8 字符串工具
//
//  提供 TUI 应用中处理 UTF-8 编码文本时常用的辅助函数：由首字节推断一个
//  码点的字节长度、统计码点个数、按索引取出码点，以及计算终端显示宽度
//  （基于 East Asian Width 与 Unicode 字符属性，兼顾 CJK / emoji /
//  全角字符）。
//
//  大多数函数接受 std::string_view 且从不分配内存；`pop_back` 是例外
//  ——它就地修改 std::string。
//
//  参考：
//    - RFC 3629 (UTF-8)
//    - Unicode TR#11 (East Asian Width)
//    - ECMA-48 / POSIX 控制字符的终端语义
// -----------------------------------------------------------------------

#include <cstdint>
#include <string>
#include <string_view>

namespace vaterm {
namespace utf {

// 返回首字节位于 `s` 起始处的 UTF-8 码点所占的字节数。
//
// 若 s[0] 不是合法的 UTF-8 首字节（即续字节 0x80-0xBF 或过长前缀），
// 返回 0。
//
//   0x00-0x7F  → 1 (ASCII)
//   0xC2-0xDF  → 2
//   0xE0-0xEF  → 3
//   0xF0-0xF7  → 4
inline size_t char_bytes(std::string_view s) {
    if (s.empty()) return 0;
    auto c = static_cast<unsigned char>(s[0]);
    if (c < 0x80) return 1;
    if (c < 0xC2) return 0;
    if (c < 0xE0) return 2;
    if (c < 0xF0) return 3;
    if (c < 0xF8) return 4;
    return 0;
}

// 返回 UTF-8 字符串 `s` 中 Unicode 码点的个数（不解析字形簇——
// 这里统计的是标量值）。
//
// 非法的续字节会被跳过，且不计入个数。
inline size_t count(std::string_view s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size();) {
        auto cb = char_bytes(s.substr(i));
        if (cb == 0) { ++i; continue; }
        ++n;
        i += cb;
    }
    return n;
}

// 返回 UTF-8 字符串 `s` 中第 i 个码点（从 0 开始）的原始字节视图
// （1-4 字节）。
//
// i 越界时返回空视图。非法续字节会被跳过，且不递增索引。
inline std::string_view at(std::string_view s, size_t i) {
    size_t idx = 0;
    for (size_t pos = 0; pos < s.size();) {
        auto cb = char_bytes(s.substr(pos));
        if (cb == 0) { ++pos; continue; }
        if (idx == i)
            return s.substr(pos, cb);
        ++idx;
        pos += cb;
    }
    return {};
}

// 就地删除 UTF-8 字符串 `s` 的最后一个码点。
// 若末尾残留孤立的续字节，会先回退到最近的首字节。典型用途：
// Backspace 删除一个字符。
inline void pop_back(std::string& s) {
    size_t i = s.size();
    while (i > 0 && (static_cast<unsigned char>(s[i - 1]) & 0xC0) == 0x80) --i;
    if (i > 0) --i;
    s.erase(i);
}

// 返回首字节位于 `s` 起始处的单个码点的终端显示宽度。
//
// 宽度规则：
//   - 控制字符（U+0000-U+001F、U+007F）→ 0
//   - ASCII 可打印字符 → 1
//   - East Asian Wide / Fullwidth / CJK / Emoji → 2
//   - 其它 → 1
//
// 宽字符范围检测覆盖 Hangul Jamo、CJK Unified
// Ideographs、CJK Compatibility、Fullwidth forms、Kana Supplement、
// Emoticons（从 U+1F004 起）以及 CJK Extension B-H。
inline int char_width(std::string_view s) {
    auto cb = char_bytes(s);
    if (cb == 0 || cb == 1) {
        if (s.empty()) return 0;
        auto c = static_cast<unsigned char>(s[0]);
        if (c < 0x20 || c == 0x7F) return 0;
        return 1;
    }
    // 把多字节序列解码为 Unicode 标量值。
    auto decode = [](std::string_view sv) -> uint32_t {
        auto b1 = static_cast<unsigned char>(sv[0]);
        auto b2 = static_cast<unsigned char>(sv[1]);
        auto b3 = static_cast<unsigned char>(sv[2]);
        if (sv.size() < 2) return 0;
        if (sv.size() == 2)
            return ((b1 & 0x1F) << 6) | (b2 & 0x3F);
        if (sv.size() >= 3) {
            auto b4 = sv.size() >= 4 ? static_cast<unsigned char>(sv[3]) : 0;
            if (b1 < 0xE0)
                return ((b1 & 0x1F) << 6) | (b2 & 0x3F);
            if (b1 < 0xF0)
                return ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            return ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F);
        }
        return 0;
    };
    auto cp = decode(s);

    // East Asian Width / CJK 分类。
    auto is_wide = [](uint32_t cp) -> bool {
        if (cp >= 0x1100 && cp <= 0x115F) return true;   // 谚文字母
        if (cp == 0x2329 || cp == 0x232A) return true;   // 尖括号
        if (cp >= 0x2E80 && cp <= 0xA4CF) return true;   // CJK 部首 + 彝文
        if (cp >= 0xA960 && cp <= 0xA97C) return true;   // 谚文字母扩展 A
        if (cp >= 0xAC00 && cp <= 0xD7AF) return true;   // 谚文音节
        if (cp >= 0xF900 && cp <= 0xFAFF) return true;   // CJK 兼容表意文字
        if (cp >= 0xFE10 && cp <= 0xFE19) return true;   // 竖排形式
        if (cp >= 0xFE30 && cp <= 0xFE6F) return true;   // CJK 兼容形式
        if (cp >= 0xFF01 && cp <= 0xFF60) return true;   // 全角形式
        if (cp >= 0xFFE0 && cp <= 0xFFE6) return true;   // 全角符号
        if (cp >= 0x1B000 && cp <= 0x1B0FF) return true; // 假名补充
        if (cp >= 0x1F004) return true;                  // 表情符号及以后
        if (cp >= 0x20000 && cp <= 0x3FFFF) return true; // CJK 扩展 B-H
        return false;
    };
    return is_wide(cp) ? 2 : 1;
}

// 返回 UTF-8 字符串 `s` 的终端显示宽度，即对每个码点累加
// char_width()。
inline int width(std::string_view s) {
    int w = 0;
    for (size_t i = 0; i < s.size();) {
        auto cb = char_bytes(s.substr(i));
        if (cb == 0) { ++i; continue; }
        w += char_width(s.substr(i, cb));
        i += cb;
    }
    return w;
}

} // namespace utf
} // namespace vaterm

#endif // VATERM_UTF_HPP
