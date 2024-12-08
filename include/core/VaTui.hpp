#pragma once
/*
 * (C) Lc3124 2024 
 * LICENSE (MIT)
 * 这段代码主要用于实现终端文本颜色及相关显示效果的控制功能，提供了多种颜色模式（如4位颜色、16位颜色、256色以及RGB颜色模式，尽管RGB模式可能因终端支持情况而异）的操作方法，同时也涵盖了颜色混合、颜色反转以及不同颜色模式之间的转换等功能，旨在方便在终端应用开发中实现丰富多样且灵活的文本颜色呈现效果。
 */


// std
#include <cmath>
#include <cstdio>
#include <cstring>
// sys
#include <unistd.h>

/*
 * Enums
 * 以下定义了不同的枚举类型
 */
namespace color4bit
{
    // 4位颜色模式下的前景色和背景色枚举定义。
    // 这些枚举值对应着 ANSI 转义序列中用于设置前景色和背景色的特定代码，可用于在终端上简单快速地设置文本颜色。
    enum color_4bit 
    {
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
};

// 16位颜色模式下的颜色枚举定义，涵盖了基本颜色以及对应的亮色版本，同样用于在相应的颜色设置函数中指定具体颜色。
enum color16
{    
    BLACK = 0,
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    PURPLE = 5,
    DEEP_GREEN = 6,
    WHITE = 7,
    BRIGHT_BLACK =8,
    BRIGHT_RED =9,
    BRIGHT_GREEN =10,
    BRIGHT_YELLOW =11,
    BRIGHT_BLUE =12,
    BRIGHT_PURPLE =13,
    BRIGHT_DEEP_GREEN =14,
    BRIGHT_WHITE =15
};

// 定义了各种文本显示效果的枚举类型，可用于控制文本的加粗、斜体、下划线、闪烁等特殊显示效果，通过相应的设置函数来启用或禁用这些效果。
enum class AnsiEffect {
    BOLD = 1,     // 加粗效果，使文本显示加粗样式。
    DIM = 2,      // 暗淡效果，让文本看起来颜色变浅、较暗淡（具体效果取决于终端实现）。
    ITALIC = 3,   // 斜体效果，使文本呈现斜体样式（部分终端可能不完全支持）。
    UNDERLINE = 4,// 下划线效果，在文本下方添加下划线。
    BLINK_SLOW = 5,  // 慢速闪烁效果，让文本以较慢频率闪烁（同样依赖终端支持情况）。
    BLINK_FAST = 6,  // 快速闪烁效果，文本闪烁频率比慢速闪烁更快。
    REVERSE = 7,  // 反转效果，用于交换文本的前景色和背景色，实现颜色反转显示。
    CONCEAL = 8,   // 隐藏效果，使文本在终端上不可见（可能有特定触发显示条件，具体依终端而定）。
    DELETE = 9    // 删除行效果，可能会清除文本所在行的内容（不同终端实现可能有差异）。
};

/*
 * VaColor class
 * VaColor 类是用于管理和操作终端文本颜色以及相关显示效果的核心类，
 * 它提供了一系列静态方法来方便地设置文本的颜色、效果等，并且包含了一些用于颜色转换、混合等实用功能的函数 
 * 旨在为终端界面开发中涉及颜色处理的场景提供一站式的解决方案。
 */
class VaColor
{
    private:
        // 一个私有静态函数，用于快速将给定的字符串输出到标准输出（STDOUT），
        // 通过系统函数 write 实现，主要在类内部其他函数中用于输出 ANSI 转义序列等控制字符来改变终端的显示效果。
        inline static void fastOutput(const char *str) {
            write(STDOUT_FILENO, str, strlen(str));
        }
    public:
        /*
         * Functions related to color control.
         * 以下是与颜色控制相关的一系列函数，用于设置文本的前景色、背景色，支持不同的颜色表示模式，方便根据实际需求灵活选择和应用颜色到终端文本显示上。
         */
        // Set the text and background's colors.
        // low bit
        // 生成用于设置4位颜色模式下文本前景色和背景色的 ANSI 转义序列字符串，并返回该字符串指针。
        // 此函数根据传入的前景色和背景色参数，按照 ANSI 转义序列的格式要求，
        // 使用 snprintf 函数将相应代码格式化为字符串存储在 escapeCommand 数组中，供后续输出使用。
        inline static const char* _SetColor4bit(int front, int background)
        {
            static char escapeCommand[64];
            snprintf(escapeCommand, sizeof(escapeCommand), "\033[%dm\033[%dm", front, background);
            return escapeCommand;
        }
        // 通过调用 fastOutput 函数输出由 _SetColor4bit 生成的 ANSI 转义序列，从而在终端上实际设置文本的前景色和背景色为指定的4位颜色模式颜色。
        inline static void SetColor4bit(int front, int background)
        {
            fastOutput(_SetColor4bit(front, background));
        }
        //16 bit
        // 生成用于设置16位颜色模式下文本前景色和背景色的 ANSI 转义序列字符串，返回该字符串指针。
        // 按照 ANSI 转义序列中针对16位颜色模式的特定格式，结合传入的前景色和背景色参数，
        // 使用 snprintf 函数构造相应字符串，存储在 escapecommand 数组中供后续输出到终端来改变颜色显示。
        inline static const char* _SetColor256(int front, int background)
        { 
            static char escapecommand[64];
            snprintf(escapecommand, sizeof(escapecommand),"\033[38;5;%dm\033[48;5;%dm",front,background);
            return escapecommand;
        }
        // 借助 fastOutput 函数输出 _SetColor256 生成的 ANSI 转义序列，实现将终端文本的前景色和背景色设置为指定的16位颜色模式颜色。
        inline static void SetColor256(int front, int background)
        {
            fastOutput(_SetColor256(front, background));
        }

