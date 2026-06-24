# vaterm — 实现思路

> vaterm 是构建在 ANSI 转义序列和 POSIX 系统调用之上的 TUI 工具库，
> 提供无状态、线程安全的底层终端操作接口。

---

## 架构分层

```
  ┌──────────────────────────────────┐
  │           vatui                  │  Framebuffer / VaTui 单例
  ├──────────────────────────────────┤
  │  vaterm::terminal   (term.hpp)   │  原始模式 / 清屏 / I/O
  │  vaterm::color      (color.hpp)  │  颜色 & 特效转义序列
  │  vaterm::cursor     (cursor.hpp) │  光标控制序列
  │  vaterm::mouse      (mouse.hpp)  │  鼠标追踪 & SGR 解析
  │  vaterm::utf        (utf.hpp)    │  UTF-8 工具
  │  vaterm::sys        (system.hpp) │  系统信息
  ├──────────────────────────────────┤
  │           POSIX / Linux syscalls │
  └──────────────────────────────────┘
```

vaterm 层全部是 **header-only**，不涉及编译。所有函数（terminal 类的实例方法除外）
返回 `std::string` 而非直接写终端——这使得它们可以安全地在任意线程中调用。

---

## 设计决策

### 1. Header-only + 无状态

- 颜色、光标、鼠标、UTF-8 模块全部是普通命名空间中的 `inline` 函数。
- 不依赖全局变量或静态存储，可重入。
- 唯一持有状态的是 `terminal` 类（原始模式的前后 termios），必须实例化。

### 2. 为什么 vaterm 不直接写终端

返回转义序列字符串，由调用方或 vatui 的 `swap()` 统一写入。这样做的好处：
- 调用方可以合并多个序列减少系统调用
- vatui 的 diff 渲染只在必要时发出序列
- 单元测试时可以捕获序列字符串做断言

### 3. 颜色深度自动检测

位于 `terminal::detect_color_depth()`：

```
$COLORTERM=truecolor / 24bit  ──→ C24
$TERM=xterm-kitty / alacritty / foot  ──→ C24
$TERM=*-256color  ──→ C8
否则 ──→ C4
```

结果静态缓存，首次调用后不再检查环境变量。转换规则：
- **上行**：C4 → 查 `_4_to_256` → C8 → 查 `_256_to_rgb` → C24（无损）
- **下行**：C24 → `rgb_to_256` → C8 → `nearest_4bit` → C4（有损）

`nearest_4bit` 使用 RGB 欧几里得距离找最近 ANSI 色。

### 4. 鼠标 SGR 解析

终端发送 `\033[<cb;col;row M/m`：
- `cb` 编码按钮和修饰键，低 2 位为按钮编号，bit 5-6 为动作类型
- 0 = 按下/释放（根据 terminator M/m 及 btn=3 区分），1 = 拖动，2 = 滚轮上，3 = 滚轮下
- col/row 从终端 1-based 转 0-based

### 5. 输入解析 NFA

`VaTui::getInput()` 的实现是一个有限状态自动机：

```
  ┌──────────┐   0x09         ┌──────────┐
  │ 0x09     │ ─────────────→ │ KEY_TAB  │
  │ 0x0D     │ ─────────────→ │ KEY_ENTER│
  │ 0x7F     │ ─────────────→ │ BACKSPACE│
  │ 0x00     │ ─────────────→ │ Ctrl+Space│
  │ 0x01-0x1A│ ─────────────→ │ Ctrl+a-z │
  │ 0x1C-0x1F│ ─────────────→ │ Ctrl+\]_^│
  │ 0x20-0x7E│ ─────────────→ │ ASCII    │
  │ 0xC2-0xF7│ ─────────────→ │ UTF-8 → cp│
  │ 0x1B [   │ ─────────────→ │ CSI → Key │
  │ 0x1B O   │ ─────────────→ │ SS3 → F1-F4│
  │ 0x1B <c> │ ─────────────→ │ Alt+key  │
  │ 0x1B [ < │ ─────────────→ │ SGR mouse │
  │ 0x1B     │ ─────────────→ │ ESC      │
  └──────────┘               └──────────┘
```

不完整的序列保留在 `input_buf_` 中，等待更多字节到来后再解析。`waitInput()`
的 busy-wait 使用 50 ms 超时的 `poll()`。

### 6. 原始模式 RAII

`terminal` 类在 `enter_raw()` 中用 `tcgetattr` 保存原始 termios，
调用 `cfmakeraw` 设置原始模式。析构函数自动恢复。
`VMIN=0, VTIME=1` 提供 100 ms 超时的非阻塞读。

---

## 模块文件索引

| 文件 | 命名空间 | 内容 |
|------|----------|------|
| `tui-utils/include/vaterm/enums.hpp` | `vaterm` | Color4、Color8、Rgb、ColorDepth、CursorDir、CursorShape、TextEffect |
| `tui-utils/include/vaterm/color.hpp` | `vaterm::color` | fg/bg 序列（4/8/24bit）、effect、reset、rgb_to_256、_256_to_rgb、_4_to_256、nearest_4bit |
| `tui-utils/include/vaterm/cursor.hpp` | `vaterm::cursor` | move、move_to、save/restore、show/hide、shape |
| `tui-utils/include/vaterm/mouse.hpp` | `vaterm::mouse` | enable/disable、parse、available |
| `tui-utils/include/vaterm/term.hpp` | `vaterm::terminal` | enter_raw、exit_raw、clear_*、size、write、flush、read_byte、detect_color_depth |
| `tui-utils/include/vaterm/system.hpp` | `vaterm::sys` | user_name、host_name、env、cwd、exec |
| `tui-utils/include/vaterm/utf.hpp` | `vaterm::utf` | char_bytes、count、at、char_width、width |

---

## 测试

- `test/tui/print.cpp` — CJK 文字渲染 + 宽字符保护 + WASD 覆盖
- `test/tui/paint.cpp` — 鼠标画板（按键、鼠标、状态栏）
- `test/tui/color.cpp` — 三档颜色色块渲染检测
