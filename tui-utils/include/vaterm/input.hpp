#ifndef VATERM_INPUT_HPP
#define VATERM_INPUT_HPP

/**
 * 我打算重新设计输入模块，区别于以前通过获取鼠标输入后手动调用parse()来解析
 * 相关数据，这次我打算完全接管终端模拟器的全部输入，在用户层面不再使用系统的
 * 原生输入。使用状态机来解析输入，解析的结果填入缓冲队列，消费者从缓冲队列
 * 获取键盘和鼠标的输入事件。
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <poll.h>
#include <unistd.h>

#include "term.hpp"

namespace vaterm {

/// 输入数据包：状态机解析出的一个键盘或鼠标事件
struct InputData {
    // ── 子结构定义 ─────────────────────────────────────────────────────────
    // ───────────────────────────────────────────────────────────────────────

    /// 数据包类型，决定联合体中哪个负载成员有效
    enum class InputType : uint8_t {
        MOUSE = 0,  ///< 鼠标事件
        KEYBD = 1,  ///< 键盘事件
        NON   = 2   ///< 空包（无事件）
    };

    // ── 鼠标 ─────────────────────────────────────────────────────────

    /// 鼠标按键
    enum class MouseButton : uint8_t {
        NONE = 0,  ///< 无按键（滚轮、松开）
        LEFT,      ///< 左键
        MIDDLE,    ///< 中键
        RIGHT      ///< 右键
    };

    /// 鼠标动作
    enum class MouseAction : uint8_t {
        RELEASE = 0,  ///< 松开
        PRESS,        ///< 按下
        DRAG,         ///< 按住拖动
        MOVE,         ///< 无按键移动（悬停）
        SCROLL_UP,    ///< 滚轮上滚
        SCROLL_DOWN   ///< 滚轮下滚
    };

    /// 鼠标事件负载（坐标为 0 起）
    struct MouseData {
        int         col     = 0;                     ///< 列
        int         row     = 0;                     ///< 行
        MouseButton button  = MouseButton::NONE;     ///< 涉及的按键
        MouseAction action  = MouseAction::RELEASE;  ///< 动作
        bool        isShift = false;                 ///< 是否按下 Shift
        bool        isAlt   = false;                 ///< 是否按下 Alt / Meta
        bool        isCtrl  = false;                 ///< 是否按下 Ctrl
    };

    // ── 键盘 ─────────────────────────────────────────────────────────

    /// 键盘特殊键码（可打印键请用 KeybdData::cp，此时 code 保持 NONE）
    enum class KeyCode : int {
        NONE = 0,
        UP, DOWN, LEFT, RIGHT,
        HOME, END, PGUP, PGDN, INS, DEL,
        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,
        ESC, TAB, ENTER, BACKSPACE
    };

    /// 键盘事件负载
    struct KeybdData {
        char32_t cp      = 0;              ///< 可打印键的 Unicode 码点（无则 0）
        KeyCode  code    = KeyCode::NONE;  ///< 特殊键码（无则 NONE）
        bool     isShift = false;          ///< 是否按下 Shift
        bool     isAlt   = false;          ///< 是否按下 Alt / Meta
        bool     isCtrl  = false;          ///< 是否按下 Ctrl
    };

    // ── 实际负载 ───────────────────────────────────────────────────────────
    // ───────────────────────────────────────────────────────────────────────

    InputType inputType = InputType::NON;  ///< 当前包的类型

    /// 事件负载；按 inputType 读取 mouse 或 keybd。
    union Value {
        MouseData mouse;  ///< 鼠标
        KeybdData keybd;  ///< 键盘

        Value() {}  ///< 不主动初始化任何成员，由构造器/赋值决定
        ~Value() {}
    } value;

    // ── 构造数据包 ─────────────────────────────────────────────────────────

    /// 构造一个空包
    InputData() {}

    /// 构造鼠标包
    static InputData makeMouse(const MouseData& m) {
        InputData d;
        d.inputType   = InputType::MOUSE;
        d.value.mouse = m;
        return d;
    }

    /// 构造键盘包
    static InputData makeKeybd(const KeybdData& k) {
        InputData d;
        d.inputType   = InputType::KEYBD;
        d.value.keybd = k;
        return d;
    }
};

// ===========================================================================
//  input_fsm — 输入状态机（内部实现）
// ===========================================================================
//
//  把终端字节流解析成 InputData 事件。之所以单独成一个类，是为了能脱离
//  终端、直接喂字节来测试；正常使用请通过 vaterm::input。
//
//  状态转移：
//    GROUND ─ 0x1B ─► ESC
//    ESC    ─ '['  ─► CSI ── 第 3 字节为 '<' ─► MOUSE
//                  └─ 终止符 0x40-0x7E ─► GROUND（键盘序列）
//    ESC    ─ 'O'  ─► SS3 ─ 1 字节 ─► GROUND
//    ESC    ─ 可打印 ─► GROUND（Alt+键）
//    ESC    ─ 其它  ─► GROUND（产出 Esc，并把该字节重新按 GROUND 处理）
//    GROUND ─ 0xC2-0xF7 ─► UTF8 ─ 后续字节 ─► GROUND
//    MOUSE  ─ 累积到 'M'/'m' ─► GROUND（超长则丢弃）
// ===========================================================================

class input_fsm {
  public:
    /// 喂入一个字节，解析出的事件追加到 out。
    void feed(unsigned char b, std::deque<InputData>& out) {
        bool reprocess = true;
        while (reprocess) {
            switch (state_) {
            case State::GROUND: reprocess = step_ground(b, out); break;
            case State::ESC:    reprocess = step_esc(b, out);    break;
            case State::CSI:    reprocess = step_csi(b, out);    break;
            case State::SS3:    reprocess = step_ss3(b, out);    break;
            case State::MOUSE:  reprocess = step_mouse(b, out);  break;
            case State::UTF8:   reprocess = step_utf8(b, out);   break;
            }
        }
    }

    /// 喂入一段字节。
    void feed(std::string_view bytes, std::deque<InputData>& out) {
        for (unsigned char b : bytes) feed(b, out);
    }

    /// 是否有尚未完成的序列（半截 CSI / 鼠标 / UTF-8，或孤立的 ESC）。
    bool pending() const { return state_ != State::GROUND; }

    /// 超时调用：把孤立的 ESC 收敛成 Esc 键事件；其它半截序列保留等待后续字节。
    void flush_pending(std::deque<InputData>& out) {
        if (state_ == State::ESC) {
            push_key(out, 0, InputData::KeyCode::ESC);
            reset();
        }
    }

    /// 复位（丢弃半截序列）。
    void reset() {
        state_ = State::GROUND;
        acc_.clear();
    }

  private:
    enum class State { GROUND, ESC, CSI, SS3, MOUSE, UTF8 };

    State       state_ = State::GROUND;
    std::string acc_;  ///< 当前序列已累积的字节

    /// 构造一个键盘事件追加到 out。
    static void push_key(std::deque<InputData>& out, char32_t cp,
                         InputData::KeyCode code,
                         bool shift = false, bool alt = false, bool ctrl = false) {
        out.push_back(InputData::makeKeybd({cp, code, shift, alt, ctrl}));
    }

    // 各状态处理；返回 true 表示同一字节需在新状态下重新处理。
    bool step_ground(unsigned char b, std::deque<InputData>& out);
    bool step_esc(unsigned char b, std::deque<InputData>& out);
    bool step_csi(unsigned char b, std::deque<InputData>& out);
    bool step_ss3(unsigned char b, std::deque<InputData>& out);
    bool step_mouse(unsigned char b, std::deque<InputData>& out);
    bool step_utf8(unsigned char b, std::deque<InputData>& out);

    /// 解析 CSI 键盘序列（params 为去头去尾后的参数串，final 为终止字节）。
    static void parse_csi(std::string_view params, char final, std::deque<InputData>& out);
    /// 解析一条完整的 SGR 鼠标序列（\033[<Cb;Cx;CyM/m）。
    static void parse_mouse(const std::string& seq, std::deque<InputData>& out);
};

// ── input_fsm 实现 ──────────────────────────────────────────────────────────

inline bool input_fsm::step_ground(unsigned char b, std::deque<InputData>& out) {
    using KC = InputData::KeyCode;
    if (b == 0x1B) { state_ = State::ESC; acc_.assign(1, '\033'); return false; }
    if (b == 0x09) { push_key(out, 0, KC::TAB);       return false; }
    if (b == 0x0D) { push_key(out, 0, KC::ENTER);     return false; }
    if (b == 0x7F) { push_key(out, 0, KC::BACKSPACE); return false; }
    if (b == 0x00) { push_key(out, U' ', KC::NONE, false, false, true); return false; }
    if (b >= 0x01 && b <= 0x1A) {
        push_key(out, static_cast<char32_t>('a' + b - 1), KC::NONE, false, false, true);
        return false;
    }
    if (b >= 0x1C && b <= 0x1F) {
        char32_t cp = b == 0x1C ? U'\\' : b == 0x1D ? U']' : b == 0x1E ? U'^' : U'_';
        push_key(out, cp, KC::NONE, false, false, true);
        return false;
    }
    if (b >= 0x20 && b <= 0x7E) {
        push_key(out, static_cast<char32_t>(b), KC::NONE);
        return false;
    }
    if (b >= 0xC2 && b <= 0xF7) {
        state_ = State::UTF8;
        acc_.assign(1, static_cast<char>(b));
        return false;
    }
    return false;  // 无法识别的字节，丢弃
}

inline bool input_fsm::step_esc(unsigned char b, std::deque<InputData>& out) {
    if (b == '[') { state_ = State::CSI; acc_ += '['; return false; }
    if (b == 'O') { state_ = State::SS3; acc_ += 'O'; return false; }
    if (b >= 0x20 && b <= 0x7E) {
        push_key(out, static_cast<char32_t>(b), InputData::KeyCode::NONE, false, true, false);
        reset();
        return false;
    }
    // 不是转义序列：先产出 Esc，再把当前字节重新按 GROUND 处理。
    push_key(out, 0, InputData::KeyCode::ESC);
    reset();
    return true;
}

inline bool input_fsm::step_csi(unsigned char b, std::deque<InputData>& out) {
    acc_ += static_cast<char>(b);
    // SGR 鼠标：\033[<
    if (acc_.size() == 3 && acc_[2] == '<') { state_ = State::MOUSE; return false; }
    // 终止字节：0x40-0x7E
    if (b >= 0x40 && b <= 0x7E) {
        std::string_view p(acc_);
        p.remove_prefix(2);
        p.remove_suffix(1);
        parse_csi(p, static_cast<char>(b), out);
        reset();
        return false;
    }
    if (acc_.size() > 64) reset();  // 失步保护
    return false;
}

inline bool input_fsm::step_ss3(unsigned char b, std::deque<InputData>& out) {
    using KC = InputData::KeyCode;
    switch (b) {
    case 'P': push_key(out, 0, KC::F1); break;
    case 'Q': push_key(out, 0, KC::F2); break;
    case 'R': push_key(out, 0, KC::F3); break;
    case 'S': push_key(out, 0, KC::F4); break;
    default: break;
    }
    reset();
    return false;
}

inline bool input_fsm::step_mouse(unsigned char b, std::deque<InputData>& out) {
    acc_ += static_cast<char>(b);
    if (b == 'M' || b == 'm') {
        parse_mouse(acc_, out);
        reset();
        return false;
    }
    if (acc_.size() > 64) reset();  // 失步保护
    return false;
}

inline bool input_fsm::step_utf8(unsigned char b, std::deque<InputData>& out) {
    acc_ += static_cast<char>(b);
    auto lead = static_cast<unsigned char>(acc_[0]);
    int  need = lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    for (int i = 1; i < static_cast<int>(acc_.size()); ++i) {
        if ((static_cast<unsigned char>(acc_[i]) & 0xC0) != 0x80) { reset(); return false; }
    }
    if (static_cast<int>(acc_.size()) < need) return false;

    auto u = [&](int i) {
        return static_cast<char32_t>(static_cast<unsigned char>(acc_[i]) & 0x3F);
    };
    char32_t cp = 0;
    if (need == 2)      cp = ((lead & 0x1F) << 6) | u(1);
    else if (need == 3) cp = ((lead & 0x0F) << 12) | (u(1) << 6) | u(2);
    else                cp = ((lead & 0x07) << 18) | (u(1) << 12) | (u(2) << 6) | u(3);

    push_key(out, cp, InputData::KeyCode::NONE);
    reset();
    return false;
}

inline void input_fsm::parse_csi(std::string_view params, char final,
                                 std::deque<InputData>& out) {
    using KC = InputData::KeyCode;
    auto to_int = [](std::string_view sv) {
        int v = 0;
        for (char c : sv) { if (c < '0' || c > '9') break; v = v * 10 + (c - '0'); }
        return v;
    };
    // 参数形如 "n" 或 "n;m"（m 为修饰键编码）。
    auto semi  = params.find(';');
    int  main_n = semi == std::string_view::npos ? to_int(params)
                                                 : to_int(params.substr(0, semi));
    int  mod    = semi == std::string_view::npos ? 0 : to_int(params.substr(semi + 1));
    bool shift  = mod != 0 && ((mod - 1) & 1) != 0;
    bool alt    = mod != 0 && ((mod - 1) & 2) != 0;
    bool ctrl   = mod != 0 && ((mod - 1) & 4) != 0;

    KC code = KC::NONE;
    if (params.empty()) {
        switch (final) {
        case 'A': code = KC::UP;    break;
        case 'B': code = KC::DOWN;  break;
        case 'C': code = KC::RIGHT; break;
        case 'D': code = KC::LEFT;  break;
        case 'H': code = KC::HOME;  break;
        case 'F': code = KC::END;   break;
        default: return;
        }
    } else if (final == '~') {
        switch (main_n) {
        case 1:  code = KC::HOME; break;
        case 2:  code = KC::INS;  break;
        case 3:  code = KC::DEL;  break;
        case 4:  code = KC::END;  break;
        case 5:  code = KC::PGUP; break;
        case 6:  code = KC::PGDN; break;
        case 11: code = KC::F1;   break;
        case 12: code = KC::F2;   break;
        case 13: code = KC::F3;   break;
        case 14: code = KC::F4;   break;
        case 15: code = KC::F5;   break;
        case 17: code = KC::F6;   break;
        case 18: code = KC::F7;   break;
        case 19: code = KC::F8;   break;
        case 20: code = KC::F9;   break;
        case 21: code = KC::F10;  break;
        case 23: code = KC::F11;  break;
        case 24: code = KC::F12;  break;
        default: return;
        }
    } else {
        return;
    }
    push_key(out, 0, code, shift, alt, ctrl);
}

inline void input_fsm::parse_mouse(const std::string& seq, std::deque<InputData>& out) {
    // seq = ESC [ < Cb ; Cx ; Cy M/m
    if (seq.size() < 6 || seq[0] != '\033' || seq[1] != '[' || seq[2] != '<') return;
    char term = seq.back();
    if (term != 'M' && term != 'm') return;

    auto p1 = seq.find(';', 3);
    if (p1 == std::string::npos) return;
    auto p2 = seq.find(';', p1 + 1);
    if (p2 == std::string::npos) return;

    auto to_int = [](std::string_view sv) {
        int v = 0;
        for (char c : sv) { if (c < '0' || c > '9') break; v = v * 10 + (c - '0'); }
        return v;
    };
    int cb  = to_int(std::string_view(seq).substr(3, p1 - 3));
    int col = to_int(std::string_view(seq).substr(p1 + 1, p2 - p1 - 1));
    int row = to_int(std::string_view(seq).substr(p2 + 1, seq.size() - p2 - 2));

    InputData::MouseData m;
    m.col     = col - 1;  // 终端 1 起 → 0 起
    m.row     = row - 1;
    m.isShift = (cb & 4) != 0;
    m.isAlt   = (cb & 8) != 0;
    m.isCtrl  = (cb & 16) != 0;

    auto to_btn = [](int b) {
        return b == 0 ? InputData::MouseButton::LEFT
             : b == 1 ? InputData::MouseButton::MIDDLE
             : b == 2 ? InputData::MouseButton::RIGHT
                      : InputData::MouseButton::NONE;
    };
    int btn = cb & 3;
    if (cb & 64) {                       // 滚轮：方向看最低位
        m.button = InputData::MouseButton::NONE;
        m.action = (cb & 1) ? InputData::MouseAction::SCROLL_DOWN
                            : InputData::MouseAction::SCROLL_UP;
    } else if (cb & 32) {                // 移动
        m.button = to_btn(btn);
        m.action = (btn == 3) ? InputData::MouseAction::MOVE
                              : InputData::MouseAction::DRAG;
    } else {                             // 按下 / 松开
        m.button = to_btn(btn);
        m.action = (term == 'm' || btn == 3) ? InputData::MouseAction::RELEASE
                                             : InputData::MouseAction::PRESS;
    }
    out.push_back(InputData::makeMouse(m));
}

// ===========================================================================
//  input — 输入模块
// ===========================================================================
//
//  接管终端输入：start() 进入 raw 模式、开启鼠标追踪并启动后台解析线程；
//  线程把 stdin 的字节喂给 input_fsm，产出的 InputData 放入内部缓冲队列；
//  消费者通过 read() 从队列取事件。stop() 停线程、关鼠标追踪、恢复终端。
//
//  线程安全：read() / empty() 可与后台线程并发调用。
// ===========================================================================

class input {
  public:
    /**
     * 枚举类型
     *
     * ON ，鼠标键盘启动，模块工作中。
     * 2 , 鼠标没有启动，如果执行`enableMouse`返回这个值，说明终端不兼容鼠标。
     * 3 , 鼠标启动了，但是键盘拿不到，显示这个一定是终端兼容出了问题
     * 0 ，键盘鼠标输入都是无效的，
     * 4 , 模块没有开启输入代理，输入由系统负责
     */
    enum class InputMode : uint8_t {
        ON = true,
        MOUSE_DISABLE = 2,
        KEYBD_DISABLE = 3,
        INPUT_DISABLE = false,
        OFF           = 4
    };

    input() = default;
    ~input() { stop(); }
    input(const input&) = delete;
    input& operator=(const input&) = delete;

    /// 用户接触的接口

    /// 接管输入：进入 raw 模式、开启鼠标追踪、启动后台解析线程（幂等）。
    InputMode start();
    /// 交还输入：停线程、关闭鼠标追踪、恢复终端（幂等）。
    InputMode stop();

    /// 开启 / 关闭鼠标追踪（需已 start）。
    InputMode enableMouse();
    InputMode disableMouse();
    /// 开启 / 关闭键盘事件（关闭后键盘事件被丢弃，仍会被解析）。
    InputMode enableKeyBd();
    InputMode disableKeyBd();

    /// 从缓冲队列取一个事件。
    /// timeout_ms < 0 阻塞直到有事件或被 stop 唤醒；= 0 非阻塞；> 0 最多等待。
    std::optional<InputData> read(int timeout_ms = 0);

    /// 缓冲队列当前是否为空。
    bool empty() const;

  private:
    void reader_loop();
    void enqueue(std::deque<InputData>& produced);

    terminal               term_;
    input_fsm              fsm_;
    std::deque<InputData>  queue_;
    mutable std::mutex     m_;
    std::condition_variable cv_;
    std::atomic<bool>      running_{false};
    std::atomic<bool>      keybd_enabled_{true};
    std::thread            reader_;
};

