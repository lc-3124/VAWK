#ifndef VATERM_ENUMS_HPP
#define VATERM_ENUMS_HPP

//- vaterm::enums — 共享枚举与简单数据类型
//
// 提供整个 vaterm 库通用的类型定义。所有取值都对应标准的
// ANSI / ECMA-48 参数。
//
// 前缀带 InputData:: / input:: 的枚举定义在这里集中管理，
// 在 vaterm/input.hpp 内用 using 别名保留原来的书写方式。

#include <cstdint>

namespace vaterm {

//- 终端色深检测
//  从低到高排序，便于比较枚举值：C4 < C8 < C24

/**
 * @brief 终端支持的颜色深度，取值本身即为位数（4 / 8 / 24）。
 */
enum class ColorDepth : uint8_t {
    C4  = 4,
    C8  = 8,
    C24 = 24,
};

//- 光标方向
//  供 cursor::move() 使用，指定光标移动的方向。

/**
 * @brief 光标移动方向。
 */
enum class CursorDir : uint8_t {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};

//- 光标形状 (DECSCUSR)
//  VTxxx 光标样式码。xterm、kitty、foot 及大多数现代终端都支持。
//  取值 1-6 对应 DECSCUSR 参数。

/**
 * @brief 光标形状，取值对应 DECSCUSR 参数 1-6。
 */
enum class CursorShape : uint8_t {
    BLINKING_BLOCK      = 1,
    STEADY_BLOCK        = 2,
    BLINKING_UNDERLINE  = 3,
    STEADY_UNDERLINE    = 4,
    BLINKING_BAR        = 5,
    STEADY_BAR          = 6,
};

//- ANSI 文本效果 (SGR 参数)
//  Select Graphic Rendition 参数。并非所有终端都支持每种
//  效果；尤其是 RAPID_BLINK 常被忽略。

/**
 * @brief SGR 文本效果，取值即 SGR 参数值。
 */
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

//- 4-bit ANSI 调色板
//  十六种标准终端颜色。取值 0-7 为"暗色"组；
//  取值 8-15 为对应的高亮变体。

/**
 * @brief 4-bit ANSI 标准颜色。
 */
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

//- RGB 三元组
//  供 24-bit 真彩色接口与色彩空间转换辅助函数使用
//  （rgb_to_256、_256_to_rgb 等）。

/**
 * @brief 24-bit 真彩色的 R、G、B 分量。
 */
struct Rgb {
    uint8_t r, g, b;
};

//- 8-bit (256-colour) 索引封装
//  显式封装，用于在重载解析中把 8-bit 颜色与裸 uint8_t 区分开。

/**
 * @brief 8-bit 256 色索引封装，成员为颜色下标 0-255。
 */
struct Color8 {
    uint8_t index;
};

//- 透明色标记
//  在 Style 初始化中作为 fg/bg 使用时，产生空 SGR 字符串。
//  在 importFrame 中表示"跳过该平面"（不设 fg / 不设 bg）。
//  输出时终端渲染其默认颜色（或底层的目标内容）。

/**
 * @brief 透明色标记类型；配套常量 TRANS。
 */
struct Trans {};

/**
 * @brief 透明色常量，可直接作为 fg/bg 参数。
 */
inline constexpr Trans TRANS{};

//- 输入模块枚举
//  (vaterm::input / vaterm::InputData 使用；定义集中在此，别名见 vaterm/input.hpp)

/**
 * @brief InputData 的数据包类型，决定联合体中哪个负载成员有效。
 * @note lc3124(260923): NON 表示"空包"；一般用户用了非阻塞的输入获取，
 *        但缓冲区没有数据，或者阻塞模式的输入获取接口超时了，就会取得一个空包。
 */
enum class InputType : uint8_t {
    MOUSE = 0,  ///< 鼠标事件
    KEYBD = 1,  ///< 键盘事件
    NON   = 2   ///< 空包（无事件）
};

/**
 * @brief InputData 的鼠标按键。
 * @note lc3124(260923): 滚轮、松开用不到具体按键，归为 NONE。
 */
enum class MouseButton : uint8_t {
    NONE = 0,  ///< 无按键（滚轮、松开）
    LEFT,      ///< 左键
    MIDDLE,    ///< 中键
    RIGHT      ///< 右键
};

/**
 * @brief InputData 的鼠标动作。
 * @note lc3124(260923): 悬停不是 SGR 兼容的状态，但是毕竟有松开和按下的动作，
 *        我把无瞬间状态又不是拖拽或按住的 SGR 序列认为是鼠标在悬浮。
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
 * @brief InputData 的键盘特殊键码。
 * @note lc3124(260923): 特殊键无法用字符表示，用枚举区分；书面值可以用
 *        KeyPrintf 来编码。可打印键请用 KeybdData::cp，此时 code 保持 NONE。
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
 * @brief input 的模块工作状态。
 * @note lc3124(260923): 各取值含义：
 *          ON   (1) —— 鼠标键盘启动，模块工作中；
 *          MOUSE_DISABLE (2) —— 鼠标没有启动，执行 enableMouse 返回这个值
 *                说明终端不兼容鼠标；
 *          KEYBD_DISABLE (3) —— 鼠标启动了但键盘拿不到，出现这个值
 *                一定是终端兼容出了问题；
 *          INPUT_DISABLE (0) —— 键盘鼠标输入都无效；
 *          OFF  (4) —— 模块没有开启输入代理，输入由系统负责。
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
