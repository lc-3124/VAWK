#ifndef VATERM_INPUT_HPP
#define VATERM_INPUT_HPP

/**
 * lc3124(260923):
 * 我重新设计了输入模块，区别于以前通过获取鼠标输入后手动调用parse()来解析
 * 相关数据，这次我打算编写一个输入引擎，完全接管终端模拟器的全部输入，在用
 * 户层面不再使用系统的原生输入。使用状态机来解析输入，解析的结果填入缓冲队
 * 列，消费者从缓冲队列获取键盘和鼠标的输入事件。
 *
 * TODO 目前还在编写这些代码并且进行测试，在测试和审查完成之后，我会分离定义
 * 和实现。
 */

// 标准库
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

// linux
#include <poll.h>
#include <termios.h>
#include <unistd.h>

// vawk
#include "term.hpp"

/**
 * VATERM_INPUT_BUFFER_SIZE
 *
 * 环形缓冲容量：只保留最近 VATERM_INPUT_BUFFER_SIZE 条事件，超出丢弃最旧的。
 * 可在编译期用 -DVATERM_INPUT_BUFFER_SIZE=N 覆盖。
 */
#ifndef VATERM_INPUT_BUFFER_SIZE
#define VATERM_INPUT_BUFFER_SIZE 64
#endif

namespace vaterm {

// ===========================================================================
//  InputData — 输入数据包
// ===========================================================================

/**
 * lc3124(260923):
 * 定义输入数据包的结构，以便于按照统一、方便解析的方式传递输入动作，
 * 输入数据包的格式兼容xterm协议下鼠标和键盘的全部输入事件。
 */

/**
 * vaterm::InputData
 *
 * 输入数据包：状态机解析出的一个键盘或鼠标事件。
 *
 * lc3124(260923): inputData的负载中，使用联合体来节约内存，毕竟一个输入事件只会
 * 是鼠标或者键盘中的一个，要不然就是空的。
 * 除此之外，inputData还有一些子结构，用于定义InputData的行为和输入本身
 */
struct InputData {
    // ── 子结构定义 ─────────────────────────────────────────────────────────
    // ───────────────────────────────────────────────────────────────────────

    /**
     * vaterm::InputData::InputType
     *
     * 数据包类型，决定联合体中哪个负载成员有效。
     *
     * lc3124(260923):NON 表示“空包”;
     * 一般用户用了非阻塞的输入获取，但缓冲区没有数据
     * 或者阻塞模式的输入获取接口超时了，就会取得一个空包。
     */
    enum class InputType : uint8_t {
        MOUSE = 0,  ///< 鼠标事件
        KEYBD = 1,  ///< 键盘事件
        NON   = 2   ///< 空包（无事件）
    };

    // ── 鼠标 ─────────────────────────────────────────────────────────

    /**
     * vaterm::InputData::MouseButton
     *
     * 鼠标按键。
     *
     * lc3124(260923):滚轮、松开用不到具体按键，归为 NONE。
     */
    enum class MouseButton : uint8_t {
        NONE = 0,  ///< 无按键（滚轮、松开）
        LEFT,      ///< 左键
        MIDDLE,    ///< 中键
        RIGHT      ///< 右键
    };

    /**
     * vaterm::InputData::MouseAction
     *
     * 鼠标动作。
     *
     * lc3124(260923):悬停不是SGR兼容的状态，但是毕竟有松开和按下的动作，
     * 我把无瞬间状态又不是拖拽或按住的的SGR序列认为是鼠标在悬浮
     */
    enum class MouseAction : uint8_t {
        RELEASE = 0,  ///< 松开
        PRESS,        ///< 按下
        DRAG,         ///< 按住拖动
        MOVE,         ///< 无按键移动（悬停）
        SCROLL_UP,    ///< 滚轮上滚
        SCROLL_DOWN   ///< 滚轮下滚
    };

    /**
     * vaterm::InputData::MouseData
     *
     * 鼠标事件负载（坐标为 0 起）。
     *
     * lc3124(260923):SGR返回的坐标是从1开始的，这和我们对终端的行列的认知有偏差
     * 于是我们规范使用以0作为坐标起始位置，这会表现在后续的 parse_mouse 处理中
     */
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

    /**
     * vaterm::InputData::KeyCode
     *
     * 键盘特殊键码（可打印键请用 KeybdData::cp，此时 code 保持 NONE）。
     *
     * lc3124(260923):特殊键无法用字符表示，用枚举区分；书面值可以用KeyPrintf
     * 来编码。
     */
    enum class KeyCode : int {
        NONE = 0,
        UP, DOWN, LEFT, RIGHT,
        HOME, END, PGUP, PGDN, INS, DEL,
        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,
        ESC, TAB, ENTER, BACKSPACE
    };

    /**
     * vaterm::InputData::KeybdData
     *
     * 键盘事件负载。
     *
     * lc3124(260923):cp 存 Unicode 码点（含 CJK,我们把从输入法搞进来的一个完整
     * 的CJK字符判定为一次键盘输入)；
     */
    struct KeybdData {
        char32_t cp      = 0;              ///< 可打印键的 Unicode 码点（无则 0）
        KeyCode  code    = KeyCode::NONE;  ///< 特殊键码（无则 NONE）
        bool     isShift = false;          ///< 是否按下 Shift
        bool     isAlt   = false;          ///< 是否按下 Alt / Meta
        bool     isCtrl  = false;          ///< 是否按下 Ctrl
    };

    // ── 负载 ───────────────────────────────────────────────────────────────
    // ───────────────────────────────────────────────────────────────────────

    InputType inputType = InputType::NON;  ///< 当前包的类型