// ── input 实现 ──────────────────────────────────────────────────────────────

inline input::InputMode input::start() {
    if (running_) return InputMode::ON;
    if (!term_.enter_raw()) return InputMode::INPUT_DISABLE;
    terminal::write("\033[?1000h\033[?1002h\033[?1006h");
    terminal::flush();
    keybd_enabled_ = true;
    running_       = true;
    reader_        = std::thread([this] { reader_loop(); });
    return InputMode::ON;
}

inline input::InputMode input::stop() {
    if (!running_.exchange(false)) return InputMode::OFF;
    cv_.notify_all();
    if (reader_.joinable()) reader_.join();
    {
        std::lock_guard<std::mutex> lock(m_);
        queue_.clear();
    }
    terminal::write("\033[?1006l\033[?1002l\033[?1000l");
    terminal::flush();
    term_.exit_raw();
    return InputMode::OFF;
}

inline input::InputMode input::enableMouse() {
    if (!running_) return InputMode::OFF;
    terminal::write("\033[?1000h\033[?1002h\033[?1006h");
    terminal::flush();
    return InputMode::ON;
}

inline input::InputMode input::disableMouse() {
    if (!running_) return InputMode::OFF;
    terminal::write("\033[?1006l\033[?1002l\033[?1000l");
    terminal::flush();
    return InputMode::ON;
}

