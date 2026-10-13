#ifndef VATERM_ENUMS_HPP
#define VATERM_ENUMS_HPP

// -----------------------------------------------------------------------
//  vaterm::enums — shared enumerations and simple data types
//
//  Provides the common type definitions used throughout the vaterm
//  library. All values correspond to standard ANSI / ECMA-48 parameters.
// -----------------------------------------------------------------------

#include <cstdint>

namespace vaterm {

// ---- Terminal color-depth detection ------------------------------------
//  Ordered from lowest to highest so the enum values can be compared:
//    C4 < C8 < C24
enum class ColorDepth : uint8_t {
    C4  = 4,
    C8  = 8,
    C24 = 24,
};

// ---- Cursor direction -------------------------------------------------
//  Used by cursor::move() to specify which way to shift the cursor.

enum class CursorDir : uint8_t {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};

// ---- Cursor shapes (DECSCUSR) -----------------------------------------
//  VTxxx cursor-style codes. Supported in xterm, kitty, foot, and most
//  modern terminals. Values 1-6 correspond to the DECSCUSR parameter.

enum class CursorShape : uint8_t {
    BLINKING_BLOCK      = 1,
    STEADY_BLOCK        = 2,
    BLINKING_UNDERLINE  = 3,
    STEADY_UNDERLINE    = 4,
    BLINKING_BAR        = 5,
    STEADY_BAR          = 6,
};

// ---- ANSI text effects (SGR parameters) -------------------------------
//  Select Graphic Rendition parameters. Not all terminals support every
//  effect; RAPID_BLINK in particular is often ignored.

enum class TextEffect : uint8_t {
    RESET           = 0,
    BOLD            = 1,
    DIM             = 2,
    ITALIC          = 3,
    UNDERLINE       = 4,
    SLOW_BLINK      = 5,
    RAPID_BLINK     = 6,
    REVERSE         = 7,
    CONCEAL         = 8,
    STRIKETHROUGH   = 9,
};

// ---- 4-bit ANSI color palette -----------------------------------------
//  The sixteen standard terminal colors. Values 0-7 are the "dark" set;
//  values 8-15 are the corresponding "bright" variants.

enum class Color4 : uint8_t {
    BLACK           = 0,
    RED             = 1,
    GREEN           = 2,
    YELLOW          = 3,
    BLUE            = 4,
    MAGENTA         = 5,
    CYAN            = 6,
    WHITE           = 7,
    BRIGHT_BLACK    = 8,
    BRIGHT_RED      = 9,
    BRIGHT_GREEN    = 10,
    BRIGHT_YELLOW   = 11,
    BRIGHT_BLUE     = 12,
    BRIGHT_MAGENTA  = 13,
    BRIGHT_CYAN     = 14,
    BRIGHT_WHITE    = 15,
};

// ---- RGB color triplet ------------------------------------------------
//  Used by the 24-bit true-color API and the color-space conversion
//  helpers (rgb_to_256, _256_to_rgb, etc.).

struct Rgb {
    uint8_t r, g, b;
};

// ---- 8-bit (256-colour) index wrapper ----------------------------------
//  Explicit wrapper to distinguish 8-bit color from a raw uint8_t in
//  overload resolution.
struct Color8 {
    uint8_t index;
};

// ---- Transparent colour tag --------------------------------------------
//  When used as fg/bg in a Style initialiser, produces an empty SGR
//  string.  On importFrame this means "skip this plane" (no fg / no bg).
//  On output the terminal renders its default colour (or the underlying
//  destination content).
struct Trans {};
inline constexpr Trans TRANS{};

// ---- input module enums -------------------------------------------------
//  (vaterm::input / vaterm::InputData 使用；定义集中在此，声明见 vaterm/input.hpp)

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

} // namespace vaterm

#endif // VATERM_ENUMS_HPP
