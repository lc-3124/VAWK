
/*
 * (c) 2024 Lc3124 
 * License (MIT)
 *
 * 本代码片段主要实现了与终端光标操作相关的一系列功能，
 * 包括将光标移动到指定位置、按指定方向和距离移动光标、
 * 重置光标到默认位置、隐藏和显示光标等操作，
 * 旨在方便在终端应用开发中对光标进行灵活控制，以实现更丰富的用户界面交互效果。
 */

#ifndef _VACUSOR_CPP_
#define _VACUSOR_CPP_

//Va
#include "VaTuiEnums.hpp"
#include "VaTui.hpp"
// std
#include <cstdio>
#include <cstring>
// sys
#include <unistd.h>


void VaTui::Cursor::fastOutput(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}
const char* VaTui::Cursor::_CursorMoveTo(int h, int w)
{
    static char escapeCommand[64];
    snprintf(escapeCommand, sizeof(escapeCommand), "\033[%d;%dH", h, w);
    return escapeCommand;
}
void VaTui::Cursor::CursorMoveTo(int h, int w)
{
    fastOutput(_CursorMoveTo(h, w));
}
const char* VaTui::Cursor::_CursorMove(int dr, int ds)
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
void VaTui::Cursor::CursorMove(int dr, int ds)
{
    fastOutput(_CursorMove(dr, ds));
}

const char*VaTui::Cursor:: _CursorReset()
{
    return "\033[H";
}
void VaTui::Cursor::CursorReset()
{
    fastOutput("\033[H");
}

const char* VaTui::Cursor::_CursorHide()
{
    return "\033[?25l";
}
// 通过调用 fastOutput 函数输出由 _CursorHide 提供的 ANSI 转义序列，从而在终端上实际隐藏光标。
void VaTui::Cursor::CursorHide()
{
    fastOutput("\033[?25l");
}

// Show the cursor.
// 返回用于显示光标的 ANSI 转义序列字符串，其固定为 "\033[?25h"，直接返回此字符串指针，供后续输出操作来实现显示光标功能，可在之前隐藏光标的场景结束后，重新显示光标以便正常交互操作。
const char* VaTui::Cursor::_CursorShow()
{
    return "\033[?25h";
}
// 通过调用 fastOutput 函数输出由 _CursorShow 提供的 ANSI 转义序列，从而在终端上实际显示光标。
void VaTui::Cursor::CursorShow()
{
    fastOutput("\033[?25h");
}

#endif