        //full color
        // 生成用于设置全彩色（RGB模式）背景色的 ANSI 转义序列字符串，不过由于作者不确定其所在控制台是否支持 RGB 颜色，所以其实际可用性可能需进一步测试验证。
        // 根据传入的红（R）、绿（G）、蓝（B）三个颜色分量的值（通常范围是0 - 255），
        // 按照 ANSI 转义序列中 RGB 颜色设置的格式要求，
        // 使用 snprintf 函数构造相应字符串并存储在 escapecommand 数组中，返回该数组指针供后续输出操作。
        inline static const char* _set_background_color_RGB(int R,int B,int G)
        {
            static char escapecommand[64];
            snprintf(escapecommand, sizeof(escapecommand),"\033[48;2;%d,%d,%dm",R,G,B);
            return escapecommand;
        }
        // 通过 fastOutput 函数输出由 _set_background_color_RGB 生成的 ANSI 转义序列，尝试在终端上设置背景色为指定的 RGB 颜色，实际效果取决于终端对 RGB 颜色的支持情况。
        inline static void set_background_color_RGB(int R, int B,int G)
        {
            fastOutput(_set_background_color_RGB(R, B, G));
        }

        // 生成用于设置全彩色（RGB模式）前景色的 ANSI 转义序列字符串，同样存在因终端支持情况不确定而需测试其实际可用性的问题。
        // 依据传入的红（R）、绿（G）、蓝（B）三个颜色分量的值，
        // 按照相应的 ANSI 转义序列格式，
        // 使用 snprintf 函数构建字符串并存入 escapecommand 数组，返回该数组指针供后续输出以改变前景色显示。
        inline static const char* _set_front_color_RGB(int R,int B,int G)
        {
            static char escapecommand[64];
            snprintf(escapecommand, sizeof(escapecommand),"\033[38;2;%d,%d,%dm",R,G,B);
            return escapecommand;
        }
        // 利用 fastOutput 函数输出由 _set_front_color_RGB 生成的 ANSI 转义序列，
        // 在终端上设置文本的前景色为指定的 RGB 颜色，其效果依赖终端对 RGB 颜色的支持与否。
        inline static void set_front_color_RGB(int R, int B,int G)
        {
            fastOutput(_set_front_color_RGB(R, G, B));
        }