    /**
     * vaterm::InputData::Value
     *
     * 事件负载；按 inputType 读取 mouse 或 keybd。
     *
     * lc3124(260923):负载在设计上应该是用构造函数构建,而不是其他方式侵入式组装的。
     */
    union Value {
        MouseData mouse;  ///< 鼠标
        KeybdData keybd;  ///< 键盘

        Value();
        ~Value();
    } value;

    // ── 构造数据包 ─────────────────────────────────────────────────────────

    /**
     * vaterm::InputData::InputData()
     *
     * 构造一个空包（inputType = NON）。
     *
     */
    InputData();

    /**
     * vaterm::InputData::makeMouse(...)
     *
     * 参数：
     * const MouseData& m ， 鼠标负载
     *
     * 返回:
     * InputData , inputType 为 MOUSE 的数据包
     *
     * lc3124(260923):统一在这里设置标签和负载。
     */
    static InputData makeMouse(const MouseData& m);

    /**
     * vaterm::InputData::makeKeybd(...)
     *
     * 参数：
     * const KeybdData& k ， 键盘负载
     *
     * 返回:
     * InputData , inputType 为 KEYBD 的数据包
     */
    static InputData makeKeybd(const KeybdData& k);
};

// ── InputData 实现 ──────────────────────────────────────────────────────────

inline InputData::InputData() {}

inline InputData::Value::Value() {}

inline InputData::Value::~Value() {}

inline InputData InputData::makeMouse(const MouseData& m) {
    InputData d;
    d.inputType   = InputType::MOUSE;
    d.value.mouse = m;
    return d;
}

inline InputData InputData::makeKeybd(const KeybdData& k) {
    InputData d;
    d.inputType   = InputType::KEYBD;
    d.value.keybd = k;
    return d;
}

/**
 * lc3124(260923):
 * 此处设计一套方案让用户拿到键码之后可以通过InputPrintf()来获取这个键码对于的书面打印值
 * 以便于用户输键码，避免「解析键码为书面值」的相关代码污染用户的业务。
 */

/**
 * vaterm::encode_utf8(...)
 *
 * 参数：
 * char32_t cp ， 用于接受码点
 *
 * 返回:
 * std::string , 换好的字符串，wstring写起来让人火大，我专门弄了一个utf库用string处理宽字符。
 *
 * lc3124(260923):把 Unicode 码点编码成 UTF-8 字符串（供 KeyPrintf 复用；虽然和vaterm::utf模块的职责
 * 交叉，但是这个模块是输入引擎的必要功能，我不打算让这个模块依赖于vaterm::utf模块）。
 * UTF看的眼睛疼，这段完全是AI实现的，我盯了几行看起来没问题。
 */
inline std::string encode_utf8(char32_t cp) {
    std::string s;

    // 这里放一张表，参考一下utf的编码方式，我们比较首字节大小来判断编码模式，然后把
    // 编码位取出来，合成一个字符
    //   0x00    .. 0x7F     → 1 0xxxxxxx
    //   0x80    .. 0x7FF    → 2 110xxxxx 10xxxxxx
    //   0x800   .. 0xFFFF   → 3 1110xxxx 10xxxxxx 10xxxxxx
    //   0x10000 .. 0x10FFFF → 4 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    if (cp < 0x80) {
        // ASCII：单字节，码点本身就是字节值
        s += static_cast<char>(cp);
    } else if (cp < 0x800) {
        // 2 字节：高 5 位写入首字节，低 6 位写入续字节，移位收集编码位
        // 第一行代码：丢弃7位，拿11000000(0xC0)做OR运算，就能把编码位滤出来，剩下的同理
        s += static_cast<char>(0xC0 | (cp >> 6)); 
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        // 3 字节：CJK
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        // 4 字节：emoji、罕见字
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return s;
}

/**
 * vaterm::KeyPrintf(InputData::KeyCode code)
 *
 * 特殊键码 → 显示串；可打印键（code == NONE）返回空串。
 *
 * 参数：
 * InputData::KeyCode code ， 要打印的特殊键码
 *
 * 返回:
 * std::string_view , 形如 "<F11>"、"<UP>"；NONE 返回 ""
 *
 * lc3124(260923):例：KeyPrintf(InputData::KeyCode::F11) == "<F11>"。
 * 这玩意不是给用户用的，这段程序用于将code字段转化成string，接下来要给下一个
 * 重载函数继续处理
 */
inline constexpr std::string_view KeyPrintf(InputData::KeyCode code) {
    switch (code) {
    case InputData::KeyCode::NONE:      return "";
    case InputData::KeyCode::UP:        return "<UP>";
    case InputData::KeyCode::DOWN:      return "<DOWN>";
    case InputData::KeyCode::LEFT:      return "<LEFT>";
    case InputData::KeyCode::RIGHT:     return "<RIGHT>";
    case InputData::KeyCode::HOME:      return "<HOME>";
    case InputData::KeyCode::END:       return "<END>";
    case InputData::KeyCode::PGUP:      return "<PGUP>";
    case InputData::KeyCode::PGDN:      return "<PGDN>";
    case InputData::KeyCode::INS:       return "<INS>";
    case InputData::KeyCode::DEL:       return "<DEL>";
    case InputData::KeyCode::F1:        return "<F1>";
    case InputData::KeyCode::F2:        return "<F2>";
    case InputData::KeyCode::F3:        return "<F3>";
    case InputData::KeyCode::F4:        return "<F4>";
    case InputData::KeyCode::F5:        return "<F5>";
    case InputData::KeyCode::F6:        return "<F6>";
    case InputData::KeyCode::F7:        return "<F7>";
    case InputData::KeyCode::F8:        return "<F8>";
    case InputData::KeyCode::F9:        return "<F9>";
    case InputData::KeyCode::F10:       return "<F10>";
    case InputData::KeyCode::F11:       return "<F11>";
    case InputData::KeyCode::F12:       return "<F12>";
    case InputData::KeyCode::ESC:       return "<ESC>";
    case InputData::KeyCode::TAB:       return "<TAB>";
    case InputData::KeyCode::ENTER:     return "<ENTER>";
    case InputData::KeyCode::BACKSPACE: return "<BACKSPACE>";
    }
    return "";
}

/**
 * vaterm::KeyPrintf(const InputData::KeybdData& key)
 *
 * 完整键（含普通键）→ 显示串。
 *
 * 参数：
 * const InputData::KeybdData& key ， 键盘负载
 *
 * 返回:
 * std::string , 无修饰的普通键返回字符本身；
 *               其余返回 KeyPrintf(code)解析出来的字面 
 */
inline std::string KeyPrintf(const InputData::KeybdData& key) {
    const bool special = key.code != InputData::KeyCode::NONE;

    if (!special && !key.isCtrl && !key.isAlt && !key.isShift)
        return encode_utf8(key.cp);

    std::string name = special ? std::string(KeyPrintf(key.code)) : encode_utf8(key.cp);

    std::string mods;
    if (key.isCtrl)  mods += "Ctrl+";
    if (key.isAlt)   mods += "Alt+";
    if (key.isShift) mods += "Shift+";
    if (mods.empty()) return name;                          // 无修饰的特殊键：<F11>

    if (special) { name.insert(1, mods); return name; }     // <Ctrl+F11>
    return "<" + mods + name + ">";                         // <Ctrl+a>
}

// ===========================================================================
//  MousePrintf / InputPrintf — 鼠标与通用事件的显示字符串
// ===========================================================================
//
// lc3124(261013): 搞了一堆重载，因为鼠标的事件往往比较复杂，设计组合。用户可以有需自取。

/**
 * vaterm::MousePrintf(InputData::MouseButton b)
 *
 * 鼠标按键 → 显示串。
 *
 * 参数：
 * InputData::MouseButton b ， 鼠标按键
 *
 * 返回:
 * std::string_view , "NONE"/"LEFT"/"MIDDLE"/"RIGHT"
 */
inline constexpr std::string_view MousePrintf(InputData::MouseButton b) {
    switch (b) {
    case InputData::MouseButton::NONE:   return "NONE";
    case InputData::MouseButton::LEFT:   return "LEFT";
    case InputData::MouseButton::MIDDLE: return "MIDDLE";
    case InputData::MouseButton::RIGHT:  return "RIGHT";
    }
    return "";
}

/**
 * vaterm::MousePrintf(InputData::MouseAction a)
 *
 * 鼠标动作 → 显示串。
 *
 * 参数：
 * InputData::MouseAction a ， 鼠标动作
 *
 * 返回:
 * std::string_view , "PRESS"/"RELEASE"/"DRAG"/"MOVE"/"SCROLL_UP"/"SCROLL_DOWN"
 */
inline constexpr std::string_view MousePrintf(InputData::MouseAction a) {
    switch (a) {
    case InputData::MouseAction::RELEASE:     return "RELEASE";
    case InputData::MouseAction::PRESS:       return "PRESS";
    case InputData::MouseAction::DRAG:        return "DRAG";
    case InputData::MouseAction::MOVE:        return "MOVE";
    case InputData::MouseAction::SCROLL_UP:   return "SCROLL_UP";
    case InputData::MouseAction::SCROLL_DOWN: return "SCROLL_DOWN";
    }
    return "";
}

/**
 * vaterm::MousePrintf(const InputData::MouseData& m)
 *
 * 完整鼠标事件 → 显示串。
 *
 * 参数：
 * const InputData::MouseData& m ， 鼠标负载
 *
 * 返回:
 * std::string , 形如 "<MOUSE (5,7) LEFT PRESS Ctrl>"
 */
inline std::string MousePrintf(const InputData::MouseData& m) {
    std::string s = "<MOUSE (";
    s += std::to_string(m.col);
    s += ",";
    s += std::to_string(m.row);
    s += ")";
    if (m.button != InputData::MouseButton::NONE) {
        s += " ";
        s += MousePrintf(m.button);
    }
    s += " ";
    s += MousePrintf(m.action);
    if (m.isCtrl)  s += " Ctrl";
    if (m.isAlt)   s += " Alt";
    if (m.isShift) s += " Shift";
    s += ">";
    return s;
}

/**
 * vaterm::InputPrintf(const InputData& e)
 *
 * 任意输入事件 → 显示串（键盘走 KeyPrintf，鼠标走 MousePrintf）。
 *
 * 参数：
 * const InputData& e ， 任意输入事件
 *
 * 返回:
 * std::string , 事件的书面表示；NON 返回 ""
 *
 * lc3124(260923):给用户一个统一的打印入口，键鼠通吃。
 */
inline std::string InputPrintf(const InputData& e) {
    switch (e.inputType) {
    case InputData::InputType::KEYBD: return KeyPrintf(e.value.keybd);
    case InputData::InputType::MOUSE: return MousePrintf(e.value.mouse);
    case InputData::InputType::NON:   return "";
    }
    return "";
}

/**
 * vaterm::input_fsm — 输入状态机
 *
 * 把终端字节流解析成 InputData 事件。之所以单独成一个类，是为了能脱离
 * 终端、直接喂字节来测试；正常使用请通过 vaterm::input。
 *
 * 状态转移表：
 *   GROUND - 0x1B -> ESC
 *   ESC    - '['  -> CSI - 第 3 字节为 '<' -> MOUSE
 *                 ->终止符 0x40-0x7E -> GROUND（键盘序列）
 *   ESC    - 'O'  -> SS3 - 1 字节 -> GROUND
 *   ESC    - 可打印 -> GROUND（Alt+键）
 *   ESC    - 其它  -> GROUND（产出 Esc，并把该字节重新按 GROUND 处理）
 *   GROUND - 0xC2-0xF7 -> UTF8 - 后续字节 -> GROUND
 *   MOUSE  - 累积到 'M'/'m' -> GROUND（超长则收敛丢弃）
 *
 * lc3124(260923):独立成类是为了可测试；对外只暴露 vaterm::input。
 * 这玩意是输入引擎最复杂的算法部分，用来将原始输入解析为输入事件，
 * 利用状态机实现。
 *
 * TODO lc3124(261013): 咱们得要把这个模块复盘一下，状态转移是我设计的，但代码是dsv4f实现的，
 * 不能保证没有遗漏什么东西。
 */

class input_fsm {
  public:
    /**
     * vaterm::input_fsm::feed(unsigned char b, std::deque<InputData>& out)
     *
     * 喂入一个字节，解析出的事件追加到 out。
     *
     * 参数：
     * unsigned char b ， 输入字节
     * std::deque<InputData>& out ， 事件输出队列
     *
     * 返回:
     * 无
     *
     * lc3124(260923):同一字节可能在新状态下被重新处理，见各 step_* 的返回值。
     */
    void feed(unsigned char b, std::deque<InputData>& out);

