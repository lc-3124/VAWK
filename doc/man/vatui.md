# vatui 用户手册

> vatui 是构建在 vaterm 之上的帧缓冲与输入封装库。
> 提供双缓冲单元格矩阵（1:1 映射到终端屏幕），
> 支持 4-bit / 8-bit / 24-bit 三种颜色深度自动适配，
> 以及统一的鼠标/键盘输入。

---

## 快速开始

```cpp
#include <vatui.hpp>

using namespace vatui;

int main() {
    auto& tui = VaTui::instance();
    tui.init();

    auto& fb = tui.buffer();
    fb.printText({.col = 0, .row = 0, .text = "Hello, 世界！",
                  .style = {.fg = Color4::BRIGHT_GREEN, .bg = Color4::BLUE}});
    fb.swap();

    tui.waitInput();
    // 析构自动恢复终端
}
```

编译：
```bash
make lib                              # 编译 vatui.o
g++ --std=c++23 -Iinclude -Itui-utils/include \
    myapp.cpp build/bin.o/vatui.o -o myapp
```

---

## Framebuffer — 帧缓冲

### 生命周期

- `VaTui::instance()` 返回 Meyer's 单例。首次调用时构造函数自动检测终端大小。
- `init()` 进入 raw mode、隐藏光标、清屏。可多次调用（幂等）。
- 析构函数在程序退出时自动显示光标、清屏、恢复 termios。

### 缓冲区控制

```cpp
void setSize(SizeArgs args);   // 重置缓冲区大小（自动清空）
int  getColMax() const;        // 缓冲区宽度
int  getRowMax() const;        // 缓冲区高度
void setPosition(PositionArgs args); // 渲染偏移（相对终端左上角）
int  getOffsetCol() const;
int  getOffsetRow() const;
```

### 绘制

```cpp
void printText(TextArgs args);   // 写入 UTF-8 文本（支持 CJK 宽字符）
void clear();                    // 清空整个缓冲区
void clearRegion(RegionArgs args);
void fillRegion(FillArgs args);  // 填充矩形区域
```

所有绘制操作使用形参结构体，支持 designated-initialiser 语法：

```cpp
fb.printText({.col = 0, .row = 0, .text = "你好",
              .style = {.fg = Rgb{255, 100, 50}, .bg = Color4::BLACK}});
```

### 输出

```cpp
void swap();
```

差异渲染——只输出自上次 `swap()` 以来发生变化的单元格。
相邻同风格的单元格合并为一次 SGR 序列，减少输出量。

### 宽字符保护

当新文本覆盖到宽字符的尾部（`isLong=true`）或头部时，
自动清理对应的另一半，并用**原始颜色**的空格填充碎片区域。
右边界放不下宽字符时，写入带当前风格的空格并继续。

---

## 颜色系统

vatui 支持三种颜色深度，可在 `Style` 中混用：

| 类型 | 示例 | 说明 |
|------|------|------|
| `Color4` | `Color4::BRIGHT_RED` | 16 色 ANSI 调色板 |
| `Color8` | `Color8{196}` | 256 色调色板索引 (0-255) |
| `Rgb` | `Rgb{255, 100, 50}` | 24-bit 真彩色 |

```cpp
// 三种风格等价使用
Style{.fg = Color4::GREEN}              // 4-bit
Style{.fg = Color8{46}}                 // 8-bit
Style{.fg = Rgb{0, 255, 0}}            // 24-bit
```

### 自动深度适配

`swap()` 时自动检测终端能力，把颜色转换到终端原生深度：

| 终端能力 | 下行转换 | 上行转换 |
|----------|----------|----------|
| C24 (truecolor) | — | C4 → C24 查 `_256_to_rgb`，C8 → C24 查 `_256_to_rgb` |
| C8 (256色) | C24 → `rgb_to_256` | C4 → `_4_to_256` |
| C4 (16色) | C24 → `nearest_4bit`，C8 → 先 `_256_to_rgb` 再 `nearest_4bit` | — |

检测规则：`$COLORTERM=truecolor/24bit` → C24，
`$TERM=*-256color` → C8，否则 C4。

---

## Style 描述符

```cpp
struct Style {
    Color fg;                                          // 前景色
    Color bg;                                          // 背景色
    std::vector<TextEffect> effects;                   // 特效
};
```

`Color` 可隐式从 `Color4`、`Color8`、`Rgb` 构造。`TextEffect` 值：

| 枚举 | 效果 |
|------|------|
| `BOLD` | 粗体 |
| `DIM` | 暗色 |
| `ITALIC` | 斜体 |
| `UNDERLINE` | 下划线 |
| `SLOW_BLINK` | 慢闪 |
| `RAPID_BLINK` | 快闪 |
| `REVERSE` | 反转 |
| `CONCEAL` | 隐藏 |
| `STRIKETHROUGH` | 删除线 |