        /*
         * Functions related to effect cotrol.
         * 以下是与文本显示效果控制相关的函数，用于设置文本的各种特殊显示效果，如加粗、斜体、闪烁等，可根据需要启用或禁用这些效果，增强终端文本显示的丰富性和交互性。
         */
        //set the effect of text 
        // 根据传入的文本显示效果枚举值（effect）以及是否启用该效果的布尔值（isEnable），生成对应的 ANSI 转义序列字符串，返回该字符串指针。
        // 如果 isEnable 为 true，则按照启用效果的 ANSI 转义序列格式，
        // 使用 snprintf 函数构造相应字符串；若为 false，则按照禁用效果的格式构造字符串，存储在 escapecommand 数组中供后续输出使用。
        inline static const char* _SetEffect(short effect,bool isEnable)
        {
            static char escapecommand[64];
            if (isEnable == true){
                snprintf(escapecommand,sizeof(escapecommand),"\033[%dm",effect);
            }
            else {
                snprintf(escapecommand,sizeof(escapecommand),"\033[2%dm",effect);
            }

            return escapecommand;
        }
        // 通过 fastOutput 函数输出由 _SetEffect 生成的 ANSI 转义序列，从而在终端上实际设置文本的显示效果为指定的效果（启用或禁用）。
        inline void SetEffect(short effect,bool isEnable)
        {
            fastOutput(_SetEffect(effect, isEnable));
        }
        // 生成用于重置文本颜色和所有显示效果的 ANSI 转义序列字符串，返回该字符串指针，以便后续可通过输出此序列将文本显示恢复到默认状态（无特殊颜色和效果）。
        inline static const char* _ColorEffectReset()
        {
            static char escapecommand[64];
            snprintf(escapecommand,sizeof(escapecommand),"\033[0m");
            return escapecommand;
        }

        /*
         * Other functions 
         * 以下是一些其他的实用颜色处理相关函数，包括颜色模式之间的转换、颜色混合以及颜色反转等功能，
         * 为更复杂的颜色操作需求提供支持，方便在不同颜色应用场景中进行灵活的颜色调整和处理。
         */


        // 将给定的 RGB 颜色值（r、g、b，范围通常是0 - 255）转换为对应的 ANSI 256 色模式下的颜色代码,
        // 不确定此函数在实际使用中的效果，后续会进行测试验证。
        // 首先计算颜色的灰度值，然后根据颜色是否为灰度（即 r、g、b 相等）以及灰度值范围，
        // 按照特定的算法来确定对应的 ANSI 256 色代码，返回该代码值。
        int RgbToAnsi256Color( int r,int g,int b )
        {
            int gray = 0.299 * r + 0.587 * g + 0.114 * b;
            if (r == g && g == b) {
                if (gray < 8) return 16;
                if (gray > 248) return 231;
                return std::round((gray - 8) / 247.0 * 24) + 232;
            } else {
                int ansiR = std::round((double)r / 51.0);
                int ansiG = std::round((double)g / 51.0);
                int ansiB = std::round((double)b / 51.0);
                return 16 + (ansiR * 36) + (ansiG * 6) + ansiB;
            }
        }

        /*这个函数不是我写的，我也不知道这玩意能不能正常工作，我会测试它的*/
        //将Ansi的256色转换成RGB颜色的三个分量( r , g , b ) 
        void Ansi256ColorToRGB(int ansi256Color, int& r, int& g, int& b)
        {
            if (ansi256Color >= 0 && ansi256Color <= 15) {
                // 处理前16个基本颜色（0 - 15），对应特定的固定RGB值
                switch (ansi256Color) {
                    case 0:  // 黑色
                        r = 0;
                        g = 0;
                        b = 0;
                        break;
                    case 1:  // 红色
                        r = 170;
                        g = 0;
                        b = 0;
                        break;
                    case 2:  // 绿色
                        r = 0;
                        g = 170;
                        b = 0;
                        break;
                    case 3:  // 黄色
                        r = 170;
                        g = 170;
                        b = 0;
                        break;
                    case 4:  // 蓝色
                        r = 0;
                        g = 0;
                        b = 170;
                        break;
                    case 5:  // 紫色
                        r = 170;
                        g = 0;
                        b = 170;
                        break;
                    case 6:  // 深绿色（青色）
                        r = 0;
                        g = 170;
                        b = 170;
                        break;
                    case 7:  // 白色
                        r = 170;
                        g = 170;
                        b = 170;
                        break;
                    case 8:  // 亮黑色（灰色）
                        r = 85;
                        g = 85;
                        b = 85;
                        break;
                    case 9:  // 亮红色
                        r = 255;
                        g = 85;
                        b = 85;
                        break;
                    case 10:  // 亮绿色
                        r = 85;
                        g = 255;
                        b = 85;
                        break;
                    case 11:  // 亮黄色
                        r = 255;
                        g = 255;
                        b = 85;
                        break;
                    case 12:  // 亮蓝色
                        r = 85;
                        g = 85;
                        b = 255;
                        break;
                    case 13:  // 亮紫色
                        r = 255;
                        g = 85;
                        b = 255;
                        break;
                    case 14:  // 亮深绿色（亮青色）
                        r = 85;
                        g = 255;
                        b = 255;
                        break;
                    case 15:  // 亮白色
                        r = 255;
                        g = 255;
                        b = 255;
                        break;
                }
            } else if (ansi256Color >= 16 && ansi256Color <= 231) {
                // 处理 16 - 231 的颜色，通过特定算法从颜色代码计算RGB分量
                int index = ansi256Color - 16;
                r = (index / 36) * 51;
                index %= 36;
                g = (index / 6) * 51;
                b = (index % 6) * 51;
            } else if (ansi256Color >= 232 && ansi256Color <= 255) {
                // 处理 232 - 255 的灰度颜色，根据代码计算灰度对应的RGB值
                int gray = 8 + (ansi256Color - 232) * 10;
                r = gray;
                g = gray;
                b = gray;
            }
        } 
        