inline input::InputMode input::enableKeyBd() {
    keybd_enabled_ = true;
    return running_ ? InputMode::ON : InputMode::OFF;
}

inline input::InputMode input::disableKeyBd() {
    keybd_enabled_ = false;
    return running_ ? InputMode::ON : InputMode::OFF;
}

inline void input::reader_loop() {
    std::deque<InputData> produced;
    while (running_) {
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        int r = ::poll(&pfd, 1, 50);
        if (r < 0) continue;
        if (r == 0) {                 // 超时：收敛孤立的 Esc
            fsm_.flush_pending(produced);
            enqueue(produced);
            continue;
        }
        while (auto b = terminal::read_byte())
            fsm_.feed(*b, produced);
        enqueue(produced);
    }
}

inline void input::enqueue(std::deque<InputData>& produced) {
    if (produced.empty()) return;
    {
        std::lock_guard<std::mutex> lock(m_);
        bool keep_key = keybd_enabled_;
        for (auto& e : produced)
            if (keep_key || e.inputType != InputData::InputType::KEYBD)
                queue_.push_back(std::move(e));
        produced.clear();
    }
    cv_.notify_all();
}

inline std::optional<InputData> input::read(int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_);
    auto ready = [this] { return !queue_.empty() || !running_; };
    if (!ready()) {
        if (timeout_ms < 0) cv_.wait(lock, ready);
        else cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready);
    }
    if (queue_.empty()) return std::nullopt;
    InputData e = std::move(queue_.front());
    queue_.pop_front();
    return e;
}

inline bool input::empty() const {
    std::lock_guard<std::mutex> lock(m_);
    return queue_.empty();
}

} // namespace vaterm

#endif // VATERM_INPUT_HPP
