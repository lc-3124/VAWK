# vatui — 实现思路与高级设计

> vatui 是构建在 vaterm 之上的帧缓冲与输入封装库。
> 本文档详细说明其内部数据结构、算法、设计取舍以及性能特性。

---

## 1. 架构总览

```
  用户代码 (printText / fillRegion / swap)
       │
       ▼
  ┌──────────────────────────────────────┐
  │            Framebuffer               │
  │  ┌─────┐  ┌──────────┐  ┌─────────┐ │
  │  │ pool │  │ out_buf_ │  │  Cell[]  │ │
  │  └─────┘  └──────────┘  └─────────┘ │
  └──────────────────────────────────────┘
       │
       ▼
  ┌──────────────────────────────────────┐
  │         vaterm (header-only)         │
  │  color / cursor / mouse / utf / sys  │
  └──────────────────────────────────────┘
       │
       ▼
  ┌──────────────────────────────────────┐
  │       terminal (RAII raw mode)       │
  │       write() / flush() / read()     │
  └──────────────────────────────────────┘
```

Framebuffer 不直接调用终端。所有 ANSI 转义序列在 `fillRegion` / `printText`
时通过 `vaterm::color::*` 预计算后存入 SGR 池。
`swap()` 只做整数比较和池索引查表，不做任何颜色转换。

---

## 2. Cell 的结构设计

### 数据结构

每个屏幕格子用一个 `Cell` 结构体表示：

```cpp
struct Cell {
    bool        isLong_ = false;   // 是否为宽字符的一部分
    bool        isHead_ = false;   // 是否为宽字符的头部
    char32_t    cp_     = 0;       // 字符的 Unicode 码点
    char        data_   = ' ';     // 实际字符（主要用于 ASCII 快速路径）
    uint32_t    sgr_id_ = 0;       // SGR 池索引
};
```

每个 Cell 只有 12 字节。80×24 的屏幕 ~22 KB，缓存友好。

### 为什么用 uint32_t 而不是存 SGR 字符串或 Style 对象

存字符串的话每个 Cell ~40 字节（含 SSO buffer），全屏 77 KB。
而且 `swap()` 时需要逐格比较字符串内容。

存 `sgr_id_`（池索引）只要 4 字节。`swap()` 比较两个 uint32_t
等于一条 CPU 指令。SGR 字符串的实际内容存放在一个全局向量中，
由所有同色的 Cell 共享。

### 字符存储

`data_` 存 `char` 是因为大部分场景是 ASCII 快速路径。
`cp_` 存 `char32_t` 是为 CJK 和 Emoji 准备的。`printText` 写入时
同时写两者：如果字符是 ASCII，`data_=ch, cp_=ch`；
如果是多字节字符，`data_=' ', cp_=解码后的码点`。

`swap()` 输出时优先用 `data_`（避开多字节解码路径）。

### isHead_ / isLong_ 双标记

宽字符（汉字、Emoji）在屏幕上占 2 列，因此需要 2 个 Cell：

```
列:  0    1
    [漢] [漢]       ← 两个 Cell 代表一个汉字
    head  tail
    long  long
```

- 左格：`isHead_=true,  isLong_=true` — 负责输出字符
- 右格：`isHead_=false, isLong_=true` — 只是占位，swap 跳过

普通字符（英文字母、数字）：
- `isHead_=true,  isLong_=false` — 占一个格子

### reset_cell_ 不重置 sgr_id_

```cpp
static void reset_cell_(Cell& cell) {
    cell.data_   = ' ';
    cell.cp_     = 0;
    cell.isLong_ = false;
    cell.isHead_ = true;
    // sgr_id_ 不重置——调用者管理
}
```

这是有意为之：宽字符碎片清理时需要保留原始颜色（见第 5 节）。
如果 `reset_cell_` 把 `sgr_id_` 也重置了，碎片就丢了颜色。

---

## 3. SGR 驻留池（intern pool）

### 原理

当用户在缓冲区画一个带颜色的格子时，颜色不会直接存入 Cell，
而是先去一个"字符串池"里查重：

```cpp
uint32_t intern_sgr_(const std::string& s) {
    for (size_t i = 0; i < sgr_pool_.size(); ++i)
        if (sgr_pool_[i] == s) return i;
    sgr_pool_.push_back(s);
    return sgr_pool_.size() - 1;
}
```

如果池里已有相同字符串，返回已有索引；否则追加并返回新索引。
这样每个唯一的 SGR 字符串在池中只存一份，Cell 只存 4 字节的索引。

### 流程示例

```
调用 fillRegion({fg_sgr="\033[32m", bg_sgr="\033[40m"})
  │
  ├─ 拼接: "\033[32m\033[40m"
  ├─ intern_sgr_ → 池中已有 → 返回 index 2
  └─ for 循环: cell.sgr_id_ = 2 (4 字节赋值)
```

