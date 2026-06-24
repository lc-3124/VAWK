# vaterm — 实现思路

> vaterm 是构建在 ANSI 转义序列和 POSIX 系统调用之上的 TUI 工具库，
> 提供无状态、线程安全的底层终端操作接口。

---

## 终端的工作原理

一个终端模拟器本质上是一个文本渲染引擎：

1. 它维护一个**光标位置**和一个**当前状态**（前景色、背景色、特效等）
2. 普通字符直接渲染到光标处，光标右移
3. 遇到 `\033`（ESC）开头的**转义序列**时，解析并改变状态

你无法直接告诉终端"把那个格子设成红色"——你只能先输出 SGR 序列改变颜色，
然后再输出文字。vaterm 就是生成这些转义序列的工具包。

---

## 架构

```
  ┌──────────────────────────────────┐
  │           vatui                  │  Framebuffer / VaTui 单例
  ├──────────────────────────────────┤
  │  vaterm 各模块                   │
  │  color / cursor / mouse / utf    │  header-only，只返回字符串
  │  terminal / sys                  │  不直接写终端
  ├──────────────────────────────────┤
  │           POSIX syscalls         │
  └──────────────────────────────────┘
```

vaterm 全部是 **header-only**，不涉及编译。所有函数返回 `std::string`，
而不是直接写终端。这样调用方可以自由组合、延迟写入或做单元测试。

---

## 各模块详解

### 1. enums — 基础类型

`vaterm/enums.hpp` 定义了所有颜色和光标相关的类型：

```cpp
namespace vaterm {

// ── 颜色类型 ─────────────────────────────────────────

// 16 色 ANSI 调色板（0-7 标准色，8-15 高亮色）
enum class Color4 : uint8_t {
    BLACK, RED, GREEN, YELLOW,           // 0-3
    BLUE, MAGENTA, CYAN, WHITE,          // 4-7
    BRIGHT_BLACK, BRIGHT_RED,            // 8-9
    BRIGHT_GREEN, BRIGHT_YELLOW,         // 10-11
    BRIGHT_BLUE, BRIGHT_MAGENTA,         // 12-13
    BRIGHT_CYAN, BRIGHT_WHITE,           // 14-15
};

// 256 色调色板索引（0 = 黑, 196 = 亮红, 255 = 白）
struct Color8 { uint8_t index; };

// 24-bit 真彩色
struct Rgb { uint8_t r, g, b; };

// 颜色深度枚举（值越大能力越强，可直接用 > 比较）
enum class ColorDepth : uint8_t { C4 = 0, C8 = 1, C24 = 2 };

// ── 光标 ─────────────────────────────────────────────

enum class CursorDir : uint8_t { UP, DOWN, LEFT, RIGHT };
enum class CursorShape : uint8_t {
    BLINKING_BLOCK     = 1,
    STEADY_BLOCK       = 2,
    BLINKING_UNDERLINE = 3,
    STEADY_UNDERLINE   = 4,
    BLINKING_BAR       = 5,
    STEADY_BAR         = 6,
};

// ── 文字特效 ─────────────────────────────────────────

enum class TextEffect : uint8_t {
    RESET, BOLD, DIM, ITALIC, UNDERLINE,
    SLOW_BLINK, RAPID_BLINK, REVERSE, CONCEAL, STRIKETHROUGH,
};

}
```

### 2. color — 颜色与特效

`vaterm/color.hpp` 生成 SGR 颜色序列。

#### 前景色 / 背景色

| 函数 | 输出示例 | 含义 |
|------|----------|------|
| `fg(Color4::RED)` | `\033[31m` | 4-bit 前景 |
| `fg(Color8{196})` | `\033[38;5;196m` | 8-bit 前景 |
| `fg(Rgb{255,0,0})` | `\033[38;2;255;0;0m` | 24-bit 前景 |
| `bg(Color4::BLUE)` | `\033[44m` | 4-bit 背景 |
| `bg(Color8{21})` | `\033[48;5;21m` | 8-bit 背景 |
| `bg(Rgb{0,0,255})` | `\033[48;2;0;0;255m` | 24-bit 背景 |

