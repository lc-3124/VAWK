#ifndef VATERM_HPP
#define VATERM_HPP

// -----------------------------------------------------------------------
//  vaterm — 面向 Linux 的头文件式终端 UI 工具库（C++23）
//
//  一组无状态、线程安全的工具，用于构建 TUI 应用。每个模块位于 vaterm/
//  下的独立头文件中，可单独包含。
//
//  模块：
//    vaterm::color    — 前景 / 背景色与文本效果的 ANSI 转义序列
//                       （4-bit、8-bit、24-bit）。
//    vaterm::cursor   — 光标移动、形状切换、保存/恢复、显示/隐藏的
//                       ANSI 转义序列。
//    vaterm::input    — 输入引擎，通过状态机接管终端键盘 / 鼠标，
//                       产出结构化的 InputData 事件。注意：已拆分为
//                       input.hpp + src/vaterm/input.cpp——唯一非
//                       header-only 的模块；使用它的程序必须链接
//                       build/bin.o/input.o。
//    vaterm::mouse    — SGR 编码的鼠标事件追踪：启用/禁用终端协议，
//                       把原始序列解析为结构化的 MouseState
//                       （按键、动作、位置）。
//    vaterm::terminal — RAII 终端 raw 模式、清屏序列、终端尺寸查询、
//                       阻塞 / 非阻塞 I/O。
//    vaterm::sys      — 系统信息辅助函数（用户名、主机名、环境变量、
//                       当前工作目录、通过 popen 执行命令）。
//    vaterm::utf      — UTF-8 字符串工具（首字节的字节长度、码点计数、
//                       按码点索引取子串、兼顾 CJK/emoji 的显示宽度）。
//
//  示例：
//    #include <vaterm.hpp>
//    #include <cstdio>
//
//    int main() {
//        printf("%sHello, TUI!%s\n",
//               vaterm::color::fg(vaterm::Color4::RED).c_str(),
//               vaterm::color::reset().c_str());
//        return 0;
//    }
// -----------------------------------------------------------------------

#include "vaterm/enums.hpp"
#include "vaterm/color.hpp"
#include "vaterm/cursor.hpp"
#include "vaterm/input.hpp"
#include "vaterm/term.hpp"
#include "vaterm/system.hpp"
#include "vaterm/utf.hpp"

#endif // VATERM_HPP