```
调用 clear()
  │
  ├─ sgr_pool_.clear()             清空池
  ├─ intern_sgr_("\033[37m\033[40m") → 返回 0
  └─ for 循环: cell.sgr_id_ = 0
```

### 为什么线性搜索

池非常小。典型的应用：
- paint 画板：黑底白字(1) + 蓝色(1) + 红色(1) + 擦除(1) + 状态栏(2) ≈ 6 条
- 极端情况 6×6×6 色块：最多 216 条

线性搜在 < 1000 条目时比哈希表更快：
- 无哈希计算开销
- 顺序内存访问，cache 友好
- `clear()` 是 O(1)，vector 不释放内存

用 `unordered_map` 或 `set` 的话，每次 `clear()` 需要 O(N) 节点析构，
而且每个条目多 8–16 字节的桶指针开销。

### out_buf_ 复用

`swap()` 使用一个 `std::string out_buf_` 累积输出。
每次 `swap()` 开头 `out_buf_.clear()`（保留 capacity），末尾一次性 `write()`。

好处：
- 两次 swap 之间如果输出量相近，不会重新分配堆内存
- 相比每次新建临时 string，避免反复 malloc/free

### dirty_ 标记

`dirty_` 在 `printText` / `fillRegion` / `clear` 时置 true。
`swap()` 开头检查：如果为 false，跳过任何终端写入。
`swap()` 末尾重置为 false。

如果一个画面没有任何修改，swap 是空操作——零系统调用。

---

## 4. swap 算法（差异渲染）

### 核心思路

每次 `swap()` 遍历逻辑缓冲区（`buf_`），与物理终端（上次 swap 的输出）
做 diff，只输出变化的单元格。

### 逐行扫描

```
for (r = 0..max_row) {
    for (c = 0..max_col) {
        if (!isHead_) continue;          // 跳过宽字符尾部

        bool adjacent = (r == prev_r && c == prev_c + 1);
        bool same_sgr = (sgr_id_ == prev_sid);

        if (adjacent && same_sgr) {
            // 相邻且同风格：只追加字符，无控制序列
            out_buf_ += data_;
        } else if (!adjacent) {
            // 不连续：移动光标 + 设置风格 + 字符
            out_buf_ += move_to(r, c);
            out_buf_ += sgr_pool_[sgr_id_];
            out_buf_ += data_;
        } else {
            // 相邻但风格不同：结束上一个 SGR + 新 SGR + 字符
            out_buf_ += "\033[0m";
            out_buf_ += sgr_pool_[sgr_id_];
            out_buf_ += data_;
        }
    }
}
```

### 合并相邻同风格

```
屏幕显示:  你好世界
SGR 分布:  [RED  ][GREEN]
输出:      SGR_RED + "你好" + "\033[0m" + SGR_GREEN + "世界"
           不输出: SGR_RED + "你" + SGR_RED + "好" + ...
```

连续同风格的格子共享同一个 SGR 序列，避免重复输出。

### 为什么不用两帧缓冲

典型的双缓冲方案需要两个同样大小的缓冲区，`swap()` 时做全量拷贝。
vatui 只保留一个逻辑缓冲区，物理终端的状态"隐含"在上次 swap 输出中。
每次 swap 只输出变化部分，减少 I/O。

---

## 5. 宽字符保护

### 问题

终端显示宽字符（汉字、Emoji）时占用 2 列，但终端内部将其视为 2 个格子。
如果新写入的文本只覆盖其中一半，另一半会成为"碎片"：

```
写"Hello"→"你好"→覆盖时只覆盖"你"的左半…
  屏幕: [你] [好]         ← 正常
  写"X": [X] [好]         ← "你"的右半变成了碎片！
```

### 解决方案

写入前检测目标格子是否属于某个宽字符的尾部：

```cpp
// 目标格子是宽字符的尾部（尾巴格子）
if (!cell_p.isHead_ && cell_p.isLong_) {
    auto old_sid = buf_[idx - 1].sgr_id_;   // 保留颜色
    reset_cell_(buf_[idx - 1]);              // 清空头部
    buf_[idx - 1].sgr_id_ = old_sid;         // 恢复颜色
}

// 目标格子是宽字符的头部（头格子）
if (cell_p.isHead_ && cell_p.isLong_) {
    auto old_sid = buf_[idx + 1].sgr_id_;   // 保留颜色
    reset_cell_(buf_[idx + 1]);              // 清空尾部
    buf_[idx + 1].sgr_id_ = old_sid;         // 恢复颜色
}
```

关键设计：碎片格填入**原始颜色**的空格。这样视觉上就像被"擦除"了，
但颜色与周围一致。

### 边界溢出

如果宽字符写到屏幕最后一列放不下（比如宽字符在 79 列，需要 80 列），
写入一个带当前风格的空格（软换行），继续处理后续内容。

---

## 6. 输入解析

### 输入缓冲区

