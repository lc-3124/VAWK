#ifndef VATERM_TERM_HPP
#define VATERM_TERM_HPP

// -----------------------------------------------------------------------
//  vaterm::terminal —— 终端模式切换、屏幕控制与 I/O
//
//  terminal 类提供：
//
//    1. RAII raw 模式：  enter_raw() / ~terminal() 在析构时
//                        自动恢复原有的 termios。
//    2. 清屏：            clear_screen()、clear_to_eol()、
//                        clear_to_eos()、clear_line()。
//    3. 终端查询：        size(rows, cols)、type()。
//    4. I/O：             write()、flush()、read_byte()、getch()。
//
//  所有静态方法（clear_*、size、type、write、flush、read_byte、
//  getch）都是线程安全的，无需实例。实例仅在
//  管理 raw 模式时才需要。
//
//  示例：
//    terminal t;
//    t.enter_raw();
//    terminal::write(terminal::clear_screen());
//    terminal::write("Hello under raw mode!\n");
//    // ~terminal() 会自动恢复原有设置。
// -----------------------------------------------------------------------

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "cursor.hpp"

namespace vaterm {

class terminal {
  public:
    terminal() = default;
    ~terminal();

    // 不可拷贝——持有 raw 模式状态。
    terminal(const terminal&)            = delete;
    terminal& operator=(const terminal&) = delete;

    // ---- 清屏序列（静态） -----------------------------------------------

    // 清空整个屏幕并把光标移到 (1,1)。
    static std::string clear_screen() {
        return "\033[2J\033[H";
    }

    // 从光标处清到行尾（EL 0）。
    static std::string clear_to_eol() {
        return "\033[K";
    }

    // 从光标处清到屏幕末尾（ED 0）。
    static std::string clear_to_eos() {
        return "\033[J";
    }

    // 清空当前整行并把光标退回第 1 列。
    static std::string clear_line() {
        return "\033[2K\r";
    }

    // ---- 终端查询（静态） -----------------------------------------------

    // 通过 TIOCGWINSZ ioctl 查询终端尺寸。
    // 成功返回 true，失败（例如不是 tty）返回 false。
    // 成功时 rows 与 cols 会填入检测到的尺寸。
    static bool size(uint16_t& rows, uint16_t& cols) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
            return false;
        rows = ws.ws_row;
        cols = ws.ws_col;
        return true;
    }

    // 对 size() 的便捷包装，直接返回值。
    // 出错（例如不是 tty）时返回 0。
    static int getColMax() {
        uint16_t r = 0, c = 0;
        return size(r, c) ? static_cast<int>(c) : 0;
    }
    static int getRowMax() {
        uint16_t r = 0, c = 0;
        return size(r, c) ? static_cast<int>(r) : 0;
    }

    // 返回环境变量 $TERM 的值，
    // 未设置时返回空串。
    static std::string type() {
        auto* t = getenv("TERM");
        return t ? std::string(t) : std::string();
    }

    // 通过检查 $COLORTERM 与 $TERM 检测终端支持的最大色深。
    // 结果在首次调用后会被缓存。
    //
    // 返回：
    //   C24 —— truecolor / 24-bit（例如 $COLORTERM=truecolor）
    //   C8  —— 256 色（例如 $TERM=xterm-256color）
    //   C4  —— 16 色 / 其他未知情况
    static ColorDepth detect_color_depth() {
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

    // ---- raw 模式（实例方法） -------------------------------------------

    // 使用 cfmakeraw(3) 把 stdin 切到 raw 模式。
    //
    // raw 模式会关闭行缓冲、信号键（^C、^Z）
    // 以及回显。调用 enter_raw() 后，每次按键都可以
    // 立刻通过 read() 读到。
    //
    // VMIN = 0、VTIME = 1 会给出带 100 ms 超时的非阻塞读
    // —— 若暂无数据待读，read_byte() 会返回 -1。
    //
    // 成功返回 true；stdin 不是 tty 或
    // tcgetattr/tcsetattr 失败时返回 false。
    bool enter_raw() {
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

    // 恢复 enter_raw() 保存下来的原有终端设置。
    // 由析构函数自动调用。
    void exit_raw() {
        if (!raw_) return;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
        raw_ = false;
    }

    // 检查当前是否处于 raw 模式。
    bool is_raw() const { return raw_; }

    // ---- I/O（静态） ----------------------------------------------------

    // 向 stdout 写一个字符串。
    static void write(std::string_view s) {
        ::write(STDOUT_FILENO, s.data(), s.size());
    }

    // 刷新 stdout（fsync）。在 write() 之后调用，
    // 确保终端立刻收到数据。
    static void flush() {
        fsync(STDOUT_FILENO);
    }

    // 从 stdin 非阻塞地读一个字节。
    // 在 raw 模式（VMIN=0、VTIME=1）下，若 100 ms 内
    // 无数据可读则返回 nullopt。
    static std::optional<unsigned char> read_byte() {
        unsigned char c;
        auto n = ::read(STDIN_FILENO, &c, 1);
        if (n <= 0) return std::nullopt;
        return c;
    }

    // read_byte() 的阻塞版本。
    // 循环等待直到有字节到达，然后以 char 返回。
    static char getch() {
        char c;
        while (::read(STDIN_FILENO, &c, 1) <= 0)
            ;
        return c;
    }

  private:
    bool        raw_ = false;       // raw 模式是否处于激活状态
    struct termios original_;       // raw 模式之前的 termios 备份
};

inline terminal::~terminal() {
    if (raw_) {
        write(cursor::show());
        write(clear_screen());
        flush();
    }
    exit_raw();
}

} // namespace vaterm

#endif // VATERM_TERM_HPP
