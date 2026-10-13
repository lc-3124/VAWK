#ifndef VATERM_INPUT_HPP
#define VATERM_INPUT_HPP

/**
 * lc3124(260923):
 * 我重新设计了输入模块，区别于以前通过获取鼠标输入后手动调用parse()来解析
 * 相关数据，这次我打算编写一个输入引擎，完全接管终端模拟器的全部输入，在用
 * 户层面不再使用系统的原生输入。使用状态机来解析输入，解析的结果填入缓冲队
 * 列，消费者从缓冲队列获取键盘和鼠标的输入事件。
 *
 * lc3124(261013): 测试和审查已经完成，定义与实现已经分离。本文件只保留声明，
 * 实现见 src/vaterm/input.cpp——这是 vaterm 里唯一非 header-only 的模块，
 * 使用它的程序需要链接该对象文件。
 */

// 标准库
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

// linux
#include <poll.h>
#include <termios.h>
#include <unistd.h>

// vawk
#include "enums.hpp"

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

    // ---- 枚举：定义在 vaterm/enums.hpp，这里保留声明（别名）----
    // 用 ::vaterm:: 前缀是为了避免在这里解析到别名自身（自引用）。
    using InputType   = ::vaterm::InputType;
    using MouseButton = ::vaterm::MouseButton;
    using MouseAction = ::vaterm::MouseAction;
    using KeyCode     = ::vaterm::KeyCode;

    // ── 鼠标 ─────────────────────────────────────────────────────────

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
 * lc3124(260923):把 Unicode 码点编码成 UTF-8 字符串供 KeyPrintf 复用；虽然和vaterm::utf模块
 * 的职责交叉，但是这个模块是输入引擎的必要功能，我不打算让这个模块依赖于vaterm::utf模块。
 */
std::string encode_utf8(char32_t cp);

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
std::string KeyPrintf(const InputData::KeybdData& key);

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
std::string MousePrintf(const InputData::MouseData& m);

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
std::string InputPrintf(const InputData& e);

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
    // 私有实现细节（FSM 内部状态），不对外暴露，故留在本类原地，不进 vaterm/enums.hpp。
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
    // 定义在 vaterm/enums.hpp；这里保留声明（别名），用法不变。
    using InputMode = ::vaterm::InputMode;

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


    //--- 咳咳，我们还在input类中，注意层次 ---
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

} // namespace vaterm

#endif // VATERM_INPUT_HPP
