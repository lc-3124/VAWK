# vatui 用户手册

> vatui 是构建在 vaterm 之上的帧缓冲与输入封装库。
> 提供双缓冲单元格矩阵（1:1 映射到终端屏幕），
> 支持 4-bit / 8-bit / 24-bit 三种颜色深度，
> 以及统一的鼠标/键盘输入。

## 核心概念

### 终端只认字符串

终端模拟器（xterm, foot, kitty 等）本质上是一个文本渲染器。
你不能"设置颜色"——你只能**输出特定的字符串**，终端看到后才会改变颜色。

比如输出 `"\033[31m"` 之后，接下来的文字就会变成红色。
这种字符串叫 **SGR 序列**（Select Graphic Rendition）。

### 预计算（Pre-computation）

vatui 把颜色的转换放在**写入缓冲区的时刻**，而不是**输出到终端的时刻**：

```
用户构造 Style          → 立即转成 SGR 字符串  ✓（此时做颜色转换）
调用 fillRegion / swap  → 只做整数比较，直接输出  ✓（零转换）
```

这样 `swap()` 中只有整数比较和字符串拼接，没有任何颜色计算。
即便每帧刷新整个屏幕，开销也极低。

### 颜色选择交给终端检测

`fg()` / `bg()` 内部在首次调用时自动检测终端能力（`detect_color_depth()`），
把颜色转换为终端原生深度，之后缓存检测结果供后续复用。转换规则：

```
你写的代码            终端 C24     终端 C8      终端 C4
───────────────────────────────────────────────────────────
fg(Color4::RED)      → 4-bit       → 4-bit      → 4-bit
fg(Color8{196})      → 24-bit      → 8-bit      → 4-bit
fg(Rgb{255,0,0})     → 24-bit      → 8-bit      → 4-bit
```

不需要手动判断终端能力——`fg()` / `bg()` 自动处理。

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
                  .style = {.fg_sgr = fg(Color4::BRIGHT_GREEN),
                            .bg_sgr = bg(Color4::BLUE)}});
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
              .style = {.fg_sgr = fg(Rgb{255, 100, 50}),
                        .bg_sgr = bg(Color4::BLACK)}});
```

### 输出

```cpp
void swap();
```

差异渲染——只输出自上次 `swap()` 以来发生变化的单元格。
相邻同风格的单元格合并为一次 SGR 序列，减少输出量。

### 宽字符保护

当新文本覆盖到宽字符的尾部（`isLong_=true`）或头部时，
自动清理对应的另一半，并用**原始颜色**的空格填充碎片区域。
右边界放不下宽字符时，写入带当前风格的空格并继续。

---

## 颜色系统

### Style 描述符

`Style` 的三个字段直接存储预计算好的 SGR 字符串：

```cpp
struct Style {
    std::string fg_sgr      = "\033[37m";   // 前景 SGR（默认白）
    std::string bg_sgr      = "\033[40m";   // 背景 SGR（默认黑）
    std::string effects_sgr;                 // 特效 SGR（默认无）
};
```

通过 `fg()` / `bg()` / `effects()` 三个辅助函数构造 SGR 字符串，
它们立即调用对应的 `vaterm::color` 函数并返回。

```cpp
// 四种颜色类型
Style{.fg_sgr = fg(Color4::GREEN)}              // 4-bit 调色板
Style{.fg_sgr = fg(Color8{46})}                  // 8-bit 256 色调色板
Style{.fg_sgr = fg(Rgb{0, 255, 0})}             // 24-bit 真彩色
Style{.fg_sgr = fg(Color4::BRIGHT_RED),          // 混用
      .bg_sgr = bg(Color8{196}),
      .effects_sgr = effects({TextEffect::BOLD, TextEffect::UNDERLINE})}
```

### fg / bg / effects 辅助函数

```cpp
// 前景色
std::string fg(Color4 c);    // 4-bit
std::string fg(Color8 c);    // 8-bit 256 色索引
std::string fg(Rgb c);       // 24-bit 真彩色

// 背景色
std::string bg(Color4 c);
std::string bg(Color8 c);
std::string bg(Rgb c);