SGR 序列的规律很简单：
- 前景：`38`，背景：`48`
- 4-bit：直接 `30-37`（标准色）或 `90-97`（高亮色）
- 8-bit：`38;5;N` / `48;5;N`
- 24-bit：`38;2;R;G;B` / `48;2;R;G;B`

#### 文字特效与重置

```cpp
effect(TextEffect::BOLD);           // → "\033[1m"
effect({BOLD, UNDERLINE});          // → "\033[1;4m"
reset();                            // → "\033[0m"
```

`reset()` 关闭所有颜色和特效，回到终端默认状态。

#### 颜色空间转换

256 色调色板布局：

```
  0-7     标准色（同 Color4 前 8 个）
  8-15    高亮色（同 Color4 后 8 个）
  16-231  6×6×6 彩色立方体
  232-255 24 级灰度梯度
```

**`rgb_to_256(r, g, b)`** 把 24-bit 颜色映射到最近的 256 色索引：

```cpp
int ir = (r * 5 + 127) / 255;    // 将 0..255 映射到 0..5
int ig = (g * 5 + 127) / 255;
int ib = (b * 5 + 127) / 255;
int index = 16 + ir * 36 + ig * 6 + ib;
```

原理：把 RGB 立方体的每个维度分成 6 格，共 6×6×6 = 216 种颜色，
加上 16 个系统色 + 24 级灰度 = 256。

**`nearest_4bit(r, g, b)`** 用欧几里得距离找最近的 ANSI 16 色。

### 3. cursor — 光标控制

`vaterm/cursor.hpp` 生成光标定位序列。

```cpp
cursor::move_to(10, 20);   // → "\033[10;20H"  1-based 坐标
cursor::move(UP, 3);       // → "\033[3A"     上移 3 行
cursor::save();            // → "\033[s"      保存当前位置
cursor::restore();         // → "\033[u"      恢复
cursor::hide();            // → "\033[?25l"
cursor::show();            // → "\033[?25h"
cursor::shape(STEADY_BAR); // → "\033[6 q"    DECSCUSR
```

注意：`move_to` 的行列从 1 开始，这是终端协议的规定。

### 4. mouse — 鼠标追踪

终端鼠标追踪的原理：终端把鼠标事件编码为键盘输入流发到 stdin。
vatui 解析这些编码。

#### 启用

```cpp
mouse::enable();
// 发送三条序列：
//   \033[?1000h  — 启用按钮事件
//   \033[?1002h  — 启用拖拽事件
//   \033[?1006h  — 启用 SGR 扩展坐标（支持 223+ 行列）
```

#### 事件格式

终端发送：`\033[<Cb;Cx;RyM`（按下/拖拽）或 `\033[<Cb;Cx;Rym`（释放）

`Cb` 是按钮+修饰+动作的复合编码（位域）：

```
7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬───┬───┬───┐
│   │ a1│ a0│ m2│ m1│ m0│ b1│ b0│
└───┴───┴───┴───┴───┴───┴───┴───┘
        动作类型    修饰键   按钮编号
```

| 位 | 含义 |
|----|------|
| 0–1 | 按钮：0=左, 1=中, 2=右, 3=释放 |
| 2–4 | 修饰键：bit2=Shift, bit3=Meta, bit4=Ctrl |
| 5–6 | 动作：0=按/释, 1=拖拽, 2=滚轮上, 3=滚轮下 |

`Cx`、`Ry` 是终端报告的 1-based 列和行。`mouse::parse()` 内部转换为 0-based。

### 5. terminal — 终端管理

`vaterm/term.hpp` 是唯一持有状态的模块（管理 raw mode）。

#### 静态方法（无状态，不需实例化）