    /**
     * vaterm::input_fsm::feed(std::string_view bytes, std::deque<InputData>& out)
     *
     * 喂入一段字节。
     *
     * 参数：
     * std::string_view bytes ， 输入字节串
     * std::deque<InputData>& out ， 事件输出队列
     *
     * 返回:
     * 无
     *
     * lc3124(260923): 把许多字节分成一个一个字节发送给feed的单字节版本。
     */
    void feed(std::string_view bytes, std::deque<InputData>& out);

    /**
     * vaterm::input_fsm::pending() const
     *
     * 是否有尚未完成的序列。
     *
     * 参数：
     * 无
     *
     * 返回:
     * bool , true 表示停在半截序列（CSI / 鼠标 / UTF-8 / 孤立 ESC）
     *
     * lc3124(260923): 供调用方判断是否需要继续等后续字节。
     */
    bool pending() const;

    /**
     * vaterm::input_fsm::flush_pending(std::deque<InputData>& out)
     *
     * 超时收尾：把孤立的 ESC 收敛成 Esc 键事件。
     *
     * 参数：
     * std::deque<InputData>& out ， 事件输出队列
     *
     * 返回:
     * 无
     *
     * lc3124(260923): 超时说明这玩意可能不是escape事件，而是一个单纯的输入，
     * 也有可能终端出现错误，导致没有完整传递输入动作；总之就是不要了。
     */
    void flush_pending(std::deque<InputData>& out);

