# vaterm 中文手册

> 版本 1.0 — 基于 ANSI 转义序列和 Linux 系统调用的 TUI 工具库

---

## 目录

1. [概述](#1-概述)
2. [模块概览](#2-模块概览)
3. [enums — 枚举与类型](#3-enums--枚举与类型)
4. [color — 颜色与特效](#4-color--颜色与特效)
5. [cursor — 光标控制](#5-cursor--光标控制)
6. [mouse — 鼠标追踪](#6-mouse--鼠标追踪)
7. [terminal — 终端管理](#7-terminal--终端管理)
8. [sys — 系统信息](#8-sys--系统信息)
9. [utf — UTF-8 工具](#9-utf--utf-8-工具)
10. [vatui — 帧缓冲与输入封装](#10-vatui--帧缓冲与输入封装)
11. [注意事项](#11-注意事项)

---

## 1. 概述

**vaterm** 是一个 C++23 header-only 的终端 UI 工具库，提供对 ANSI 转义序列的封装和 Linux 终端系统调用的便捷接口。

所有函数返回转义序列字符串，而非直接写入终端——这使得它们是**无状态、线程安全**的（`terminal` 类除外，它有 RAII 状态）。

### 编译要求

- C++23 编译器（GCC 14+ / Clang 18+）
- Linux 操作系统
- 无需链接额外库

### 使用方式

```cpp
#include <vaterm.hpp>
// 或按需包含子模块：
// #include <vaterm/color.hpp>
// #include <vaterm/cursor.hpp>
// ...

int main() {
    // color 返回转义序列字符串，需要输出到终端才生效
    printf("%sHello%s\n",
           vaterm::color::fg(vaterm::Color4::GREEN).c_str(),
           vaterm::color::reset().c_str());
}
```

编译：
```bash
g++ --std=c++23 -I tui-utils/include demo.cpp -o demo
```

---

## 2. 模块概览

| 头文件 | 命名空间 | 功能 |
|--------|----------|------|
| `vaterm/enums.hpp` | `vaterm` | 枚举定义：颜色、光标方向/形状、文字特效 |
| `vaterm/color.hpp` | `vaterm::color` | 前景/背景色（4bit/8bit/24bit）、文字特效、颜色空间转换 |
| `vaterm/cursor.hpp` | `vaterm::cursor` | 光标移动、显示/隐藏、保存/恢复、形状设置 |
| `vaterm/mouse.hpp` | `vaterm::mouse` | 鼠标追踪启用/禁用、SGR 事件解析、可用性检测 |
| `vaterm/term.hpp` | `vaterm::terminal` | 原始模式（RAII）、清屏、终端大小、I/O |
| `vaterm/system.hpp` | `vaterm::sys` | 用户名、主机名、环境变量、当前路径、命令执行 |
| `vaterm/utf.hpp` | `vaterm::utf` | UTF-8 码点计数、字符宽度、字符串切片 |

---

## 3. enums — 枚举与类型

头文件：`vaterm/enums.hpp` · 命名空间：`vaterm`

### Color4 — 4 位调色板

16 种标准 ANSI 颜色，前 8 个为标准色，后 8 个为高亮版本。

```cpp
enum class Color4 : uint8_t {
    // 标准色 (0–7)
    BLACK, RED, GREEN, YELLOW,
    BLUE, MAGENTA, CYAN, WHITE,
    // 高亮色 (8–15)
    BRIGHT_BLACK, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
    BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,
};
```

### CursorDir — 光标方向

```cpp
enum class CursorDir : uint8_t { UP, DOWN, LEFT, RIGHT };
```

### CursorShape — 光标形状（DECSCUSR）

```cpp
enum class CursorShape : uint8_t {
    BLINKING_BLOCK     = 1,  // 闪烁的块
    STEADY_BLOCK       = 2,  // 不闪烁的块
    BLINKING_UNDERLINE = 3,  // 闪烁的下划线
    STEADY_UNDERLINE   = 4,  // 不闪烁的下划线
    BLINKING_BAR       = 5,  // 闪烁的竖线
    STEADY_BAR         = 6,  // 不闪烁的竖线
};
```

### TextEffect — 文字特效

```cpp
enum class TextEffect : uint8_t {
    RESET         = 0,  // 重置所有特效和颜色
    BOLD          = 1,  // 粗体
    DIM           = 2,  // 暗淡
    ITALIC        = 3,  // 斜体
    UNDERLINE     = 4,  // 下划线
    SLOW_BLINK    = 5,  // 慢速闪烁
    RAPID_BLINK   = 6,  // 快速闪烁
    REVERSE       = 7,  // 反转前景/背景
    CONCEAL       = 8,  // 隐藏
    STRIKETHROUGH = 9,  // 删除线
};
```

### Rgb — RGB 颜色结构体

```cpp
struct Rgb {
    uint8_t r, g, b;
};
```

---

## 4. color — 颜色与特效

头文件：`vaterm/color.hpp` · 命名空间：`vaterm::color`

### 前景色

```cpp
// 4 位调色板
std::string fg(Color4 c);

// 8 位（256 色）调色板
std::string fg(uint8_t c);

// 24 位真彩色
std::string fg(uint8_t r, uint8_t g, uint8_t b);

// 从 Rgb 结构体设置
std::string fg(Rgb rgb);
```

示例：
```cpp
using namespace vaterm;
printf("%s红字%s\n",      color::fg(Color4::RED).c_str(),      color::reset().c_str());
printf("%s256色%s\n",     color::fg(196).c_str(),               color::reset().c_str());
printf("%s真彩色%s\n",    color::fg(255, 100, 50).c_str(),      color::reset().c_str());
printf("%sRGB结构体%s\n", color::fg(Rgb{128, 200, 255}).c_str(), color::reset().c_str());
```

### 背景色

```cpp
// 4 位调色板
std::string bg(Color4 c);

// 8 位调色板
std::string bg(uint8_t c);

// 24 位真彩色
std::string bg(uint8_t r, uint8_t g, uint8_t b);

// 从 Rgb 结构体设置
std::string bg(Rgb rgb);
```

### 文字特效

```cpp
// 单个特效
std::string effect(TextEffect e);

// 多个特效组合（用 initializer_list）
std::string effect(std::initializer_list<TextEffect> effects);

// 重置所有属性（颜色 + 特效）
std::string reset();
```

示例：
```cpp
// 粗体 + 下划线
printf("%s重要信息%s\n",
       color::effect({TextEffect::BOLD, TextEffect::UNDERLINE}).c_str(),
       color::reset().c_str());
```

### 颜色转换

```cpp
// RGB → 最近 256 色值
uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b);

// 256 色值 → 近似 RGB
Rgb _256_to_rgb(uint8_t c);

// 4 位色 → 256 色
uint8_t _4_to_256(Color4 c);

// 混合两个 256 色（按 RGB 平均）
uint8_t blend_256(uint8_t a, uint8_t b);

// 反转 256 色
uint8_t invert_256(uint8_t c);
```

示例：
```cpp
// 将真彩色映射到最近的 256 色调色板
auto idx = color::rgb_to_256(200, 100, 50);
printf("映射到 256 色索引: %d\n", idx);

// 反转颜色
auto inverted = color::invert_256(196);  // 红色 → 青色系
```

---

## 5. cursor — 光标控制

头文件：`vaterm/cursor.hpp` · 命名空间：`vaterm::cursor`

```cpp
// 移动到绝对位置（行列均为 1-based）
std::string move_to(uint16_t row, uint16_t col);

// 相对方向移动
std::string move(CursorDir dir, uint16_t steps = 1);

// 保存当前光标位置
std::string save();

// 恢复到上次保存的位置
std::string restore();

// 显示/隐藏光标
std::string show();
std::string hide();

// 设置光标形状
std::string shape(CursorShape s);

// 请求终端返回光标位置（终端回复 \033[row;colR）
std::string report_position();
```

示例：
```cpp
// 在终端中央输出
uint16_t rows, cols;
vaterm::terminal::size(rows, cols);
printf("%sVAWK%s",
       cursor::move_to(rows/2, cols/2 - 2).c_str(),
       color::reset().c_str());

// 隐藏光标
printf("%s", cursor::hide().c_str());
// ... 绘图 ...
printf("%s", cursor::show().c_str());
```

---

## 6. mouse — 鼠标追踪

头文件：`vaterm/mouse.hpp` · 命名空间：`vaterm`

vaterm 支持 SGR 编码的 XTerm 鼠标追踪（`\033[?1000h` / `?1002h` / `?1006h`），可识别点击、拖拽、滚轮。

### MouseState 结构体

```cpp
struct MouseState {
    int col, row;                       // 事件位置（0-based）
    enum Button : uint8_t { NONE, LEFT, MIDDLE, RIGHT };
    enum Action : uint8_t { RELEASE, PRESS, DRAG, SCROLL_UP, SCROLL_DOWN };
    Button button;
    Action action;
};
```

### mouse 类

```cpp
class mouse {
public:
    static std::string enable();              // 启用追踪的转义序列
    static std::string disable();             // 禁用追踪的转义序列
    static bool        available();           // 检查终端是否为 tty

    static std::optional<MouseState>           // 解析 SGR 转义序列
        parse(std::string_view seq);

    MouseState capture();                     // 阻塞等待下一次鼠标事件
};
```

- `enable()` 启用 SGR 按钮事件追踪（`?1000h`+`?1002h`+`?1006h`）
- `capture()` 内部读 stdin 并解析，阻塞直到收到完整鼠标事件
- `parse()` 独立的状态解析器，配合 `poll()` 可实现键盘+鼠标共存

### 使用示例 — 画板

```cpp
#include <vaterm.hpp>
#include <cstdio>
#include <poll.h>
#include <string>

using namespace vaterm;

int main() {
    terminal term;
    term.enter_raw();
    terminal::write(terminal::clear_screen());
    terminal::write(mouse::enable());

    std::string buf;
    bool quit = false;

    while (!quit) {
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        if (poll(&pfd, 1, 50) <= 0) continue;

        char c;
        if (::read(STDIN_FILENO, &c, 1) <= 0) continue;

        if (c == 'q' || c == 'Q') break;          // 键盘退出
        buf += c;

        auto esc = buf.rfind('\033');
        if (esc == std::string::npos) { buf.clear(); continue; }
        auto tail = buf.substr(esc);
        if (tail.back() != 'M' && tail.back() != 'm') continue;

        if (auto s = mouse::parse(tail)) {
            buf.clear();
            if (s->button == MouseState::LEFT)
                terminal::write(cursor::move_to(s->row, s->col) +
                                color::fg(0,100,255) + "*" + color::reset());
            if (s->button == MouseState::RIGHT)
                terminal::write(cursor::move_to(s->row, s->col) +
                                color::fg(255,50,50) + "*" + color::reset());
        }
    }

    terminal::write(mouse::disable());
    return 0;
}
```

运行交互式画板 demo：

```bash
./test/utils/mouse_demo      # 左键画蓝、右键画红、Q 退出
```

### SGR 编码参考

| Button 码 | 含义 | 修饰位 |
|-----------|------|--------|
| 0–2 | 左/中/右键按下 | +4=Shift, +8=Meta, +16=Ctrl |
| 32–34 | 左/中/右键拖拽 | 同上 |
| 64–65 | 滚轮上/下滚动 | 同上 |

序列格式：`\033[<Cb;Cx;CyM`（按下/拖拽/滚动）或 `\033[<Cb;Cx;Cym`（松开）。

---

## 7. terminal — 终端管理

头文件：`vaterm/term.hpp` · 命名空间：`vaterm`

`terminal` 是 vaterm 中唯一的类（非命名空间函数）。它管理终端的原始模式切换并持有状态。

```cpp
class terminal {
public:
    terminal() = default;
    ~terminal();              // RAII：析构时自动退出原始模式

    // 禁止拷贝
    terminal(const terminal&) = delete;
    terminal& operator=(const terminal&) = delete;
};
```

### 静态方法（无状态）

```cpp
// 清屏并归位光标
static std::string clear_screen();

// 清除光标到行尾
static std::string clear_to_eol();

// 清除光标到屏尾
static std::string clear_to_eos();

// 清除整行
static std::string clear_line();

// 获取终端行列数，成功返回 true
static bool size(uint16_t& rows, uint16_t& cols);

// 获取 $TERM 环境变量值
static std::string type();

// 写入字符串到 stdout（直接 write 系统调用）
static void write(std::string_view s);

// 刷新 stdout（fsync）
static void flush();

// 非阻塞读取一个字节，无数据返回 -1
static int read_byte();

// 阻塞读取一个字符（一直等待直到有数据）
static char getch();
```

### 实例方法（有状态）

```cpp
// 进入原始模式（关闭回显、行缓冲等），成功返回 true
bool enter_raw();

// 退出原始模式，恢复原始终端设置
void exit_raw();

// 检查当前是否处于原始模式
bool is_raw() const;
```

### 完整示例

```cpp
#include <vaterm.hpp>
#include <cstdio>

int main() {
    vaterm::terminal term;

    if (!term.enter_raw()) {
        fprintf(stderr, "无法进入原始模式\n");
        return 1;
    }

    // 使用静态方法清屏
    vaterm::terminal::write(term.clear_screen());

    // 读取按键
    int ch;
    while ((ch = vaterm::terminal::read_byte()) != 'q') {
        // 处理按键...
    }

    // 析构时自动 exit_raw()
    return 0;
}
```

### 原始模式说明

`enter_raw()` 会设置以下终端属性：

| 属性 | 效果 |
|------|------|
| 非规范模式 | 不等待回车，立即读取 |
| 关闭 ECHO | 按键不回显 |
| 关闭 ICANON | 禁用行缓冲 |
| 关闭 ISIG | 禁用 Ctrl+C/SIGINT 等信号生成 |
| VMIN=0, VTIME=1 | 非阻塞读取，超时 100ms |

---

## 8. sys — 系统信息

头文件：`vaterm/system.hpp` · 命名空间：`vaterm::sys`

```cpp
// 当前用户名（优先 getpwuid，回退 $USER）
std::string user_name();

// 主机名
std::string host_name();

// 获取环境变量值（不存在返回空字符串）
std::string env(const std::string& key);

// 当前工作目录绝对路径
std::string cwd();

// 执行命令并获取 stdout 输出
std::string exec(const std::string& command);
```

示例：
```cpp
using namespace vaterm::sys;

auto user = user_name();        // "root" 或当前用户名
auto host = host_name();        // "my-machine"
auto home = env("HOME");        // "/home/user"
auto dir  = cwd();              // "/home/user/project"
auto out  = exec("ls -la");     // 执行 ls 命令

printf("%s@%s: %s$\n", user.c_str(), host.c_str(), dir.c_str());
```

> ⚠️ 安全注意：`exec()` 内部使用 `popen()`，不要传入不可信的命令字符串。

---

## 9. utf — UTF-8 工具

头文件：`vaterm/utf.hpp` · 命名空间：`vaterm::utf`

```cpp
// 返回字符串开头的 UTF-8 字符的字节长度
// 无效首字节返回 0，空字符串返回 0
size_t char_bytes(std::string_view s);

// 返回 UTF-8 字符串的码点数（非字节数）
size_t count(std::string_view s);

// 获取第 i 个（0-based）码点的子串视图
// 越界返回空 string_view
std::string_view at(std::string_view s, size_t i);

// 单个 UTF-8 字符的显示宽度
// 控制字符 → 0，CJK/全角 → 2，半角 → 1
int char_width(std::string_view s);

// 整个字符串的总显示宽度
int width(std::string_view s);
```

### 宽度规则

`char_width` 和 `width` 遵循 Unicode 东亚洲宽度属性（East Asian Width）：

| 类别 | 宽度 | 范围示例 |
|------|------|----------|
| 控制字符（0x00–0x1F, 0x7F） | 0 | 换行、制表符等 |
| ASCII 可见字符 | 1 | 英文字母、数字、常见符号 |
| CJK 统一表意文字 | 2 | 汉字 |
| 全角符号 | 2 | 全角逗号、括号等 |
| 韩文（Hangul） | 2 | 한글 |
| 彝文（Yi） | 2 | ꀀ–꒏ |
| Emoji（部分） | 2 | 😀、🎉 |
| 中日韩统一表意文字扩展区 | 2 | 扩展 B–H 区 |

### 示例

```cpp
using namespace vaterm::utf;

auto s = "Hello, 世界!"sv;

count(s);        // 9（H,e,l,l,o,,, ,世,界,!）
width(s);        // 11（每个 ASCII 1，汉字各 2）
char_bytes("世"sv);  // 3
char_bytes("a"sv);   // 1

at(s, 7);        // "世"
at(s, 8);        // "界"
at(s, 9);        // "!"
```

### 与终端对齐的实用技巧

使用 `width()` 计算字符串实际占用列数，以实现对齐：

```cpp
void print_aligned(const std::string& label, const std::string& value, int width) {
    auto pad = width - utf::width(label);
    printf("%s%s%*s%s%s\n",
           color::fg(Color4::CYAN).c_str(), label.c_str(),
           pad > 0 ? pad : 0, "",
           color::reset().c_str(), value.c_str());
}
```

---

## 10. 注意事项

### 终端兼容性

- 所有颜色和光标功能依赖终端对 ANSI/XTerm 转义序列的支持
- 不支持 ANSI 的终端（如老式串口终端）可能出现乱码
- 建议在 `TERM=xterm-256color` 或兼容环境下使用

### 平台依赖

- 仅支持 Linux/Unix 平台
- 使用以下 POSIX API：
  - `termios.h` — 原始模式控制
  - `sys/ioctl.h` — 终端大小查询
  - `unistd.h` — `read`/`write`/`gethostname` 系统调用
  - `pwd.h` — 用户信息

### 线程安全

除 `terminal` 类（持有原始模式状态）外，所有函数均为纯函数，线程安全。

## 11. vatui — 帧缓冲与输入封装

> `vatui`（`tui-utils/include/vatui.hpp` + `src/utils/vatui.cpp`）是构建在 vaterm 之上的帧缓冲库。
> 它提供了双缓冲单元格矩阵（1:1 映射到终端屏幕），并封装了鼠标/键盘输入。
> 与 vaterm 不同，vatui 是分声明/实现文件的（.hpp + .cpp），需链接 `build/bin.o/vatui.o`。

### 设计思想

vatui 的核心设计是 **预计算**：所有颜色和特效在构造 `Style` 的瞬间
就被转换为 ANSI SGR 转义字符串。`swap()` 时不再做任何颜色转换，
只做整数比较和字符串池输出。

颜色深度适配的控制权交给用户：在调用 `fg()`/`bg()` 之前，
可以使用 `terminal::detect_color_depth()` 检测终端能力，
自行选择合适的颜色类型。

---

### 11.1 帧缓冲 API

```cpp
namespace vatui {

// 颜色辅助函数——立即返回 SGR 转义序列字符串
std::string fg(Color4 c);    // 4-bit
std::string fg(Color8 c);    // 8-bit 256 色
std::string fg(Rgb c);       // 24-bit 真彩色
std::string bg(Color4 c);
std::string bg(Color8 c);
std::string bg(Rgb c);
std::string effects(std::initializer_list<TextEffect>);

// Style 描述符——三个字段直接存储预计算好的 SGR 字符串
struct Style {
    std::string fg_sgr      = "\033[37m";   // 前景 SGR（默认白）
    std::string bg_sgr      = "\033[40m";   // 背景 SGR（默认黑）
    std::string effects_sgr;                 // 特效 SGR（默认无）
};

// 所有绘制操作均使用形参结构体，支持 designated-initialiser 语法。
struct SizeArgs      { int col, row; };
struct PositionArgs  { int col, row; };
struct TextArgs      { int col, row; std::string_view text; Style style = {}; };
struct RegionArgs    { int col, row, w, h; };
struct FillArgs      { int col, row, w, h; char ch; Style style = {}; };
class Framebuffer {
    void setSize(SizeArgs args);    // 调整缓冲区大小（自动清空）
    int  getColMax() const;
    int  getRowMax() const;

    void setPosition(PositionArgs args);   // 渲染偏移（相对终端左上角）
    int  getOffsetCol() const;
    int  getOffsetRow() const;

    void printText(TextArgs args);         // 写入文本（支持 UTF-8、中日韩宽字符）
    void clear();                          // 清除整个缓冲区
    void clearRegion(RegionArgs args);
    void fillRegion(FillArgs args);

    void swap();  // 差异渲染：只输出变化的单元格，相邻同风格合并
};

class VaTui {
    static VaTui& instance();   // 全局唯一实例

    void init();                // 进入 raw mode、隐藏光标、清屏（幂等）

    Framebuffer& buffer();      // 获取帧缓冲引用

    std::optional<Input> getInput();
    Input                waitInput();

    void enableMouse();
    void disableMouse();
};

}
```

**使用示例：**

```cpp
using namespace vatui;

// 构造 Style——颜色立即转为 SGR 字符串
Style normal = {.fg_sgr = fg(Color4::WHITE), .bg_sgr = bg(Color4::BLACK)};
Style warn   = {.fg_sgr = fg(Color8{196}),               // 亮红
                .bg_sgr = bg(Color4::BLACK),
                .effects_sgr = effects({TextEffect::BOLD})};
Style rgb    = {.fg_sgr = fg(Rgb{255, 100, 50})};
```

**生命周期：**

- `VaTui` 是单例（Meyer's Singleton）。首次调用 `instance()` 时构造函数自动
  检测终端大小并设定 `Framebuffer` 尺寸。
- `init()` 进入 raw mode、隐藏光标、清屏。可多次调用（幂等）。
- 析构函数在程序退出时自动显示光标、清屏、恢复 termios。

**宽字符保护：**

写入新文本时自动处理被覆盖的宽字符碎片：
- 如果目标单元格是某宽字符的尾部（`isLong_ = true`），则清理其头部（写为带**原始颜色**
  的空格）。
- 如果目标单元格是某宽字符的头部，则清理其尾部（同样保留原始颜色）。
- 右边界放不下宽字符时，写入带当前风格的空格并继续。

---

### 11.2 统一输入 API

vatui 提供统一的非阻塞/阻塞输入方法，通过 `Input.type` 区分键盘和鼠标事件：

```cpp
enum KeyCode : int {
    KEY_NONE = 0,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, KEY_INS, KEY_DEL,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_BACKSPACE,
};

struct Key {
    char32_t cp   = 0;       // 可打印字符的 Unicode 码点
    KeyCode  code = KEY_NONE; // 特殊键码（可打印键时为 KEY_NONE）
    bool     alt  = false;   // Alt/Meta 修饰键
    bool     ctrl = false;   // Ctrl 修饰键
};

enum InputType : int { INPUT_KEY = 0, INPUT_MOUSE = 1 };

struct Input {
    InputType          type;
    Key                key;
    vaterm::MouseState mouse;
};

std::optional<Input> getInput();   // 非阻塞
Input                waitInput();  // 阻塞

void enableMouse();    // 发送 SGR 鼠标追踪开启序列（仅初始化后生效）
void disableMouse();   // 发送关闭序列
```

**输入解析顺序：**

`getInput()` 内部实现为有限状态自动机，每次调用先耗尽 stdin 再尝试解析：

| 原始字节 | 解析结果 |
|----------|----------|
| `\033[<…M/m` | SGR 鼠标事件 |
| `\033[…字母` | CSI 序列（方向键、Home/End、F1–F12 等） |
| `\033O…` | SS3 序列（F1–F4 另一编码） |
| `\033` + 可打印字符 | Alt+key |
| 裸 `\033` | Escape |
| `0x09` | Tab |
| `0x0D` | Enter |
| `0x7F` | Backspace |
| `0x00` | Ctrl+Space |
| `0x01–0x1A` | Ctrl+a–z |
| `0x1C–0x1F` | Ctrl+\]^\_ |
| `0x20–0x7E` | ASCII 直接返回 |
| `0xC2–0xF7` 开头 | UTF-8 解码为 Unicode 码点 |

不完整的序列保留在 `input_buf_` 中，下次调用时继续解析。
`waitInput()` 循环调用 `getInput()`，若返回 `nullopt` 则 `poll()` 等待 50 ms 后重试。

**完整示例：**

```cpp
#include <vatui.hpp>
#include <cstdio>

using namespace vatui;

int main() {
    auto& tui = VaTui::instance();
    tui.init();
    tui.enableMouse();

    auto& fb = tui.buffer();
    fb.printText({.col = 0, .row = 0, .text = "Move mouse or press Q",
                  .style = {.fg_sgr = fg(Color4::GREEN)}});
    fb.swap();

    while (true) {
        auto inp = tui.waitInput();
        if (inp.type == INPUT_MOUSE) {
            auto& m = inp.mouse;
            printf("Mouse at (%d,%d)\n", m.col, m.row);
        } else if (inp.type == INPUT_KEY) {
            auto& k = inp.key;
            if (k.code == KEY_ESC || k.cp == 'q' || k.cp == 'Q')
                break;
        }
    }
}
```

---

### 11.3 构建

vatui 不是 header-only，需要单独编译：

```bash
make lib                    # 编译为 build/bin.o/vatui.o
g++ --std=c++23 -Iinclude -Itui-utils/include \
    myapp.cpp build/bin.o/vatui.o -o myapp
```

测试程序：

```bash
make -C test/tui all        # 编译 print / paint / color
./test/tui/print            # CJK 文字+WASD 覆盖测试
./test/tui/paint            # 鼠标画板交互测试
./test/tui/color            # 三档颜色色块检测
```

---

> 本文档使用 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) 许可。
> 如有问题请提交 Issue 至 [VAWK](https://github.com/lc-3124/VAWK)。
