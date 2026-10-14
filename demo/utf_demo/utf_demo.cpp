//- utf_demo — 演示 vaterm::utf 中的每一个函数
//
//  覆盖：
//    1. char_bytes()     — 首字节分析
//    2. count()           — 码点计数
//    3. at()              — 按下标提取码点
//    4. char_width()      — 单个码点的显示宽度
//    5. width()           — 字符串的总显示宽度
//    6. 对齐工具 — 按 UTF-8 显示宽度填充标签
//
//  每个测试都会在结果旁打印 PASS/FAIL 判定。
//
//  运行：  ./utf_demo

#include <vaterm.hpp>
#include <cstdio>
#include <string_view>

using namespace vaterm;
using namespace std::string_view_literals;

// 辅助函数：打印单个测试结果。
static void check(const std::string& label, bool cond) {
    printf("  %-30s %s\n", label.c_str(), cond ? "PASS" : "FAIL");
}

// 测试 char_bytes：根据 UTF-8 字符的首字节
// 确定其字节长度。
static void demo_char_bytes() {
    printf("%s1. char_bytes()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    check("char_bytes(\"\") == 0",      utf::char_bytes(""sv) == 0);
    check("char_bytes(\"a\") == 1",     utf::char_bytes("a"sv) == 1);
    check("char_bytes(\"\\x7F\") == 1",  utf::char_bytes("\x7F"sv) == 1);
    check("char_bytes(\"\\xC0\") == 0",  utf::char_bytes("\xC0"sv) == 0);
    check("char_bytes(\"\\xC2\") == 2",  utf::char_bytes("\xC2"sv) == 2);
    check("char_bytes(\"\\xE0\") == 3",  utf::char_bytes("\xE0"sv) == 3);
    check("char_bytes(\"\\xF0\") == 4",  utf::char_bytes("\xF0"sv) == 4);
}

// 测试 count：字符串中 Unicode 标量值的数量。
static void demo_count() {
    printf("\n%s2. count()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    check("count(\"\") == 0",           utf::count(""sv) == 0);
    check("count(\"abc\") == 3",        utf::count("abc"sv) == 3);
    check("count(\"你好\") == 2",        utf::count("你好"sv) == 2);
    check("count(\"Hello, 世界!\") == 10", utf::count("Hello, 世界!"sv) == 10);
}

// 测试 at：按下标提取单个码点。
static void demo_at() {
    printf("\n%s3. at()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    auto s = "Hello, 世界!"sv;
    check("at(s, 0) == \"H\"",     utf::at(s, 0) == "H"sv);
    check("at(s, 7) == \"世\"",     utf::at(s, 7) == "世"sv);
    check("at(s, 8) == \"界\"",     utf::at(s, 8) == "界"sv);
    check("at(s, 9) == \"!\"",     utf::at(s, 9) == "!"sv);
    check("at(s, 99).empty()",     utf::at(s, 99).empty());
}

// 测试 char_width：单个码点的终端显示宽度。
static void demo_char_width() {
    printf("\n%s4. char_width()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    check("char_width(\" \") == 1",    utf::char_width(" "sv) == 1);
    check("char_width(\"\\n\") == 0",   utf::char_width("\n"sv) == 0);
    check("char_width(\"\\t\") == 0",   utf::char_width("\t"sv) == 0);
    check("char_width(\"\\x7F\") == 0", utf::char_width("\x7F"sv) == 0);
    check("char_width(\"a\") == 1",    utf::char_width("a"sv) == 1);
    check("char_width(\"A\") == 1",    utf::char_width("A"sv) == 1);
    check("char_width(\"世\") == 2",    utf::char_width("世"sv) == 2);
    check("char_width(\"界\") == 2",    utf::char_width("界"sv) == 2);
    check("char_width(\"한\") == 2",    utf::char_width("한"sv) == 2);   // 谚文
    check("char_width(\"\\uFF01\") == 2", utf::char_width("\uFF01"sv) == 2); // 全角
    check("char_width(\"\\u2E80\") == 2", utf::char_width("\u2E80"sv) == 2); // CJK 部首
}

// 测试 width：多码点字符串的总显示宽度。
static void demo_width() {
    printf("\n%s5. width()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    check("width(\"\") == 0",            utf::width(""sv) == 0);
    check("width(\"abc\") == 3",         utf::width("abc"sv) == 3);
    check("width(\"你好\") == 4",         utf::width("你好"sv) == 4);
    check("width(\"Hello, 世界!\") == 12", utf::width("Hello, 世界!"sv) == 12);
    check("width(\"a世c\") == 4",        utf::width("a世c"sv) == 4);
}

// 演示用 utf::width 做定宽标签对齐。
static void demo_alignment() {
    printf("\n%s6. Alignment Utility%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    struct Entry { std::string_view label; std::string_view value; };
    Entry entries[] = {
        {"Name", "Alice"},
        {"Age", "28"},
        {"Nationality", "China"},
        {"Occupation", "Software Engineer"},
    };

    // 计算所有标签中的最大显示宽度。
    int max_w = 0;
    for (auto& e : entries)
        if (utf::width(e.label) > max_w) max_w = utf::width(e.label);

    // 用空格右填充每个标签，使 ':' 对齐。
    for (auto& e : entries) {
        auto pad = max_w - utf::width(e.label);
        printf("%*s%s: %s\n",
               pad > 0 ? pad : 0, "",
               std::string(e.label).c_str(),
               std::string(e.value).c_str());
    }
    printf("  (labels aligned by UTF-8 display width)\n");
}

int main() {
    printf("%s=== vaterm UTF-8 API Demo ===%s\n\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    demo_char_bytes();
    demo_count();
    demo_at();
    demo_char_width();
    demo_width();
    demo_alignment();

    printf("\n%sAll UTF-8 API tests completed.%s\n",
           color::fg(Color4::GREEN).c_str(), color::reset().c_str());
    return 0;
}
