<p align="center">
  <br>
  <img src="../logo.jpg" alt="vaterm" width="200">
  <br>
</p>

<h1 align="center">vaterm</h1>

<p align="center">
  <b>Terminal UI Toolkit for Linux</b><br>
  <i>基于 ANSI 转义序列和 Linux 系统调用的 TUI 工具库</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B">
  <img src="https://img.shields.io/badge/license-MIT-green">
  <img src="https://img.shields.io/badge/platform-Linux-red?logo=linux">
</p>

<p align="center">
  <a href="#-模块">模块</a> •
  <a href="#-快速开始">快速开始</a> •
  <a href="#-构建">构建</a> •
  <a href="#-枚举定义">枚举定义</a> •
  <a href="#-注意事项">注意事项</a> •
  <a href="#-许可">许可</a>
</p>

---

## 📦 模块

| 模块 | 命名空间 / 类 | 功能 |
|------|---------------|------|
| `enums.hpp` | `vaterm` | 枚举定义：`Color4`, `Color8`, `ColorDepth`, `Rgb`, `CursorDir`, `CursorShape`, `TextEffect` |
| `color.hpp` | `vaterm::color` | ANSI 颜色控制：4bit/8bit/24bit 颜色设置、文字特效、颜色空间转换与混合 |
| `cursor.hpp` | `vaterm::cursor` | 光标控制：移动、定位、显示/隐藏、形状设置 |
| `term.hpp` | `vaterm::terminal` | 终端管理：raw mode、屏幕清空、终端大小、I/O |
| `system.hpp` | `vaterm::sys` | 系统信息：用户名、主机名、环境变量、当前目录 |
| `utf.hpp` | `vaterm::utf` | UTF-8 处理：字符宽度、字节计数、字符串遍历 |
| `vatui.hpp` | `vatui::Framebuffer` + `vatui::VaTui` | 帧缓冲引擎（`Framebuffer`：双缓冲单元格矩阵、SGR 池差异渲染、宽字符保护）；单例 `VaTui`（`getInput`/`waitInput`/`enableMouse`） |

## 🚀 快速开始

```cpp
#include <cstdio>
#include <vaterm.hpp>

int main() {
    // 设置颜色并输出
    printf("%sHello, TUI!%s\n",
           vaterm::color::fg(vaterm::Color4::GREEN).c_str(),
           vaterm::color::reset().c_str());

    // 获取终端大小
    uint16_t rows, cols;
    vaterm::terminal::size(rows, cols);
    printf("Terminal: %dx%d\n", rows, cols);

    // 获取用户名
    printf("User: %s\n", vaterm::sys::user_name().c_str());

    return 0;
}
```

编译：

```bash
g++ --std=c++23 -Iinclude demo.cpp -o demo
```

## 🔨 构建

```bash
make          # 编译所有模块和测试
make check    # 验证头文件编译
make clean    # 清理构建产物
```

## 📋 枚举定义

### 颜色

```cpp
enum class Color4 : uint8_t {
    BLACK, RED, GREEN, YELLOW,
    BLUE, MAGENTA, CYAN, WHITE,
    BRIGHT_BLACK, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
    BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,
};

struct Rgb { uint8_t r, g, b; };
```

### 光标

```cpp
enum class CursorDir : uint8_t { UP, DOWN, LEFT, RIGHT };

enum class CursorShape : uint8_t {
    BLINKING_BLOCK     = 1,
    STEADY_BLOCK       = 2,
    BLINKING_UNDERLINE = 3,
    STEADY_UNDERLINE   = 4,
    BLINKING_BAR       = 5,
    STEADY_BAR         = 6,
};
```

### 文字特效

```cpp
enum class TextEffect : uint8_t {
    RESET, BOLD, DIM, ITALIC, UNDERLINE,
    SLOW_BLINK, RAPID_BLINK, REVERSE, CONCEAL, STRIKETHROUGH,
};
```

## ⚠️ 注意事项

- 大部分功能依赖终端对 ANSI 转义序列的支持。不支持 ANSI 的终端可能出现乱码。
- 当前仅支持 Linux/Unix 平台（依赖 `termios`、`sys/ioctl.h` 等 POSIX API）。
- `vaterm::sys::exec()` 内部使用 `popen()`，避免传入不可信的命令字符串。

## 📜 许可

MIT License. 详见 [LICENSE](./LICENSE)。

---

<p align="center">
  <sub>
    Part of <a href="https://github.com/lc-3124/VAWK">VAWK</a>
    · Built by <a href="https://github.com/lc-3124">lc3124</a>
    · Design assisted by <b>DeepSeek</b>
  </sub>
</p>

