// vaterm::terminal —— 声明与实现的分离版：实现集中在此处。

#include "vaterm/term.hpp"

#include "vaterm/cursor.hpp"
#include "vaterm/enums.hpp"

#include <cstdlib>
#include <string_view>
#include <unistd.h>
#include <sys/ioctl.h>

namespace vaterm {

bool terminal::size(uint16_t& rows, uint16_t& cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        return false;
    rows = ws.ws_row;
    cols = ws.ws_col;
    return true;
}

std::string terminal::type() {
    auto* t = getenv("TERM");
    return t ? std::string(t) : std::string();
}

ColorDepth terminal::detect_color_depth() {
    static ColorDepth depth = [] {
        auto* ct = getenv("COLORTERM");
        if (ct) {
            std::string_view sv(ct);
            if (sv == "truecolor" || sv == "24bit")
                return ColorDepth::C24;
        }
        auto* term = getenv("TERM");
        if (term) {
            std::string_view sv(term);
            if (sv.ends_with("-256color"))
                return ColorDepth::C8;
            // 一些始终支持 truecolor 的现代终端
            if (sv == "xterm-kitty" || sv == "alacritty" || sv == "foot" || sv == "foot-extra")
                return ColorDepth::C24;
        }
        return ColorDepth::C4;
    }();
    return depth;
}

bool terminal::enter_raw() {
    if (raw_) return true;
    if (!isatty(STDIN_FILENO)) return false;
    if (tcgetattr(STDIN_FILENO, &original_) == -1) return false;
    struct termios raw = original_;
    cfmakeraw(&raw);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return false;
    raw_ = true;
    return true;
}

void terminal::exit_raw() {
    if (!raw_) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
    raw_ = false;
}

void terminal::write(std::string_view s) {
    ::write(STDOUT_FILENO, s.data(), s.size());
}

void terminal::flush() {
    fsync(STDOUT_FILENO);
}

std::optional<unsigned char> terminal::read_byte() {
    unsigned char c;
    auto n = ::read(STDIN_FILENO, &c, 1);
    if (n <= 0) return std::nullopt;
    return c;
}

char terminal::getch() {
    char c;
    while (::read(STDIN_FILENO, &c, 1) <= 0)
        ;
    return c;
}

terminal::~terminal() {
    if (raw_) {
        write(cursor::show());
        write(clear_screen());
        flush();
    }
    exit_raw();
}

} // namespace vaterm