// 特效（可组合）
std::string effects(std::initializer_list<TextEffect> effects);
```

### 颜色类型

| 类型 | 示例 | 说明 |
|------|------|------|
| `Color4` | `Color4::BRIGHT_RED` | 16 色 ANSI 调色板 |
| `Color8` | `Color8{196}` | 256 色调色板索引 (0–255) |
| `Rgb` | `Rgb{255, 100, 50}` | 24-bit 真彩色 |

### 颜色深度检测

`fg()` / `bg()` 内部已经自动调用 `terminal::detect_color_depth()`，
不需要手动触发。如需直接查询终端能力（例如决定是否渲染 8-bit 色块），
可手动调用：

```cpp
auto depth = terminal::detect_color_depth();
// depth == ColorDepth::C24 → 24-bit 真彩色支持
// depth == ColorDepth::C8  → 256 色调色板
// depth == ColorDepth::C4  → 16 色 ANSI 调色板
```

检测规则（结果全局缓存，仅首次检查环境变量）：
```
$COLORTERM=truecolor / 24bit  ──→ C24
$TERM=xterm-kitty / alacritty / foot  ──→ C24
$TERM=*-256color  ──→ C8
否则 ──→ C4
```

### TextEffect 特效

| 枚举 | 效果 | SGR 码 |
|------|------|--------|
| `BOLD` | 粗体 | 1 |
| `DIM` | 暗色 | 2 |
| `ITALIC` | 斜体 | 3 |
| `UNDERLINE` | 下划线 | 4 |
| `SLOW_BLINK` | 慢闪 | 5 |
| `RAPID_BLINK` | 快闪 | 6 |
| `REVERSE` | 反转 | 7 |
| `CONCEAL` | 隐藏 | 8 |
| `STRIKETHROUGH` | 删除线 | 9 |

---

## 输入系统

### Key 键盘事件

```cpp
enum KeyCode : int {
    KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, KEY_INS, KEY_DEL,
    KEY_F1, ..., KEY_F12,
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_BACKSPACE,
};

struct Key {
    char32_t cp   = 0;       // Unicode 码点（可打印字符）
    KeyCode  code = KEY_NONE; // 特殊键
    bool     alt  = false;   // Alt 修饰
    bool     ctrl = false;   // Ctrl 修饰
};
```

支持的组合键：
- `Ctrl+@` / `Ctrl+Space` (0x00) → `cp=' '` + `ctrl=true`
- `Ctrl+a–z` (0x01–0x1A) → `cp='a'…'z'` + `ctrl=true`
- `Ctrl+\`, `]`, `^`, `_` (0x1C–0x1F) → 对应字符 + `ctrl=true`
- `Alt+x` → `cp=x` + `alt=true`
- 方向键、Home/End、PgUp/PgDn、Insert/Delete、F1–F12 → `code` 对应值

### Mouse 鼠标事件

```cpp
struct MouseState {
    enum Button : uint8_t { NONE, LEFT, MIDDLE, RIGHT };
    enum Action : uint8_t { RELEASE, PRESS, DRAG, SCROLL_UP, SCROLL_DOWN };
    Button  button;
    Action  action;
    int     col;   // 0-based
    int     row;   // 0-based
};
```

### Input 统一事件

```cpp
enum InputType : int { INPUT_KEY = 0, INPUT_MOUSE = 1 };

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

// vatui helper functions
std::string fg(Color4);
std::string fg(Color8);
std::string fg(Rgb);
std::string bg(Color4);
std::string bg(Color8);
std::string bg(Rgb);
std::string effects(std::initializer_list<TextEffect>);

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

    Style blue  = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BRIGHT_BLUE)};
    Style erase = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BLACK)};
    Style draw  = blue;
    Style sb    = {.fg_sgr = fg(Color4::BLACK), .bg_sgr = bg(Color4::BRIGHT_WHITE)};

    // 状态栏
    fb.fillRegion({0, fb.getRowMax() - 1, fb.getColMax(), 1, ' ', sb});
    fb.printText({.col = 0, .row = fb.getRowMax() - 1,
                  .text = "左键画 右键擦 ESC=q", .style = sb});
    fb.swap();

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
        if (ev.type == INPUT_KEY && ev.key.code == KEY_ESC)
            quit = true;

        fb.swap();
    }
}
```

### 颜色检测（test/tui/color.cpp）

运行 `./test/tui/color` 查看终端对各颜色深度（4-bit / 8-bit / 24-bit）的支持情况。

---

## 构建

```bash
# 编译 lib
make lib

# 语法检查所有头文件
make check

# 编译测试
make -C test/tui all

# 运行非交互测试
make -C test/tui check
```

链接：
```bash
g++ --std=c++23 -Iinclude -Itui-utils/include myapp.cpp build/bin.o/vatui.o -o myapp
```

---

## 常见问题

**Q: swap() 后屏幕无变化？**
A: 检查是否调用了 `tui.init()`。`VaTui` 构造函数不会进入 raw mode。

**Q: 颜色显示不对？**
A: 确保 `$TERM` 和 `$COLORTERM` 环境变量正确设置。
`fg()` / `bg()` 依赖它们判断终端能力。参见"颜色深度检测"节的检测规则。

**Q: 鼠标事件收不到？**
A: 需要先调用 `tui.enableMouse()`。确保终端支持 SGR 鼠标（xterm, foot, kitty 等）。