        //混合两个Ansi256色，返回混合的结果
        int MixAnsi256Colors( int color1,int color2 )
        {
            int r1, g1, b1, r2, g2, b2;
            if (color1 >= 16 && color1 <= 231) {
                int index = color1 - 16;
                r1 = index / 36;
                index %= 36;
                g1 = index / 6;
                b1 = index % 6;
                r1 *= 51;
                g1 *= 51;
                b1 *= 51;
            } else if (color1 >= 232 && color1 <= 255) {
                int gray = ((color1 - 232) * 247 / 24) + 8;
                r1 = g1 = b1 = gray;
            }

            if (color2 >= 16 && color2 <= 231) {
                int index = color2 - 16;
                r2 = index / 36;
                index %= 36;
                g2 = index / 6;
                b2 = index % 6;
                r2 *= 51;
                g2 *= 51;
                b2 *= 51;
            } else if (color2 >= 232 && color2 <= 255) {
                int gray = ((color2 - 232) * 247 / 24) + 8;
                r2 = g2 = b2 = gray;
            }

            int r = (r1 + r2) / 2;
            int g = (g1 + g2) / 2;
            int b = (b1 + b2) / 2;

            return RgbToAnsi256Color(r, g, b);
        }

        //将Ansi256色色号反色处理，返回反色结果
        int AntiAnsi256Color(int colorcode)
        {
            int color1 =colorcode;
            int r1, g1, b1;
            if (color1 >= 16 && color1 <= 231) {
                int index = color1 - 16;
                r1 = index / 36;
                index %= 36;
                g1 = index / 6;
                b1 = index % 6;
                r1 *= 51;
                g1 *= 51;
                b1 *= 51;
            } else if (color1 >= 232 && color1 <= 255) {
                int gray = ((color1 - 232) * 247 / 24) + 8;
                r1 = g1 = b1 = gray;
            }
            int r = 250 - r1;
            int g = 250 - g1;
            int b = 250 - b1;

            return RgbToAnsi256Color(r, g, b);
        }

        // 将 ANSI 16 色转换为 ANSI 256 色的函数
        int Ansi16ColorToAnsi256(int ansi16Color)
        {
            if (ansi16Color >= 0 && ansi16Color <= 7) {
                return ansi16Color + 16;  // 前8个基本颜色对应到 256 色中的前16个颜色里，索引偏移16
            } else if (ansi16Color >= 8 && ansi16Color <= 15) {
                return ansi16Color + 232 - 8;  // 后8个亮色对应到 256 色中的特定范围，进行相应索引转换
            }
            return 0;  // 如果传入的 16 色代码不符合规范，返回默认值（这里返回0，可根据实际情况调整）
        }

