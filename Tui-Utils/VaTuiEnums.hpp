#ifndef _VATUIENUMS_HPP_

#define _VATUIENUMS_HPP_

// VaCursor类中光标移动的枚举常量
enum CursorMovement{
    CUR_LEFT  = 0,
    CUR_RIGHT = 1,
    CUR_UP    = 2,
    CUR_DOWN  = 3,
};

// 用于定义光标形状的枚举
enum CursorShape {
    CURSOR_BLOCK = 0,
    CURSOR_UNDERLINE = 1,
    CURSOR_VERTICAL_BAR = 2
};

// 4位颜色模式下的前景色和背景色枚举定义
namespace color4bit {
    enum color_4bit {
        FRONT_BLACK = 30,
        FRONT_RED = 31,
        FRONT_GREEN = 32,
        FRONT_YELLOW = 33,
        FRONT_BLUE = 34,
        FRONT_PURPLE = 35,
        FRONT_DEEP_GREEN = 36,
        FRONT_WHITE = 37,
        BACKGROUND_BLACK = 40,
        BACKGROUND_RED = 41,
        BACKGROUND_GREEN = 42,
        BACKGROUND_YELLOW = 43,
        BACKGROUND_BLUE = 44,
        BACKGROUND_PURPLE = 45,
        BACKGROUND_DEEP_GREEN = 46,
        BACKGROUND_WHITE = 47
    };
}

// 16位颜色模式下的颜色枚举定义
enum color16 {
    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    PURPLE = 5,
    DEEP_GREEN = 6,
    WHITE = 7,
    BRIGHT_BLACK = 8,
    BRIGHT_RED = 9,
    BRIGHT_GREEN = 10,
    BRIGHT_YELLOW = 11,
    BRIGHT_BLUE = 12,
    BRIGHT_PURPLE = 13,
    BRIGHT_DEEP_GREEN = 14,
    BRIGHT_WHITE = 15
};

// 定义各种文本显示效果的枚举类型
enum AnsiEffect {
    BOLD = 1,
    DIM = 2,
    ITALIC = 3,
    UNDERLINE = 4,
    BLINK_SLOW = 5,
    BLINK_FAST = 6,
    REVERSE = 7,
    CONCEAL = 8,
    DELETE = 9
};

#endif