    /**
     * vaterm::input_fsm::reset()
     *
     * 复位状态机（丢弃半截序列）。
     *
     * 参数：
     * 无
     *
     * 返回:
     * 无
     *
     * lc3124(260923):解析成功或失步时调用。
     */
    void reset();

  private:
    enum class State { GROUND, ESC, CSI, SS3, MOUSE, UTF8 };

    State       state_ = State::GROUND;
    std::string acc_;  ///< 当前序列已累积的字节

    /**
     * vaterm::input_fsm::push_key(...)
     *
     * 构造一个键盘事件追加到 out。
     *
     * 参数：
     * std::deque<InputData>& out ， 事件输出队列
     * char32_t cp ， 可打印键的码点（特殊键传 0）
     * InputData::KeyCode code ， 特殊键码
     * bool shift/alt/ctrl ， 修饰键
     *
     * 返回:
     * 无
     */
    static void push_key(std::deque<InputData>& out, char32_t cp,
                         InputData::KeyCode code,
                         bool shift = false, bool alt = false, bool ctrl = false);

    /**
     * vaterm::input_fsm::step_* 系列
     *
     * 各状态处理函数。
     *
     * 参数：
     * unsigned char b ， 当前字节
     * std::deque<InputData>& out ， 事件输出队列
     *
     * 返回:
     * bool , true 表示同一字节需在新状态下重新处理
     */
    bool step_ground(unsigned char b, std::deque<InputData>& out);
    bool step_esc(unsigned char b, std::deque<InputData>& out);
    bool step_csi(unsigned char b, std::deque<InputData>& out);
    bool step_ss3(unsigned char b, std::deque<InputData>& out);
    bool step_mouse(unsigned char b, std::deque<InputData>& out);
    bool step_utf8(unsigned char b, std::deque<InputData>& out);


    /**
     * lc3124(261013): 拿到完整的csi序列还不够，传入这里解析成事件。
     */
    /**
     * vaterm::input_fsm::parse_csi(...)
     *
     * 解析 CSI 键盘序列。
     *
     * 参数：
     * std::string_view params ， 去头去尾后的参数串
     * char final ， 终止字节
     * std::deque<InputData>& out ， 事件输出队列
     *
     * 返回:
     * 无
     */
    static void parse_csi(std::string_view params, char final, std::deque<InputData>& out);

