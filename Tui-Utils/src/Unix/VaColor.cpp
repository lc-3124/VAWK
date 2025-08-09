/*
 * (C) Lc3124 2024 
 * LICENSE (MIT)
 * 这个文件用来实现VaTui::Color类
 *
 */

#ifndef _VACOLOR_CPP_
#define _VACOLOR_CPP_

//Va
#include "VaTui.hpp"
// std
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
// sys
#include <unistd.h>

/*
 * 简单封装一些东西，没必要过多说明
 */

void VaTui::Color::fastOutput(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

const char* VaTui::Color::_SetColor4bit(int front, int background)
{
    static char escapeCommand[64];
    snprintf(escapeCommand, sizeof(escapeCommand), "\033[%dm\033[%dm", front, background);
    return escapeCommand;
}

void VaTui::Color::SetColor4bit(int front, int background)
{
    //我不清楚静态成员怎么显式调用其他成员，都知道这个函数是VaTui::Color的就好,后面同理
    fastOutput(_SetColor4bit(front, background));
}

const char* VaTui::Color::_SetColor256(int front, int background)
{ 
    static char escapecommand[64];
    snprintf(escapecommand, sizeof(escapecommand),"\033[38;5;%dm\033[48;5;%dm",front,background);
    return escapecommand;
}

void VaTui::Color::SetColor256(int front, int background)
{
    fastOutput(_SetColor256(front, background));
}

const char* VaTui::Color:: _set_background_color_RGB(int R,int B,int G)
{
    static char escapecommand[64];
    snprintf(escapecommand, sizeof(escapecommand),"\033[48;2;%d,%d,%dm",R,G,B);
    return escapecommand;
}

void VaTui::Color::set_background_color_RGB(int R, int B,int G)
{
    fastOutput(_set_background_color_RGB(R, B, G));
}

const char* VaTui::Color::_set_front_color_RGB(int R,int B,int G)
{
    static char escapecommand[64];
    snprintf(escapecommand, sizeof(escapecommand),"\033[38;2;%d,%d,%dm",R,G,B);
    return escapecommand;
}

void VaTui::Color::set_front_color_RGB(int R, int B,int G)
{
    fastOutput(_set_front_color_RGB(R, G, B));
}

const char* VaTui::Color::_SetEffect(short effect,bool isEnable)
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

void VaTui::Color::SetEffect(short effect,bool isEnable)
{
    fastOutput(_SetEffect(effect, isEnable));
}

void VaTui::Color::ColorEffectReset()
{
    fastOutput(_ColorEffectReset());
}

const char* VaTui::Color::_ColorEffectReset()
{
    static char escapecommand[64];
    snprintf(escapecommand,sizeof(escapecommand),"\033[0m");
    return escapecommand;
}


/*
* 将给定的 RGB 颜色值（r、g、b，范围通常是0 - 255）转换为对应的 ANSI 256 色模式下的颜色代码,
* 我无法保证在实际使用中的效果，后续会进行测试验证。
* 首先计算颜色的灰度值，然后根据颜色是否为灰度（即 r、g、b 相等）以及灰度值范围，
* 然后来确定对应的 ANSI 256 色代码，返回该代码值。
*
* PS:
* 有几个函数不是我自己写的，所以不确定他们可不可以正常工作
* 但我会测试它们的
*/


/*
 * 预期范围内似乎没有问题，但是没有检查输入
 */
int VaTui::Color::RgbToAnsi256Color( int r,int g,int b )
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
void VaTui::Color::Ansi256ColorToRGB(int ansi256Color, int& r, int& g, int& b)
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
int VaTui::Color::MixAnsi256Colors( int color1,int color2 )
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
int VaTui::Color::AntiAnsi256Color(int colorcode)
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

/*
 * 下面这一段不是我写的，是deepseek生成的代码，
 * 我会测试他们的
 */
// 将 ANSI 16 色转换为 ANSI 256 色的函数
int VaTui::Color::Ansi16ColorToAnsi256(int ansi16Color)
{
    if (ansi16Color >= 0 && ansi16Color <= 7) {
        return ansi16Color + 16;  // 前8个基本颜色对应到 256 色中的前16个颜色里，索引偏移16
    } else if (ansi16Color >= 8 && ansi16Color <= 15) {
        return ansi16Color + 232 - 8;  // 后8个亮色对应到 256 色中的特定范围，进行相应索引转换
    }
    return 0;  // 如果传入的 16 色代码不符合规范，返回默认值（这里返回0，可根据实际情况调整）
}

// 将 ANSI 256 色转换为 ANSI 16 色的函数
int VaTui::Color::Ansi256ColorToAnsi16(int ansi256Color)
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


int VaTui::Color::Ansi256ColorToAnsi4bit(int ansi256Color, bool isFrontOrBack) {
    if (ansi256Color < 16) {
        // 如果已经是4bit颜色，直接返回
        return ansi256Color;
    }

    if (ansi256Color >= 232) {
        // 处理灰度颜色
        int gray = ansi256Color - 232;
        if (gray < 4) return isFrontOrBack ? 30 : 40; // 黑色
        if (gray < 8) return isFrontOrBack ? 90 : 100; // 亮黑色
        if (gray < 12) return isFrontOrBack ? 37 : 47; // 白色
        return isFrontOrBack ? 97 : 107; // 亮白色
    }

    // 处理彩色
    int colorIndex = ansi256Color - 16;
    int r = (colorIndex / 36) % 6;
    int g = (colorIndex / 6) % 6;
    int b = colorIndex % 6;

    // 将6级颜色映射到4级颜色
    r = r < 3 ? 0 : 1;
    g = g < 3 ? 0 : 1;
    b = b < 3 ? 0 : 1;

    int ansi4bitColor = (r << 2) | (g << 1) | b;

    // 映射到4bit颜色
    static const int ansi4bitMap[] = {
        0, 4, 2, 6, 1, 5, 3, 7
    };

    int result = ansi4bitMap[ansi4bitColor];

    // 如果是前景色，返回前景色代码，否则返回背景色代码
    return isFrontOrBack ? result + 30 : result + 40;
}

int VaTui::Color::Ansi4bitColorToAnsi16(int ansi4bitColor) {
    // 4bit颜色直接映射到16色
    if (ansi4bitColor < 8) {
        return ansi4bitColor;
    } else {
        return ansi4bitColor + 8;
    }
}
#endif 