```cpp
terminal::clear_screen();     // → "\033[2J\033[H"  清屏+归位
terminal::clear_to_eol();     // → "\033[K"         清到行尾
terminal::clear_to_eos();     // → "\033[0J"        清到屏尾
terminal::clear_line();       // → "\033[2K"        清整行

uint16_t rows, cols;
terminal::size(rows, cols);   // TIOCGWINSZ ioctl

terminal::write("\033[31m");  // write(STDOUT_FILENO) 系统调用
terminal::flush();            // fsync(STDOUT_FILENO)
int ch = terminal::read_byte(); // 非阻塞，无数据返回 -1
char ch = terminal::getch();    // 阻塞
```

#### 实例方法（需要 terminal 对象）

```cpp
terminal t;
t.enter_raw();  // 进入原始模式
// ... 使用终端 ...
// ~terminal() 自动恢复原始模式
```

#### 原始模式做了什么

`cfmakeraw()` 一键设置以下属性：

| 终端行为 | 正常模式 | 原始模式 |
|----------|---------|---------|
| 按键回显 | 显示按的键 | 不回显 |
| 行缓冲 | 等回车才传递 | 每按一个键立即传递 |
| Ctrl+C | 发 SIGINT 终止程序 | 不处理，作为字节 0x03 |
| Ctrl+Z | 发 SIGTSTP 暂停 | 不处理 |
| Ctrl+\ | 发 SIGQUIT | 不处理 |
| Ctrl+V | 转义下一个字符 | 不处理 |
| 回车转换 | CR→NL | 原始字节 |
| 输出处理 | NL→CR+NL | 原始字节 |
| VMIN/VTIME | 依赖配置 | VMIN=0, VTIME=1（100ms 超时） |

#### 颜色深度检测

```cpp
auto depth = terminal::detect_color_depth();
```

逻辑：

```
$COLORTERM=truecolor / 24bit  ──→ C24
$TERM=xterm-kitty / alacritty / foot  ──→ C24
$TERM=*-256color  ──→ C8
否则 ──→ C4
```

结果静态缓存，首次调用后不再检查环境变量。

### 6. sys — 系统信息

```cpp
sys::user_name();   // 当前用户名（getpwuid → $USER）
sys::host_name();   // 主机名（gethostname）
sys::env("HOME");   // 环境变量（getenv）
sys::cwd();         // 当前工作目录
sys::exec("ls");    // 执行命令并捕获 stdout（popen）
```

### 7. utf — UTF-8 工具

#### 编码规则

| 首字节范围 | 码点范围 | 后续字节数 |
|-----------|----------|-----------|
| 0x00–0x7F | U+0000–U+007F | 0 |
| 0xC2–0xDF | U+0080–U+07FF | 1 |
| 0xE0–0xEF | U+0800–U+FFFF | 2 |
| 0xF0–0xF7 | U+10000–U+10FFFF | 3 |

```cpp
utf::char_bytes("世");         // 3
utf::count("Hello, 世界！");   // 9
utf::at("Hello, 世界！", 7);  // "世"
```

#### 字符宽度

终端里一个汉字占 2 列，英文字母占 1 列。终端根据宽度对齐文本。

```cpp
utf::char_width("a");          // 1
utf::char_width("世");         // 2
utf::width("Hello, 世界！");   // 11
```

宽度规则：CJK 表意文字、全角标点、韩文、彝文、Emoji 宽度为 2；
控制字符宽度为 0；其余为 1。

---

## 线程安全

所有命名空间函数（`color::*`, `cursor::*`, `utf::*`, `sys::*`）是纯函数，
不访问全局状态，可任意线程安全调用。

`terminal` 类的实例方法不是线程安全的——它持有 termios 备份。
每个线程应当有自己的 `terminal` 实例，或使用互斥保护。

---

## 测试

| 文件 | 测试内容 |
|------|----------|
| `test/utils/sys_demo.cpp` | 系统信息获取（无终端依赖） |
| `test/utils/utf_demo.cpp` | UTF-8 宽度/计数（无终端依赖） |
| `test/tui/color.cpp` | 4-bit / 8-bit / 24-bit 色块渲染（需终端） |
| `test/tui/print.cpp` | CJK 文字 + WASD 覆盖（需终端） |
| `test/tui/paint.cpp` | 鼠标画板交互（需终端） |