    /**
     * vaterm::input_fsm::parse_mouse(...)
     *
     * 解析一条完整的 SGR 鼠标序列。
     *
     * 参数：
     * const std::string& seq ， 形如 \033[<Cb;Cx;CyM/m
     * std::deque<InputData>& out ， 事件输出队列
     *
     * 返回:
     * 无
     */
    static void parse_mouse(const std::string& seq, std::deque<InputData>& out);
};

// ── input_fsm 实现 ──────────────────────────────────────────────────────────

inline void input_fsm::feed(unsigned char b, std::deque<InputData>& out) {
    bool reprocess = true; ///< 这个字节还要不要处理？要！
    while (reprocess) {
        switch (state_) {
          // 根据当前状态步进
        case State::GROUND: reprocess = step_ground(b, out); break;
        case State::ESC:    reprocess = step_esc(b, out);    break;
        case State::CSI:    reprocess = step_csi(b, out);    break;
        case State::SS3:    reprocess = step_ss3(b, out);    break;
        case State::MOUSE:  reprocess = step_mouse(b, out);  break;
        case State::UTF8:   reprocess = step_utf8(b, out);   break;
        }
    }
}


inline void input_fsm::feed(std::string_view bytes, std::deque<InputData>& out) {
    for (unsigned char b : bytes) feed(b, out); ///< 投喂全部字节
}

// 如果当前状态不是GROUND，说明还在处理其他东西
inline bool input_fsm::pending() const {
    return state_ != State::GROUND;
}

// 结束当前的解析任务
inline void input_fsm::flush_pending(std::deque<InputData>& out) {
    if (state_ == State::ESC) {
        push_key(out, 0, InputData::KeyCode::ESC);
        reset();
    }
}

inline void input_fsm::reset() {
    state_ = State::GROUND;
    acc_.clear();
}

inline void input_fsm::push_key(std::deque<InputData>& out, char32_t cp,
                                InputData::KeyCode code,
                                bool shift, bool alt, bool ctrl) {
    out.push_back(InputData::makeKeybd({cp, code, shift, alt, ctrl}));
}

/**
 * vaterm::input_fsm::step_ground(...)
 *
 * GROUND（地面态）：不在任何序列之中、逐字节识别普通输入的“默认状态”。
 * 绝大多数按键都在这里一次处理完；只有 ESC、UTF-8 首字节会把状态机推向别的状态。
 *
 * 参数：
 * unsigned char b ， 当前字节
 * std::deque<InputData>& out ， 事件输出队列
 *
 * 返回:
 * bool , 恒为 false —— GROUND 是收敛态，不会出现“同一字节需在新状态再处理”的
 *        情况（返回值约定见 step_* 系列总说明：true = 请在新状态下重处理该字节）。
 *
 * lc3124(260923):字节分类按优先级依次判断：转义(0x1B) → 具名控制键 → Ctrl 组合键
 * → 可打印 ASCII → UTF-8 首字节 → 其余丢弃。
 */
inline bool input_fsm::step_ground(unsigned char b, std::deque<InputData>& out) {
    using KC = InputData::KeyCode;

    // 0x1B = ESC：它既可能是“单独按下的 Esc 键”，也可能是某个转义序列（CSI/SS3）的开头，
    // 此刻仅凭一个字节无法区分。于是转入 ESC 态，把已见字节（就它自己）存进 acc_，
    // 交给后续字节、或超时时的 flush_pending() 来最终定夺。
    if (b == 0x1B) { state_ = State::ESC; acc_.assign(1, '\033'); return false; }

    // ── 三个有专属键名的控制字符 ────────────────────────────────────────
    if (b == 0x09) { push_key(out, 0, KC::TAB);       return false; }  // Tab
    if (b == 0x0D) { push_key(out, 0, KC::ENTER);     return false; }  // 回车(CR)
    if (b == 0x7F) { push_key(out, 0, KC::BACKSPACE); return false; }  // 退格(DEL)

    // 0x00 = Ctrl+Space（也常记作 Ctrl+@）：载荷用空格码点，修饰位标 Ctrl。
    if (b == 0x00) { push_key(out, U' ', KC::NONE, false, false, true); return false; }

    // 0x01-0x1A = Ctrl+a .. Ctrl+z：
    // 终端里 Ctrl+字母 产生的就是该字母 ASCII 码减 0x40，反过来 b-1 即字母序号
    // （0x01→'a'，0x1A→'z'）。
    if (b >= 0x01 && b <= 0x1A) {
        push_key(out, static_cast<char32_t>('a' + b - 1), KC::NONE, false, false, true);
        return false;
    }

    // 0x1C-0x1F = Ctrl+\ Ctrl+] Ctrl+^ Ctrl+_：
    // 这几个不落在字母区间内，需要单独映射（同样由 ASCII 码减 0x40 得出）。
    if (b >= 0x1C && b <= 0x1F) {
        char32_t cp = b == 0x1C ? U'\\' : b == 0x1D ? U']' : b == 0x1E ? U'^' : U'_';
        push_key(out, cp, KC::NONE, false, false, true);
        return false;
    }

    // 0x20-0x7E = 可打印 ASCII：字节值本身就是字符码点，直接产出一个无修饰键事件。
    if (b >= 0x20 && b <= 0x7E) {
        push_key(out, static_cast<char32_t>(b), KC::NONE);
        return false;
    }

    // 0xC2-0xF7 = UTF-8 首字节区间，按首字节大小决定后续要几个字节：
    //   0xC2-0xDF 两字节、0xE0-0xEF 三字节（含 CJK）、0xF0-0xF7 四字节（emoji 等）。
    // 不含 0xC0/0xC1（属“过长编码”，非法），也不含 0xF8 及以上（非 UTF-8）。
    // 此处只把首字节存入 acc_ 并转入 UTF8 态，等续字节到齐后再解码出码点。
    if (b >= 0xC2 && b <= 0xF7) {
        state_ = State::UTF8;
        acc_.assign(1, static_cast<char>(b));
        return false;
    }

    // 其余字节（如 0x80-0xBF 的孤立续字节、0xF8-0xFF 等）无法识别，直接丢弃。
    return false;
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
    auto semi   = params.find(';');
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

/**
 * vaterm::input — 输入模块
 *
 * 接管终端输入：start() 开启鼠标追踪并启动后台解析线程；线程把 stdin 的
 * 字节喂给 input_fsm，产出的 InputData 放进定长环形缓冲（只保留最近若干条，
 * 为了省内存，已入队事件不会被取走）；消费者用 getInput() 查询最新事件，
 * 或用 waitInput() 等待新的匹配事件。stop() 停线程、关鼠标追踪。
 * （raw 模式的进入/退出不属于本模块，由调用方负责。）
 *
 * 线程安全：getInput() / waitInput() / getState() 可与后台线程并发调用。
 *
 * lc3124(260923):对外只暴露 start/stop、enableMouse/enableKeyBd、getInput/waitInput/getState。
 */

class input {
    // 前置私有定义：倾倒器 dumper 会引用缓冲条目
    struct Entry {
        std::uint64_t seq;   ///< 单调递增写入序号
        InputData     data;  ///< 事件
    };

  public:
    /**
     * vaterm::input::InputMode
     *
     * 模块工作状态。
     *
     * ON ，鼠标键盘启动，模块工作中。
     * 2 , 鼠标没有启动，如果执行`enableMouse`返回这个值，说明终端不兼容鼠标。
     * 3 , 鼠标启动了，但是键盘拿不到，显示这个一定是终端兼容出了问题
     * 0 ，键盘鼠标输入都是无效的，
     * 4 , 模块没有开启输入代理，输入由系统负责
     *
     */
    enum class InputMode : uint8_t {
        ON = true,
        MOUSE_DISABLE = 2,
        KEYBD_DISABLE = 3,
        INPUT_DISABLE = false,
        OFF           = 4
    };

    input();
    ~input();
    input(const input&) = delete;
    input& operator=(const input&) = delete;

    /** 用户接触的接口 */

    /**
     * vaterm::input::start()
     *
     * 接管输入：开启鼠标追踪、启动后台解析线程。
     *
     * 参数：
     * 无
     *
     * 返回:
     * bool , 成功返回 true
     *
     * lc3124(260923):raw mode 需调用方预先开启；未处于 raw 模式时返回 false、不启动。
     * 这样设计是因为我的预想设计中，input模块由再次封装，raw模式的切换已经在vatui
     * 的构造和析构种完成。
     */
    bool start();

    /**
     * vaterm::input::stop()
     *
     * 交还输入：停线程、关闭鼠标追踪。
     *
     * 参数：
     * 无
     *
     * 返回:
     * bool , 成功返回 true
     *
     */
    bool stop();

    /**
     * vaterm::input::enableMouse() / disableMouse()
     *
     * 开启 / 关闭鼠标追踪。
     *
     * 参数：
     * 无
     *
     * 返回:
     * bool , 成功返回 true（未 start 返回 false）
     *
     * lc3124(260923):开关会同步更新 getState()。
     */
    bool enableMouse();
    bool disableMouse();

    /**
     * vaterm::input::enableKeyBd() / disableKeyBd()
     *
     * 开启 / 关闭键盘事件。
     *
     * 参数：
     * 无
     *
     * 返回:
     * bool , 成功返回 true（未 start 返回 false）
     *
     * lc3124(260923):关闭后键盘事件被丢弃，但仍会被解析。
     */
    bool enableKeyBd();
    bool disableKeyBd();

    /**
     * vaterm::input::getState() const
     *
     * 查询当前工作状态。
     *
     * 参数：
     * 无
     *
     * 返回:
     * InputMode , ON / OFF / MOUSE_DISABLE / KEYBD_DISABLE / INPUT_DISABLE
     *
     * lc3124(260923):线程安全，可随时调用。
     */
    InputMode getState() const;

    /**
     * vaterm::input::getInput(...)
     *
     * 非阻塞读取：从缓冲区历史里读最新的一条事件，不取走、不消费。
     *   getInput(type) ：符合 type 的最新事件
     *   getInput()     ：不限类型，取最新的一条
     *
     * 参数：
     * InputData::InputType type ， 想要的类型；传 NON 表示不限类型
     *
     * 返回:
     * InputData , 最新的一条匹配事件；没有则 NON
     *
     */
    InputData getInput(InputData::InputType type);
    InputData getInput();

    /**
     * vaterm::input::waitInput(...)
     *
     * 阻塞等待新的匹配事件，不取走、不消费。
     *   waitInput(type, timeout_ms) ：等待符合 type 的新事件
     *   waitInput(timeout_ms)       ：不限类型，等待任意新事件
     *
     * 参数：
     * InputData::InputType type ， 想要的类型；传 NON 表示不限类型
     * int timeout_ms ， 负数无限等待；>= 0 有限等待
     *
     * 返回:
     * InputData , 新的匹配事件；超时或被 stop 返回 NON
     *
     */
    InputData waitInput(InputData::InputType type, int timeout_ms = -1);
    InputData waitInput(int timeout_ms = -1);

    /**
     * vaterm::input::dumper
     *
     * 倾倒器：每个消费者各持一个，拥有自己的游标，从旧到新按序号顺序消费共享
     * 缓冲区里的事件（不去重、不折叠），因此多个对象可各自读全量流。
     *
     * lc3124(261012):这个设计是为了解决一个麻烦：原始输入的处理和输入消息的缓存是异步的，
     * 导致上面两个接口有可能因为原始输入过快而漏掉输入（比如输入法一次性输入一大段句子）
     * ，所以在希望获取一次准确的用户输入的时候，
     * 使用倾倒器可以从队列的历史消息中跟进输入来保证输入的准确。
     * 倾倒器的作用就是确保输入的东西不会遗漏，除非缓冲区耗尽。
     *
     * NOTICE 注意，缓冲区耗尽会导致倾倒器的游标被强制前移，导致数据丢失，可以设置宏来改变缓冲区大小。
     */
    class dumper {
      public:
        /// 构造：把游标对齐到 input 当前最新（只倾倒此后新产生的事件）。
        explicit dumper(input& in);

        dumper(const dumper&) = delete;
        dumper& operator=(const dumper&) = delete;

        /**
         * vaterm::input::dumper::dump(...)
         *
         * 倾倒一条：返回游标之后“最旧”的事件，并把游标前进到它；已追平最新则等待。
         *
         * 参数：
         * int timeout_ms ， 负数无限等待；>= 0 有限等待
         *
         * 返回:
         * InputData , 下一条未倾倒的事件；超时或被 stop 返回 NON
         */
        InputData dump(int timeout_ms = -1);

        /// 重置：游标对齐回当前最新（忽略此前积压），并把丢帧计数清零。
        void reset();

        /// 自构造 / 上次 reset 以来，被环形缓冲覆盖而丢掉的事件条数。
        std::uint64_t lost() const;

      private:
        const Entry* next_unread();   ///< 游标之后最旧的一条（需持 in_->m_）

        input*        in_;
        std::uint64_t cursor_     = 0;   ///< 最后一条已倾倒事件的序号
        /**
         * lc3124(261012): 引擎世代设计是为了检测当前环形缓冲的游标是否失效，
         * 输入框架热重载的时候会重置缓冲区，这时候倾倒器的游标是错误的，
         * 输入框架会在每次重载的时候更新世代，倾倒器跟进这个世代就可以知道自己是不是
         * 最新的，以便于游标回到正确位置。
         */
        std::uint64_t generation_ = 0;   ///< 构造/重置时的引擎世代
        std::uint64_t lost_       = 0;   ///< 累计丢帧
    };


    //--- 咳咳，我们还在input类中，注意 ---
  private:
    void reader_loop();
    void enqueue(std::deque<InputData>& produced);
    void update_state();

    /**
     * vaterm::input::match(...) / latest(...)
     *
     * 在持有 m_ 时调用的查询辅助。
     *
     * 参数：
     * InputData::InputType type ， 想要的类型；NON 表示不限
     * const InputData& d ， 待判定的数据包
     *
     * 返回:
     * match → bool ；latest → InputData
     */
    static bool match(InputData::InputType type, const InputData& d);
    InputData   latest(InputData::InputType type) const;   ///< 历史里最新的匹配项

    input_fsm               fsm_;       ///< 状态机实例
    std::deque<Entry>       buffer_;   ///< 定长环形缓冲，超出容量丢弃最旧的
    std::uint64_t           seq_ = 0;  ///< 最近一次写入的序号
    std::uint64_t           generation_ = 0;  ///< 引擎世代
    mutable std::mutex      m_;
    std::mutex              lifecycle_;   ///< 串行化 start/stop
    bool                    started_ = false;  ///< 是否已接管终端
    std::condition_variable cv_;          ///< 用于waitInput消费者
    std::atomic<bool>       running_{false};
    std::atomic<bool>       mouse_enabled_{false};
    std::atomic<bool>       keybd_enabled_{true};
    std::atomic<InputMode>  state_{InputMode::OFF};
    std::thread             reader_;      ///< 线程句柄
};

// ── input 实现 ──────────────────────────────────────────────────────────────

inline input::input() = default;

inline input::~input() { stop(); }


/**
 * 完成启动的一些环境变量的设置，启动工作线程
 */
inline bool input::start() {
    std::lock_guard<std::mutex> lk(lifecycle_);
    if (running_) return true;

    // 终端必须已经处于 raw 模式（非规范模式）：否则不接管输入，直接返回 false。
    struct termios tio{};
    if (::tcgetattr(STDIN_FILENO, &tio) != 0 || (tio.c_lflag & ICANON) != 0) {
        state_ = InputMode::INPUT_DISABLE;
        return false;
    }

    terminal::write("\033[?1000h\033[?1002h\033[?1006h");
    terminal::flush();
    started_ = true;
    {
        std::lock_guard<std::mutex> lock(m_);
        buffer_.clear();
        seq_ = 0;
        ++generation_;          // 引擎已清空：所有 dumper 需重对齐游标
    }
    if (reader_.joinable()) reader_.join();   // 回收可能已结束的旧线程
    mouse_enabled_ = true;
    keybd_enabled_ = true;
    running_       = true;
    reader_        = std::thread([this] { reader_loop(); });
    state_         = InputMode::ON;
    return true;
}

inline bool input::stop() {
    std::lock_guard<std::mutex> lk(lifecycle_);
    running_ = false;
    cv_.notify_all();
    if (reader_.joinable()) reader_.join();
    {
        std::lock_guard<std::mutex> lock(m_);
        buffer_.clear();
        seq_ = 0;
        ++generation_;          // 引擎已清空：所有 dumper 需重对齐游标
    }
    // 写终止序列，如果没有跑过引擎就不打扰终端。
    if (started_) {
        terminal::write("\033[?1006l\033[?1002l\033[?1000l");
        terminal::flush();
        started_ = false;
    }
    state_ = InputMode::OFF;
    return true;
}

inline bool input::enableMouse() {
    if (!running_) return false;
    terminal::write("\033[?1000h\033[?1002h\033[?1006h");
    terminal::flush();
    mouse_enabled_ = true;
    update_state();
    return true;
}

inline bool input::disableMouse() {
    if (!running_) return false;
    terminal::write("\033[?1006l\033[?1002l\033[?1000l");
    terminal::flush();
    mouse_enabled_ = false;
    update_state();
    return true;
}

inline bool input::enableKeyBd() {
    if (!running_) return false;
    keybd_enabled_ = true;
    update_state();
    return true;
}

inline bool input::disableKeyBd() {
    if (!running_) return false;
    keybd_enabled_ = false;
    update_state();
    return true;
}

inline input::InputMode input::getState() const {
    return state_.load();
}

inline void input::update_state() {
    bool k  = keybd_enabled_;
    bool mo = mouse_enabled_;
    state_  = (k && mo)  ? InputMode::ON
            : (k && !mo) ? InputMode::MOUSE_DISABLE
            : (!k && mo) ? InputMode::KEYBD_DISABLE
                         : InputMode::INPUT_DISABLE;
}


/**
 * lc3124(261012): 这一段代码不是我写的，我需要花一些时间来理清楚这玩意。
 */
inline void input::reader_loop() {
    std::deque<InputData> produced;
    int idle_ready = 0;   // 连续“poll 说可读却读不到”的次数，用于识别 EOF
    while (running_) {
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        int r = ::poll(&pfd, 1, 50);
        if (r < 0) continue;

        /**
         * 如果终端发来escape序列，按理来说应该读到序列结束，但是如果esc序列
         * 长时间没有终止，那就不等来，直接收敛
         */
        if (r == 0) {                 // 超时：收敛孤立的 Esc
            idle_ready = 0;
            fsm_.flush_pending(produced);
            enqueue(produced);
            continue;
        }

        // 只排空“此刻确实可读”的字节：用 poll(0) 预检，
        // 避免 read_byte() 在非 raw / 管道下阻塞住。
        bool got_any = false;
        for (;;) {
            struct pollfd q = {STDIN_FILENO, POLLIN, 0};
            if (::poll(&q, 1, 0) <= 0) break;
            auto b = terminal::read_byte();
            if (!b) break;
            fsm_.feed(*b, produced);
            got_any = true;
        }
        enqueue(produced);

        // 输入源结束：HUP/ERR/NVAL，或反复“可读却无数据”。
        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            running_ = false;
            state_   = InputMode::INPUT_DISABLE;
            cv_.notify_all();
            break;
        }
        if (got_any) {
            idle_ready = 0;
        } else if (++idle_ready > 3) {
            running_ = false;
            state_   = InputMode::INPUT_DISABLE;
            cv_.notify_all();
            break;
        }
    }
}

/** 
 * lc3124(261012):这段没注释更是让人头大
 * 看起来是输入事件缓冲有关的
 */
inline void input::enqueue(std::deque<InputData>& produced) {
    if (produced.empty()) return;
    bool pushed = false;
    {
        std::lock_guard<std::mutex> lock(m_);
        bool keep_key = keybd_enabled_;
        for (auto& e : produced) {
            if (!keep_key && e.inputType == InputData::InputType::KEYBD) continue;
            buffer_.push_back(Entry{++seq_, std::move(e)});
            pushed = true;
        }
        while (buffer_.size() > VATERM_INPUT_BUFFER_SIZE)
            buffer_.pop_front();
        produced.clear();
    }
    if (pushed) cv_.notify_all();
}

inline bool input::match(InputData::InputType type, const InputData& d) {
    return type == InputData::InputType::NON || d.inputType == type;
}

inline InputData input::latest(InputData::InputType type) const {
    for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it)
        if (match(type, it->data)) return it->data;
    return InputData{};
}

