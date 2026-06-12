# VaTui — Terminal UI Toolkit for Linux

VaTui 是一个基于 ANSI 转义序列和 Linux 系统调用的 TUI（文本用户界面）工具库，提供终端颜色控制、光标操作、终端管理、系统信息获取和 UTF-8 处理等能力。

## 模块

| 模块 | 功能 |
|------|------|
| `VaTui::Color` | ANSI 颜色控制：4bit/256bit/RGB 颜色设置、文字特效、颜色空间转换与混合 |
| `VaTui::Cursor` | 光标控制：移动、定位、显示/隐藏、重置 |
| `VaTui::Term`  | 终端管理：属性设置、输入模式、屏幕清空、终端大小、非阻塞按键 |
| `VaTui::System`| 系统信息：用户名、主机名、环境变量、当前目录、系统命令输出 |
| `VaTui::Utf`   | UTF-8 处理：字符宽度检测、字节计数、字符串遍历与提取 |

## 快速开始

```cpp
#include "VaTui.hpp"

int main() {
    // 设置颜色并输出
    VaTui::Color::SetColor256(196, 16);  // 红色前景，黑色背景
    printf("Hello, TUI!\n");
    VaTui::Color::ColorEffectReset();

    // 获取终端大小
    int rows, cols;
    VaTui::Term::getTerminalSize(rows, cols);
    printf("Terminal size: %dx%d\n", rows, cols);

    // 获取用户名
    printf("User: %s\n", VaTui::System::getUserName().c_str());

    return 0;
}
```

编译：

```bash
g++ -std=c++11 -I. src/Unix/VaColor.cpp src/Unix/VaCusor.cpp \
    src/Unix/VaSystem.cpp src/Unix/VaTerm.cpp src/Unix/VaUtf.cpp \
    main.cpp -o demo
```

## 构建

```bash
make          # 编译所有模块和测试
make clean    # 清理构建产物
```

## 枚举定义

`VaTuiEnums.hpp` 提供了以下枚举类型：

- `CursorMovement` — 光标移动方向（`CUR_LEFT`, `CUR_RIGHT`, `CUR_UP`, `CUR_DOWN`）
- `CursorShape` — 光标形状（`CURSOR_BLOCK`, `CURSOR_UNDERLINE`, `CURSOR_VERTICAL_BAR`）
- `color4bit` — 4bit 颜色（前景/背景各 8 色）
- `color16` — 16 色模式颜色
- `AnsiEffect` — 文字特效（加粗、下划线、闪烁等）

## 注意事项

- 大部分功能依赖终端对 ANSI 转义序列的支持。不支持 ANSI 的终端可能出现乱码。
- 当前仅支持 Linux/Unix 平台（依赖 `termios`、`sys/ioctl.h` 等 POSIX API）。
- `VaTui::System::getSystemOuput()` 内部使用 `popen()`，避免传入不可信的命令字符串。

## 许可

MIT License. 详见 [LICENSE](./LICENSE)。