```cpp
Style{.fg = Color4::GREEN, .bg = Color4::BLACK,
      .effects = {TextEffect::BOLD, TextEffect::UNDERLINE}};
```

---

## 输入系统

### Key 键盘事件

```cpp
enum KeyCode {
    KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, KEY_INS, KEY_DEL,
    KEY_F1, ..., KEY_F12,
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_BACKSPACE,
};

struct Key {
    char32_t cp   = 0;    // Unicode 码点（可打印字符）
    KeyCode  code = KEY_NONE;  // 特殊键
    bool     alt  = false; // Alt 修饰
    bool     ctrl = false; // Ctrl 修饰
};
```

支持的组合键：
- Ctrl+@/Space (NUL) → `cp=' '` + `ctrl=true`
- Ctrl+a–z → `cp='a'…'z'` + `ctrl=true`
- Ctrl+\\, ], ^, \_ → 对应字符 + `ctrl=true`
- Alt+x → `cp=x` + `alt=true`
- 方向键、Home/End、PgUp/PgDn、Insert/Delete、F1–F12 → `code` 对应值

### Mouse 鼠标事件

```cpp
struct MouseState {
    enum Button { NONE, LEFT, MIDDLE, RIGHT };
    enum Action { PRESS, RELEASE, DRAG, SCROLL_UP, SCROLL_DOWN };
    Button  button;
    Action  action;
    int     col;  // 0-based
    int     row;  // 0-based
};
```

### Input 统一事件

```cpp
enum InputType { INPUT_KEY = 0, INPUT_MOUSE = 1 };

struct Input {
    InputType          type;
    Key                key;
    MouseState         mouse;
};

std::optional<Input> getInput();   // 非阻塞
Input                waitInput();  // 阻塞（50ms poll 轮询）
```

使用示例：

```cpp
auto inp = tui.waitInput();
if (inp.type == INPUT_MOUSE) {
    auto& m = inp.mouse;
    printf("Mouse at (%d,%d)\n", m.col, m.row);
} else {
    auto& k = inp.key;
    if (k.code == KEY_ESC) break;
    if (k.cp == 'q') break;
}
```

鼠标需要显式启用：

```cpp
tui.enableMouse();
// ... 事件循环 ...
tui.disableMouse();  // 可选，析构时不会自动发送 disable 序列
```

---

## 全局函数速查

```cpp
// VaTui
static VaTui& VaTui::instance();
void   VaTui::init();
Framebuffer& VaTui::buffer();
std::optional<Input> VaTui::getInput();
Input                VaTui::waitInput();
void   VaTui::enableMouse();
void   VaTui::disableMouse();

// Framebuffer
void   Framebuffer::setSize(SizeArgs);
int    Framebuffer::getColMax() const;
int    Framebuffer::getRowMax() const;
void   Framebuffer::setPosition(PositionArgs);
int    Framebuffer::getOffsetCol() const;
int    Framebuffer::getOffsetRow() const;
void   Framebuffer::printText(TextArgs);
void   Framebuffer::clear();
void   Framebuffer::clearRegion(RegionArgs);
void   Framebuffer::fillRegion(FillArgs);
void   Framebuffer::swap();

// 参数结构体（均 support designated initialisers）
SizeArgs     { int col, row; }
PositionArgs { int col, row; }
TextArgs     { int col, row; std::string_view text; Style style = {}; }
RegionArgs   { int col, row, w, h; }
FillArgs     { int col, row, w, h; char ch; Style style = {}; }
```

---

## 完整示例

### 鼠标画板（test/tui/paint.cpp 精简版）

```cpp
#include <vatui.hpp>
#include <cstdio>

using namespace vatui;

int main() {
    auto& tui = VaTui::instance();
    tui.init();
    tui.enableMouse();
    auto& fb = tui.buffer();

    Style blue  = {.fg = Color4::WHITE, .bg = Color4::BRIGHT_BLUE};
    Style erase = {.fg = Color4::WHITE, .bg = Color4::BLACK};
    Style draw  = blue;

    bool quit = false;
    while (!quit) {
        auto ev = tui.waitInput();

        if (ev.type == INPUT_MOUSE) {
            auto& m = ev.mouse;
            if (m.button == MouseState::LEFT)  draw = blue;
            if (m.button == MouseState::RIGHT) draw = erase;
            if ((m.action == MouseState::PRESS || m.action == MouseState::DRAG)
                && m.col >= 0 && m.row >= 0)
                fb.fillRegion({m.col, m.row, 1, 1, ' ', draw});
        }

        if (ev.type == INPUT_KEY && ev.key.cp == 'q')
            quit = true;

        fb.swap();
    }
}
```

### 颜色检测（test/tui/color.cpp）

运行 `./test/tui/color` 查看终端对各颜色深度（4-bit / 8-bit / 24-bit）的支持情况。