inline InputData input::getInput(InputData::InputType type) {
    std::lock_guard<std::mutex> lock(m_);
    return latest(type);
}

inline InputData input::getInput() {
    std::lock_guard<std::mutex> lock(m_);
    return latest(InputData::InputType::NON);
}

inline InputData input::waitInput(InputData::InputType type, int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_);
    const std::uint64_t start = seq_;
    // “最近更新的那一条”既要是新的（序号 > start），类型也要匹配。
    auto newest_matches = [&] {
        return !buffer_.empty() && buffer_.back().seq > start
               && match(type, buffer_.back().data);
    };
    auto ready = [&] { return !running_ || newest_matches(); };

    if (timeout_ms < 0) {
        cv_.wait(lock, ready);
    } else if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready)) {
        return InputData{};                 // 超时 → NON
    }
    if (!running_ || !newest_matches()) return InputData{};   // 被 stop / 最新不是要的
    return buffer_.back().data;             // 返回最近更新的那一条
}

inline InputData input::waitInput(int timeout_ms) {
    return waitInput(InputData::InputType::NON, timeout_ms);
}


/**
 * 倾倒器实现
 */

// 构造
inline input::dumper::dumper(input& in) : in_(&in) {
    std::lock_guard<std::mutex> lock(in_->m_);
    cursor_     = in_->seq_;
    generation_ = in_->generation_;
}

