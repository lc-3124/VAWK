#ifndef VATERM_UTF_HPP
#define VATERM_UTF_HPP

// -----------------------------------------------------------------------
//  vaterm::utf — UTF-8 string utilities
//
//  Provides helpers for working with UTF-8 encoded text that are commonly
//  needed in TUI applications: determining the byte length of a codepoint
//  from its leading byte, counting codepoints, extracting a codepoint by
//  index, and computing the terminal display width (with CJK / emoji /
//  fullwidth awareness based on East Asian Width and Unicode character
//  properties).
//
//  All functions accept std::string_view and never allocate.
//
//  References:
//    - RFC 3629 (UTF-8)
//    - Unicode TR#11 (East Asian Width)
//    - ECMA-48 / POSIX terminal semantics for control chars
// -----------------------------------------------------------------------

#include <cstdint>
#include <string_view>

namespace vaterm {
namespace utf {

// Return the number of bytes occupied by the UTF-8 codepoint whose
// leading byte is at the start of `s`.
//
// Returns 0 if the byte at s[0] is not a valid UTF-8 leading byte
// (i.e. continuation byte 0x80-0xBF or overlong prefix).
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

// Return the number of Unicode codepoints (grapheme clusters are NOT
// resolved — this counts scalar values) in the UTF-8 string `s`.
//
// Invalid continuation bytes are skipped and do not increment the
// count.
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

// Return the i-th codepoint (0-based) of the UTF-8 string `s` as a
// view of its raw bytes (1-4 bytes).
//
// Returns an empty view if i is out of range. Invalid continuation
// bytes are skipped without incrementing the index.
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

// Return the terminal display width of the single codepoint whose
// leading byte is at the start of `s`.
//
// Width rules:
//   - Control characters (U+0000-U+001F, U+007F) → 0
//   - ASCII printable → 1
//   - East Asian Wide / Fullwidth / CJK / Emoji → 2
//   - Everything else → 1
//
// The wide-range detection covers Hangul Jamo, CJK Unified
// Ideographs, CJK Compatibility, Fullwidth forms, Kana Supplement,
// Emoticons (starting at U+1F004), and CJK Extension B-H.
inline int char_width(std::string_view s) {
    auto cb = char_bytes(s);
    if (cb == 0 || cb == 1) {
        if (s.empty()) return 0;
        auto c = static_cast<unsigned char>(s[0]);
        if (c < 0x20 || c == 0x7F) return 0;
        return 1;
    }
    // Decode the multi-byte sequence into a Unicode scalar value.
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

    // East Asian Width / CJK classification.
    auto is_wide = [](uint32_t cp) -> bool {
        if (cp >= 0x1100 && cp <= 0x115F) return true;   // Hangul Jamo
        if (cp == 0x2329 || cp == 0x232A) return true;   // Angle brackets
        if (cp >= 0x2E80 && cp <= 0xA4CF) return true;   // CJK radicals + Yi
        if (cp >= 0xA960 && cp <= 0xA97C) return true;   // Hangul Jamo EA
        if (cp >= 0xAC00 && cp <= 0xD7AF) return true;   // Hangul Syllables
        if (cp >= 0xF900 && cp <= 0xFAFF) return true;   // CJK Compatibility Ideographs
        if (cp >= 0xFE10 && cp <= 0xFE19) return true;   // Vertical forms
        if (cp >= 0xFE30 && cp <= 0xFE6F) return true;   // CJK Compatibility Forms
        if (cp >= 0xFF01 && cp <= 0xFF60) return true;   // Fullwidth Forms
        if (cp >= 0xFFE0 && cp <= 0xFFE6) return true;   // Fullwidth Signs
        if (cp >= 0x1B000 && cp <= 0x1B0FF) return true; // Kana Supplement
        if (cp >= 0x1F004) return true;                  // Emoticons & beyond
        if (cp >= 0x20000 && cp <= 0x3FFFF) return true; // CJK Extension B-H
        return false;
    };
    return is_wide(cp) ? 2 : 1;
}

// Return the terminal display width of the UTF-8 string `s` by summing
// char_width() for each codepoint.
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