        // 将 ANSI 256 色转换为 ANSI 16 色的函数
        int Ansi256ColorToAnsi16(int ansi256Color)
        {
            if (ansi256Color >= 16 && ansi256Color <= 231) {
                // 256 色中的前 216 种颜色转换到 16 色中的前 8 种基本色的逻辑
                int index = ansi256Color - 16;
                int r = index / 36;
                index %= 36;
                int g = index / 6;
                int b = index % 6;
                if (r == g && g == b) {
                    return r;
                }
                return 8;  // 如果不是单一颜色，对应到 16 色中的亮黑色（这里简单对应，可根据需求调整更合适的逻辑）
            } else if (ansi256Color >= 232 && ansi256Color <= 255) {
                // 256 色中的灰度范围颜色对应到 16 色中的后 8 种亮色的逻辑
                return (ansi256Color - 232 + 8);
            }
            return 0;  // 如果传入的 256 色代码不符合规范，返回默认值（这里返回0，可根据实际情况调整）
        }
};
/*
 * copyright lc-3124 release 2024 
 * LICENSE (MIT)
 * 本代码片段主要实现了与终端光标操作相关的一系列功能，包括将光标移动到指定位置、按指定方向和距离移动光标、重置光标到默认位置、隐藏和显示光标等操作，旨在方便在终端应用开发中对光标进行灵活控制，以实现更丰富的用户界面交互效果。
 */

// std
#include <cstdio>
#include <cstring>
// sys
#include <unistd.h>

/*
 * Enums
 * 定义了用于表示光标移动方向的枚举类型，通过这些枚举值可以清晰直观地指定光标在终端屏幕上的移动方向，便于后续光标移动相关函数的参数传递和逻辑处理。
 */
enum
{
    CUR_LEFT = 0,    // 表示光标向左移动的方向枚举值。
    CUR_RIGHT = 1,   // 表示光标向右移动的方向枚举值。
    CUR_UP = 3,       // 表示光标向上移动的方向枚举值。
    CUR_DOWN = 4      // 表示光标向下移动的方向枚举值。
};


/*
 * Functions related to cursor actions.
 * VaCursor 类封装了一系列与终端光标操作相关的函数，通过生成和输出 ANSI 转义序列来控制终端光标的位置、显示状态等，为在终端应用中实现自定义的光标行为提供了便捷的方式。
 */
class VaCursor
{
    private:
       // 一个私有静态函数，用于快速将给定的字符串输出到标准输出（STDOUT），借助系统函数 write 实现，主要在类内部其他函数中用于输出 ANSI 转义序列，从而改变终端上光标的相关状态（如位置、显示与否等）。
       inline static void fastOutput(const char *str) {
            write(STDOUT_FILENO, str, strlen(str));
        }
    public:
        /*
         * Functions related to cursor actions.
         * 以下是与光标操作相关的一系列公共函数，提供了不同类型的光标操作功能，例如移动到指定位置、按方向移动以及控制光标显示隐藏等，方便开发者根据具体需求灵活操控终端光标。
         */
        // Move the cursor to the specified position (h, w).
        // 生成用于将光标移动到指定行（h）和列（w）位置的 ANSI 转义序列字符串，并返回该字符串指针。
        // 根据 ANSI 转义序列中设置光标位置的格式要求，使用 snprintf 函数将行和列参数格式化为相应字符串，存储在 escapeCommand 数组中，供后续输出操作使用，以实现将光标定位到指定坐标位置。
        inline static const char* _CursorMoveTo(int h, int w)
        {
            static char escapeCommand[64];
            snprintf(escapeCommand, sizeof(escapeCommand), "\033[%d;%dH", h, w);
            return escapeCommand;
        }
        // 通过调用 fastOutput 函数输出由 _CursorMoveTo 生成的 ANSI 转义序列，从而在终端上实际将光标移动到指定的（h, w）位置。
        inline static void CursorMoveTo(int h, int w)
        {
          fastOutput(_CursorMoveTo(h, w));
        }