`getInput()` 每次调用：
1. 非阻塞读尽 stdin 所有可用字节，追加到 `input_buf_`
2. 从 buffer 头部尝试解析一个完整的输入事件

### 解析优先级（有限状态自动机）

```
字节序列               → 解析结果
─────────────────────────────────────────────────────
0x1B '[' '<' ... M/m  → SGR 鼠标事件
0x1B '[' ... 字母     → CSI 序列（方向键/Home/End/Fn）
0x1B 'O' ...          → SS3 序列（F1-F4 另一编码）
0x1B + 0x20-0x7E      → Alt+key（\033a → Alt+a）
0x1B                  → Escape
─────────────────────────────────────────────────────
0x09                  → Tab
0x0D                  → Enter
0x7F                  → Backspace
0x00                  → Ctrl+Space（cp=' ', ctrl=true）
0x01-0x1A             → Ctrl+a-z（cp='a'-'z', ctrl=true）
0x1C-0x1F             → Ctrl+\]^_（cp='\\'']''^''_', ctrl=true）
0x20-0x7E             → ASCII 可打印字符
0xC2-0xF7 开头        → UTF-8 多字节解码 → char32_t
```

### 不完整序列处理

CSI 序列以 `\033[` 开头，以 0x40-0x7E 范围内的字节结尾。
如果 buffer 以 `\033[` 开头但还没收到终结字节，就保留在 buffer 中，
下次调用继续等待。

安全上限：buffer 超过 128 字节时清空（防终端数据洪水攻击）。

### Alt 与 Escape 的区分

终端发送 Alt+a 时实际上发送 `\033a`（ESC + a）。
与单独按 Escape 的区别：

- Escape：裸 `\033`，没有后续字节。解析器返回 `Key{.code=KEY_ESC}`
- Alt+a：`\033a`，解析器返回 `Key{.cp='a', .alt=true}`

### 鼠标与键盘共存

`enableMouse()` 发送的序列让终端把鼠标事件编码为 `\033[<...M/m`。
输入解析器统一处理：
1. 先匹配 `\033[<` → SGR 鼠标
2. 再匹配 `\033[字母` → CSI 键盘
3. 最后匹配 `\033 + 可打印` → Alt+key

这样鼠标和键盘通过同一个 `getInput()` 获取。

---

## 7. VaTui 单例

### 生命周期

```cpp
VaTui& VaTui::instance() {
    static VaTui inst;    // C++11 起线程安全
    return inst;
}
```

构造函数：检测终端大小（`TIOCGWINSZ`），初始化 Framebuffer。
**不进入 raw mode**——raw mode 由 `init()` 延迟进入。

这样做的原因：
- 用户可以在构造后查询 terminal 属性
- 纯 buffer 操作（如单元测试）可以不调 `init()`
- `init()` 是幂等的，多次调用无副作用

析构函数：显示光标 → 清屏 → 恢复 termios。所有恢复都是自动的。

### 线程安全

Framebuffer 不是线程安全的——所有方法访问同一个 `buf_` 和 `sgr_pool_`。
如果在多线程使用，需要外部加锁。

VaTui 单例也不可重入——`getInput()` 读写 `input_buf_`。

vaterm 层函数（`vaterm::color::*` 等）是线程安全的——纯函数，无全局状态。

---

## 8. 性能特征

### 内存

| 项 | 容量 |
|----|------|
| Cell 大小 | 12 B |
| 80×24 缓冲区 | 22 KB |
| 池典型大小 | 10–50 条字符串 |
| out_buf_ 容量 | 约 4 KB（复用） |

### 时间复杂度

| 操作 | 成本 |
|------|------|
| `fillRegion`（N 格，同色） | 1 次 intern（拼接+池搜）+ N × uint32_t 赋值 |
| `swap`（N 格，S 种色） | S 次 uint32_t 比较 + 输出字符串 |
| `clear`（N 格） | N × uint32_t 赋值 |
| 一次 intern | ~1 次 string 拼接 + ～10 次 string 比较（命中时） |

### 为什么这么快

关键路径上没有颜色转换、没有字符串比较、没有动态内存分配。
`swap()` 内部最重的操作是 `out_buf_ += data_`（追加单字符到 string），
而这在 SSO 范围内时只是 memcpy 几个字节。

---

## 9. 文件组织

| 文件 | 内容 |
|------|------|
| `tui-utils/include/vatui.hpp` | 类声明：Style、Framebuffer、VaTui、参数结构体 |
| `src/utils/vatui.cpp` | 实现：Framebuffer 方法、VaTui 方法、输入解析器 |
| `test/tui/print.cpp` | CJK 文字+WASD 覆盖测试 |
| `test/tui/paint.cpp` | 鼠标画板（全部键事件演示） |
| `test/tui/color.cpp` | 三档颜色色块检测 |
| `doc/dev/vaterm.md` | vaterm 层实现思路 |
| `doc/man/vatui.md` | vatui 用户手册 |
