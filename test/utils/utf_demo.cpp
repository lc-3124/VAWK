// -----------------------------------------------------------------------
//  utf_demo — demonstrates every function in vaterm::utf
//
//  Exercises:
//    1. char_bytes()     — leading-byte analysis
//    2. count()           — codepoint count
//    3. at()              — codepoint-by-index extraction
//    4. char_width()      — display width of a single codepoint
//    5. width()           — total display width of a string
//    6. Alignment utility — pad labels by UTF-8 display width
//
//  Each test prints the result alongside a PASS/FAIL verdict.
//
//  Run:  ./utf_demo
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <string_view>

using namespace vaterm;
using namespace std::string_view_literals;

// Helper: print a single test result.
static void check(const std::string& label, bool cond) {
    printf("  %-30s %s\n", label.c_str(), cond ? "PASS" : "FAIL");
}

// Test char_bytes: determine byte-length from the leading byte of a
// UTF-8 character.
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

// Test count: number of Unicode scalar values in a string.
static void demo_count() {
    printf("\n%s2. count()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    check("count(\"\") == 0",           utf::count(""sv) == 0);
    check("count(\"abc\") == 3",        utf::count("abc"sv) == 3);
    check("count(\"你好\") == 2",        utf::count("你好"sv) == 2);
    check("count(\"Hello, 世界!\") == 10", utf::count("Hello, 世界!"sv) == 10);
}

// Test at: extract a single codepoint by index.
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

// Test char_width: terminal display width of individual codepoints.
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
    check("char_width(\"한\") == 2",    utf::char_width("한"sv) == 2);   // Hangul
    check("char_width(\"\\uFF01\") == 2", utf::char_width("\uFF01"sv) == 2); // fullwidth
    check("char_width(\"\\u2E80\") == 2", utf::char_width("\u2E80"sv) == 2); // CJK radical
}

// Test width: total display width of multi-codepoint strings.
static void demo_width() {
    printf("\n%s5. width()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    check("width(\"\") == 0",            utf::width(""sv) == 0);
    check("width(\"abc\") == 3",         utf::width("abc"sv) == 3);
    check("width(\"你好\") == 4",         utf::width("你好"sv) == 4);
    check("width(\"Hello, 世界!\") == 12", utf::width("Hello, 世界!"sv) == 12);
    check("width(\"a世c\") == 4",        utf::width("a世c"sv) == 4);
}

// Demonstrate using utf::width for fixed-width label alignment.
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

    // Compute the maximum display width among the labels.
    int max_w = 0;
    for (auto& e : entries)
        if (utf::width(e.label) > max_w) max_w = utf::width(e.label);

    // Right-pad each label with spaces to align the ':'.
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
