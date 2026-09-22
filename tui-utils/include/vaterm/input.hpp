#ifndef VATERM_INPUT_HPP
#define VATERM_INPUT_HPP

/**
 * 我打算重新设计输入模块，区别于以前通过获取鼠标输入后手动调用parse()来解析
 * 相关数据，这次我打算完全接管终端模拟器的全部输入，在用户层面不再使用系统的
 * 原生输入。使用状态机来解析输入，解析的结果填入缓冲队列，消费者从缓冲队列
 * 获取键盘和鼠标的输入事件。
 */

#include <cstdint>
#include <vector>
#include <string>

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

class input {
  
  protected:
  
    /// 状态机状态枚举
    enum class ParseStatus {
    KEYBD,MOUSE,MEETESC,OTHER ///< TODO 接到\033就进入MEETESC，再看是不是鼠标
    };

    bool isRunning = false;
    bool isMouseEnable = true;
    bool isKeyBdEnable = true;

    ParseStatus state = ParseStatus::KEYBD;

    std::string _parseBuffer; ///< 模块从终端源源不断读取输入，如果读取到了\033就暂存在这里供最后的解析。
    std::vector< InputData > iptDataFifoBuffer; ///< 本人觉得这里搞一个循环队列比较好

    void submit(unsigned char byte );///< 向状态机提交数据(其实这就是状态转移的操作者)
    void parse (); ///< 一个完整输入报文成立的时候，submit调用此函数处理报文，包装为数据包

    void mainloop(); ///< 这玩意跑起来，不断接受终端输入填入缓区,还负责其他琐事

    /// 这是一个并发的模块，还要记得定义锁啥的

  public:
    /**
     * 枚举类型
     *
     * ON ，鼠标键盘启动，模块工作中。
     * 2 , 鼠标没有启动，如果执行`enableMouse`返回这个值，说明终端不兼容鼠标。
     * 3 , 鼠标启动了，但是键盘拿不到，显示这个一定是终端兼容出了问题
     * 0 ，键盘鼠标输入都是无效的，但是模块本身在运行,这是异常的
     * 4 , 模块没有开启输入代理，输入由系统负责
     */
    enum class InputMode : uint8_t {
        ON = true,
        MOUSE_DISABLE = 2,
        KEYBD_DISABLE = 3,
        INPUT_DISABLE = false,
        OFF           = 4
    };

    /// 用户接触的接口
    InputMode start();
    InputMode enableMouse();
    InputMode disableMouse();
    InputMode enableKeyBd();
    InputMode disableKeyBd();
    InputMode stop();

    InputData getInput( InputData::InputType input_type   ); ///< 从缓冲队列非阻塞读取第一个数据
    InputData getInput(); ///< 从缓冲队列非阻塞读取第一个数据,不指定要的类型
    InputData waitInput( InputData::InputType input_type ); ///< 消费者阻塞等待一个新的输入数据，使用条件变量锁定，在队列更新的时候解锁
    InputData waitInput(); ///< 消费者阻塞等待一个输入数据，不指定要的类型
};

} // namespace vaterm

#endif // VATERM_INPUT_HPP