        // Move the cursor in a given direction by a specified distance.
        // 根据给定的光标移动方向（dr，通过之前定义的枚举值表示）和移动距离（ds），生成相应的 ANSI 转义序列字符串，返回该字符串指针，用于控制光标按指定方向移动指定的距离。
        // 通过 switch 语句根据不同的移动方向枚举值，按照 ANSI 转义序列中对应方向移动的格式要求，使用 snprintf 函数构造相应字符串存储在 escapeCommand 数组中，供后续输出操作来实现光标移动。例如，对于向左移动（CUR_LEFT），按照向左移动的格式生成相应的转义序列字符串。
        inline static const char* _CursorMove(int dr, int ds)
        {
            static char escapeCommand[64];
            switch (dr)
            {
                case CUR_LEFT:
                    {
                        // 生成向左移动指定距离（ds）的 ANSI 转义序列字符串，格式为 "\033[%dD"，其中 %d 会被实际移动距离替换。
                        snprintf(escapeCommand, sizeof(escapeCommand), "\033[%dD", ds);
                        return escapeCommand;
                    }
                case CUR_RIGHT:
                    {
                        // 生成向右移动指定距离（ds）的 ANSI 转义序列字符串，格式为 "\033[%dC"。
                        snprintf(escapeCommand, sizeof(escapeCommand), "\033[%dC", ds);
                        return escapeCommand;
                    }
                case CUR_UP:
                    {
                        // 生成向上移动指定距离（ds）的 ANSI 转义序列字符串，格式为 "\033[%dA"。
                        snprintf(escapeCommand, sizeof(escapeCommand), "\033[%dA", ds);
                        return escapeCommand;
                    }
                case CUR_DOWN:
                    {
                        // 生成向下移动指定距离（ds）的 ANSI 转义序列字符串，格式为 "\033[%dB"。
                        snprintf(escapeCommand, sizeof(escapeCommand), "\033[%dB", ds);
                        return escapeCommand;
                    }
                default:
                    // 如果传入的移动方向枚举值不合法（不在预定义的方向枚举范围内），则返回 nullptr，表示无法生成有效的移动转义序列。
                    return nullptr;
            }
        }
        // 通过调用 fastOutput 函数输出由 _CursorMove 生成的 ANSI 转义序列，从而在终端上实际控制光标按指定方向移动指定的距离。
        inline static void CursorMove(int dr, int ds)
        {
          fastOutput(_CursorMove(dr, ds));
        }

        // Reset the cursor to its default position.
        // 返回用于将光标重置到默认位置（通常是终端屏幕左上角，即第一行第一列）的 ANSI 转义序列字符串。
        // 该转义序列固定为 "\033[H"，直接返回此字符串指针，供后续输出操作来实现光标位置的重置。
        inline static const char* _CursorReset()
        {
            return "\033[H";
        }
        // 通过调用 fastOutput 函数输出由 _CursorReset 提供的 ANSI 转义序列，从而在终端上实际将光标重置到默认位置。
        inline static void CursorReset()
        {
          fastOutput("\033[H");
        }

        // Hide the cursor.
        // 返回用于隐藏光标的 ANSI 转义序列字符串，该字符串固定为 "\033[?25l"，直接返回此字符串指针，供后续输出操作来实现隐藏光标功能，常用于一些不需要光标显示干扰的场景，比如全屏展示内容时。
        inline static const char* _CursorHide()
        {
            return "\033[?25l";
        }
        // 通过调用 fastOutput 函数输出由 _CursorHide 提供的 ANSI 转义序列，从而在终端上实际隐藏光标。
        inline static void CursorHide()
        {
          fastOutput("\033[?25l");
        }

        // Show the cursor.
        // 返回用于显示光标的 ANSI 转义序列字符串，其固定为 "\033[?25h"，直接返回此字符串指针，供后续输出操作来实现显示光标功能，可在之前隐藏光标的场景结束后，重新显示光标以便正常交互操作。
        inline static const char* _CursorShow()
        {
            return "\033[?25h";
        }
        // 通过调用 fastOutput 函数输出由 _CursorShow 提供的 ANSI 转义序列，从而在终端上实际显示光标。
        inline static void CursorShow()
        {
          fastOutput("\033[?25h");
        }


};

/*
 * (C) Lc3124 
 * release in  2024 
 * LICENSE (MIT)
 * 这个文件包含一个类，用来获取各种系统信息
 */

#include <iostream>
#include <ctime>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <cstdlib>

class VaSystem {
    public:
        // 获取用户名
        static std::string getUserName() {
            uid_t uid = getuid();
            struct passwd *pw = getpwuid(uid);
            if (pw!= nullptr) {
                return std::string(pw->pw_name);
            }
            return "";

        }

        // 获取当前时间
        static std::string getCurrentTime() {
            time_t now = time(nullptr);
            char buffer[80];
            struct tm *timeinfo = localtime(&now);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            return std::string(buffer);

        }

        //获取环境变量
        static std::string getRunningEnvironment(const char *index) {
            const char *env = std::getenv("PATH");
            std::string result = "";
            if (env!= nullptr) {
                result += env;
            }
            return result;

        }

        // 获取设备名称（这里通过utsname结构体获取系统信息中的机器名部分来近似表示设备名称）
        static std::string getDeviceName() {
            struct utsname uts;
            if (uname(&uts)!= -1) {
                return std::string(uts.machine);
            }
            return "";

        }

        // 获取主机名
        static std::string getHostName() {
            char hostname[256];
            if (gethostname(hostname, sizeof(hostname))!= -1) {
                return std::string(hostname);
            }
            return "";
        }

