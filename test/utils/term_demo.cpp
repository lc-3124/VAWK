// -----------------------------------------------------------------------
//  term_demo — demonstrates the vaterm::terminal class
//
//  Exercises:
//    1. Clear-screen sequences (clear_screen, clear_to_eol, etc.)
//    2. Terminal size query (terminal::size)
//    3. Terminal type / $TERM lookup (terminal::type)
//    4. write / flush / read_byte I/O
//    5. Raw-mode RAII lifecycle (enter_raw / exit_raw / destructor)
//
//  All tests are non-interactive and safe to run in a pipe.
//
//  Run:  ./term_demo
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace vaterm;

// Print the raw bytes of each clear-sequence for inspection.
static void demo_clear_sequences() {
    printf("%s1. Clear Sequences (static)%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    printf("  clear_screen(): \"%s\"\n", terminal::clear_screen().c_str());
    printf("  clear_to_eol(): \"%s\"\n", terminal::clear_to_eol().c_str());
    printf("  clear_to_eos(): \"%s\"\n", terminal::clear_to_eos().c_str());
    printf("  clear_line():   \"%s\"\n", terminal::clear_line().c_str());
}

// Query and display terminal dimensions via TIOCGWINSZ.
static void demo_size() {
    printf("\n%s2. Terminal Size%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    uint16_t rows = 0, cols = 0;
    if (terminal::size(rows, cols))
        printf("  terminal size: %d rows x %d cols\n", rows, cols);
    else
        printf("  FAIL: unable to get terminal size\n");
}

// Show the value of $TERM.
static void demo_type() {
    printf("\n%s3. Terminal Type%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    auto t = terminal::type();
    printf("  $TERM = \"%s\"\n", t.empty() ? "(unset)" : t.c_str());
}

// Demonstrate write / flush / read_byte.
static void demo_write_flush() {
    printf("\n%s4. write() / flush() / read_byte()%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    terminal::write("  write() via ::write(STDOUT_FILENO, ...)\n");
    terminal::flush();
    printf("  (write+flush completed)\n");
}

// Enter raw mode, verify is_raw(), attempt a non-blocking read,
// exit raw mode, and confirm the terminal is restored.
static void demo_raw_mode() {
    printf("\n%s5. Raw Mode (RAII)%s\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    terminal t;
    if (!t.enter_raw()) {
        printf("  enter_raw() returned false (not a tty or permission issue)\n");
        printf("  This is expected in non-tty environments.\n");
        return;
    }
    printf("  is_raw() = %s\n", t.is_raw() ? "true" : "false");
    printf("  Raw mode active. Attempting non-blocking read...\n");

    auto ch = terminal::read_byte();
    if (ch >= 0)
        printf("  read_byte() = %d (0x%02x, '%c')\n", ch, ch, (ch >= 0x20 && ch < 0x7f) ? ch : '.');
    else
        printf("  read_byte() = -1 (no data available, as expected)\n");

    t.exit_raw();
    printf("  is_raw() after exit_raw() = %s\n", t.is_raw() ? "true" : "false");
    printf("  Raw mode test passed.\n");
}

int main() {
    printf("%s=== vaterm terminal API Demo ===%s\n\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    demo_clear_sequences();
    demo_size();
    demo_type();
    demo_write_flush();
    demo_raw_mode();

    printf("\n%sAll terminal API tests completed.%s\n",
           color::fg(Color4::GREEN).c_str(), color::reset().c_str());
    return 0;
}
