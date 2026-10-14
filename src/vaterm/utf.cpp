// vaterm::utf —— 声明与实现的分离版：实现集中在此处。

#include "vaterm/utf.hpp"

namespace vaterm {
namespace utf {

size_t char_bytes(std::string_view s) {
    if (s.empty()) return 0;
    auto c = static_cast<unsigned char>(s[0]);
    if (c < 0x80) return 1;
    if (c < 0xC2) return 0;
    if (c < 0xE0) return 2;
    if (c < 0xF0) return 3;
    if (c < 0xF8) return 4;
    return 0;
}

size_t count(std::string_view s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size();) {
        auto cb = char_bytes(s.substr(i));
        if (cb == 0) { ++i; continue; }
        ++n;
        i += cb;
    }
    return n;
}

std::string_view at(std::string_view s, size_t i) {
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

void pop_back(std::string& s) {
    size_t i = s.size();
    while (i > 0 && (static_cast<unsigned char>(s[i - 1]) & 0xC0) == 0x80) --i;
    if (i > 0) --i;
    s.erase(i);
}

int char_width(std::string_view s) {
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

int width(std::string_view s) {
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