        // 获取运行目录
        static std::string getRunningDirectory() {
            char buffer[256];
            if (getcwd(buffer, sizeof(buffer))!= nullptr) {
                return std::string(buffer);
            }
            return "";

        }

};
#pragma once
/*
 * copyright lc-3124 release 2024 
 * LICENSE (MIT)
 */

// std
#include <cstdio>
#include <cstring>
#include <iostream>
// sys
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// 定义不同的光标形状常量
enum CursorShape {
    CURSOR_BLOCK = 0,
    CURSOR_UNDERLINE = 1,
    CURSOR_VERTICAL_BAR = 2

};

class VaTerm
{
    private:
        termios originalAttrs;
        termios currentAttrs;
    public:
        VaTerm() {
            tcgetattr(STDIN_FILENO, &originalAttrs);
            currentAttrs = originalAttrs;
        }

        //释放时会恢复终端设置
        ~VaTerm() {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalAttrs);
        }

        /*
         * 以下的各功能几乎只是对termios等库的封装
         */

        // Clear the entire screen.
        inline static const char* _Clear()
        {
            return "\033[2J";
        }
        inline static void Clear()
        {
            fastOutput("\033[2J");
        }

        // Clear the area from the cursor's position to the end of the line.
        inline static const char* _ClearLine()
        {
            return "\033[K";
        }
        inline static void ClearLine()
        {
            fastOutput("\033[K");
        }
        void getTerminalAttributes() {
            tcgetattr(STDIN_FILENO, &currentAttrs);
        }

        void setTerminalAttributes(const termios &newAttrs) {
            currentAttrs = newAttrs;
            tcsetattr(STDIN_FILENO, TCSANOW, &currentAttrs);
        }

        void enableEcho() {
            currentAttrs.c_lflag |= ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &currentAttrs);
        }

        void disableEcho() {
            currentAttrs.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &currentAttrs);
        }

        void enableConsoleBuffering() {
            int flags = fcntl(STDIN_FILENO, F_GETFL);
            fcntl(STDIN_FILENO, F_SETFL, flags & ~O_SYNC);
        }

        void disableConsoleBuffering() {
            int flags = fcntl(STDIN_FILENO, F_GETFL);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_SYNC);
        }

        void getTerminalSize(int &rows, int &cols) {
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            rows = w.ws_row;
            cols = w.ws_col;
        }

        void setCursorPosition(int row, int col) {
            std::cout << "\033[" << row << ";" << col << "H";
        }

        void saveCursorPosition() { std::cout << "\033[s"; }

        void restoreCursorPosition() { std::cout << "\033[u"; }

        inline static void fastOutput(const char *str) {
            write(STDOUT_FILENO, str, strlen(str));
        }

        char nonBufferedGetKey() {
            struct termios oldt, newt;
            char c;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            read(STDIN_FILENO, &c, 1);
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return c;
        }

        const char *getTerminalType() { return std::getenv("TERM"); }


        void setLineBuffering(bool enable) {
            if (enable) {
                currentAttrs.c_lflag |= ICANON;
            } else {
                currentAttrs.c_lflag &= ~ICANON;
            }
            tcsetattr(STDIN_FILENO, TCSANOW, &currentAttrs);
        }
        
        //禁止回显，然后阻塞，返回获取到的字符，类似于getch()
        char getCharacter() {
            disableEcho();
            char c = nonBufferedGetKey();
            enableEcho();
            return c;
        }
        
        //判断终端是否支持某一功能 
            bool isTerminalFeatureSupported(const char *feature) {
                const char *termType = getTerminalType();
                if (termType == nullptr) {
                    return false;

                }
                    if (strstr(termType, feature)!= nullptr) {
                        return true;
                    }
                return false;
            }

        // 设置字符输入延迟
        void setCharacterDelay(int milliseconds) {
            termios newAttrs = currentAttrs;
                newAttrs.c_cc[VMIN] = 0;
            newAttrs.c_cc[VTIME] = milliseconds / 100;
            setTerminalAttributes(newAttrs);

        }

        // 获取输入速度
        int getInputSpeed() {
            speed_t speed;
            tcgetattr(STDIN_FILENO, &currentAttrs);
            speed = cfgetospeed(&currentAttrs);
            return static_cast<int>(speed);

        }

        // 设置输入速度
        void setInputSpeed(int speed) {
            termios newAttrs = currentAttrs;
            cfsetospeed(&newAttrs, static_cast<speed_t>(speed));
            cfsetispeed(&newAttrs, static_cast<speed_t>(speed));
            setTerminalAttributes(newAttrs);

        }

        // 设置输出速度
        void setOutputSpeed(int speed) {
            termios newAttrs = currentAttrs;
            cfsetospeed(&newAttrs, static_cast<speed_t>(speed));
            setTerminalAttributes(newAttrs);

        }

        int keyPressed(char &k) {
            struct termios oldt, newt;
            int oldf;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            oldf = fcntl(STDIN_FILENO, F_GETFL);
            fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

            char c;
            int res = read(STDIN_FILENO, &c, 1);
            if (res > 0) {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                fcntl(STDIN_FILENO, F_SETFL, oldf);
                k=c;
                return 1;
            } else {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                fcntl(STDIN_FILENO, F_SETFL, oldf);
                k=static_cast<char>(-1);
                return -0;
            }
        }

        // 函数用于设置光标形状
        void setCursorShape(CursorShape shape) {
            termios newAttrs = currentAttrs;

            // 根据传入的光标形状设置相应的c_cflag值
            switch (shape) {
                case CURSOR_BLOCK:
                    newAttrs.c_cflag &= ~(ECHOCTL);
                    break;
                case CURSOR_UNDERLINE:
                    newAttrs.c_cflag |= (ECHOCTL | ECHOE);
                    break;
                case CURSOR_VERTICAL_BAR:
                    newAttrs.c_cflag |= ECHOCTL;
                    break;
            }
            // 设置新的终端属性
            setTerminalAttributes(newAttrs);
        }
};
#pragma once
/*
 * (C) Lc3124 
 * release in 2024 
 * LICENSE (MIT)
 * 这个文件包含一个类，用来识别Utf字节，当然还有一些其他的功能，也会在之后加入新的功能
 */