// 从旧到新，找游标之后“最旧”的一条（需持 in_->m_）。
inline const input::Entry* input::dumper::next_unread() {
    for (const auto& e : in_->buffer_)
        if (e.seq > cursor_) return &e;
    return nullptr;
}

inline InputData input::dumper::dump(int timeout_ms) {
    std::unique_lock<std::mutex> lock(in_->m_);

    // 引擎被清空/重启,世代变了,游标重对齐到最新，避免“冲到未来”卡死。
    if (generation_ != in_->generation_) {
        cursor_     = in_->seq_;
        generation_ = in_->generation_;
    }

    auto ready = [&] { return !in_->running_ || next_unread() != nullptr; };
    if (!ready()) {
        if (timeout_ms < 0) {  ///< 无时限：一直等到 ready() 为真（有未读事件，或引擎停止）
            in_->cv_.wait(lock, ready);
        } else if (!in_->cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready)) {
            return InputData{};        // 超时 → NON
        }
    }

    const Entry* e = next_unread();
    if (!e) return InputData{};        // 被 stop / 无新事件
    if (e->seq > cursor_ + 1)          // 中间有被环形覆盖的 → 记丢帧
        lost_ += e->seq - cursor_ - 1;
    cursor_ = e->seq;                  // 游标前进到这一条
    return e->data;
}

inline void input::dumper::reset() {
    std::lock_guard<std::mutex> lock(in_->m_);
    cursor_     = in_->seq_;
    generation_ = in_->generation_;
    lost_       = 0;
}

inline std::uint64_t input::dumper::lost() const {
    std::lock_guard<std::mutex> lock(in_->m_);
    return lost_;
}

} // namespace vaterm

#endif // VATERM_INPUT_HPP