#include <locale>

class VaUtf
{

//get the width of a UTF character 
size_t getUtf8CharWidth(const char* s) {
    if (!s ||!*s) return 0;
    unsigned char c = static_cast<unsigned char>(*s);
    if (c < 0x80) return 1;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    else return 0;

}

// recognize a ASCII character 
bool isAscii(char c) {
    return (c & 0x80) == 0;

}

// 判断是否为 UTF-8 多字节序列的开头字节
bool isUtf8StartByte(char c) {
    return (c & 0xC0)!= 0x80 && (c & 0xF0)!= 0xF8 && (c & 0xFE)!= 0xFC;

}

// 判断是否为 UTF-8 编码字符
bool isUtf8Char(const char* bytes, int len) {
    if (len <= 0) return false;
    char firstByte = bytes[0];
    if (isAscii(firstByte)) return true;
    if (isUtf8StartByte(firstByte)) {
	int numBytes;
	if ((firstByte & 0xE0) == 0xC0) numBytes = 2;
	else if ((firstByte & 0xF0) == 0xE0) numBytes = 3;
	else if ((firstByte & 0xF8) == 0xF0) numBytes = 4;
	else return false;
	return len == numBytes;

    }
    return false;

}

// 判断是否为 GBK 编码字符（简单判断，假设连续两个字节都在 GBK 编码范围内）
bool isGbkChar(const char* bytes, int len) {
    if (len!= 2) return false;
    unsigned char firstByte = static_cast<unsigned char>(bytes[0]);
    unsigned char secondByte = static_cast<unsigned char>(bytes[1]);
    return ((firstByte >= 0x81 && firstByte <= 0xFE) && (secondByte >= 0x40 && secondByte <= 0xFE && secondByte!= 0x7F));

}


};
#pragma once

/* (C) Lc3124 2024
 * LICENSE (MIT)
 *
 * The functions implemented in this file are the basis for VAWK to perform synthesis,
 * plotting, invoking the system, obtaining system and environment information, 
 * and other underlying functions.
 *
 * This file contains the entire functionality of the other modules of the entire Va-untils,
 * but with a lot of modifications on top of that.
 * I consider this file to be a refactoring of Va-unitls 
 */


class VaTui 
{
    public:
        /*there are the submodels*/
        VaCursor Cursor;
        VaColor Color;
        VaTerm Term;
        VaSystem System;
        VaUtf Utf;
    
    private:
    public:

};
