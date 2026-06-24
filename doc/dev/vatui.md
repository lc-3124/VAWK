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

Framebuffer 不直接调用终端，所有 ANSI 转义序列通过 `vaterm::color::*` 生成后缓存。

---

## 2. Cell 的三次演变

### 阶段一：`Cell::style_`

```cpp
struct Cell {
    Style    style_;       // fg:Color4 + bg:Color4 + effects:vector
    // ...
};
```

`swap()` 遍历时每遇到风格不同的格子就调用 `style_to_seq(style_)`。
问题：216 色块的立方体导致 swap 内做 216 次颜色转换。

### 阶段二：`Cell::sgr_` (std::string)

```cpp
struct Cell {
    std::string sgr_;      // 预计算的 SGR 字符串
    // ...
};
```

`fillRegion` / `printText` 在写入时一次算好 SGR 字符串，循环内 `cell.sgr_ = sgr`
（string 拷贝）。`swap()` 只比较 string + 直接输出。

Cell 大小约 40 B（含 SSO buffer）。1920 格全屏 ~77 KB。

### 阶段三：`Cell::sgr_id_` (uint32_t) ← 当前

```cpp
struct Cell {
    uint32_t    sgr_id_;   // 指向 sgr_pool_ 的索引
    // ...
};
```

Cell 缩至 12 B，1920 格 ~22 KB。SGR 字符串移入全局池。

---

## 3. SGR 驻留池 (intern pool)

### 结构

```cpp
class Framebuffer {
    std::vector<std::string> sgr_pool_;    // 唯一 SGR 字符串
    std::string              out_buf_;     // swap 输出缓冲

    uint32_t intern_sgr_(const std::string& s) {
        for (size_t i = 0; i < sgr_pool_.size(); ++i)
            if (sgr_pool_[i] == s) return static_cast<uint32_t>(i);
        sgr_pool_.push_back(s);
        return static_cast<uint32_t>(sgr_pool_.size() - 1);
    }
};
```

### 生命流程

```
用户调用 fillRegion(fg_sgr="\033[32m", bg_sgr="\033[40m")
  │
  ├─ concat: "\033[32m\033[40m"
  ├─ intern_sgr_ → 池查找 → 没找到 → push → 返回 index 2
  └─ for (cell : buf)
       └─ cell.sgr_id_ = 2          (4 B 赋值)
```

```
用户调用 clear()
  │
  ├─ sgr_pool_.clear()              (池清空)
  ├─ intern_sgr_("\033[37m\033[40m") → 返回 0
  └─ for (cell : buf)
       └─ cell.sgr_id_ = 0          (4 B 赋值)
```

### 为什么池查找用线性搜

池非常小：
- 一个典型的 paint 程序：黑底白字 (1) + 蓝 (1) + 红 (1) + 擦除 (1) + 状态栏 (2) = ~6 条
- 6×6×6 色块：最多 216 条（全不同），但这是极端情况
- clear() 或 setSize() 时池被清空重启

线性搜在池大小 < 1000 时比 hash 更快（无 hash 计算，cache 友好）。

### 碎片清理时如何保留原始 SGR

宽字符碎片清理时需要保留被覆盖格子的颜色。代码：

```cpp
// 保存旧格子的池索引
auto old_sid = buf_[idx - 1].sgr_id_;
// 重置格子
reset_cell_(buf_[idx - 1]);
// 恢复池索引
buf_[idx - 1].sgr_id_ = old_sid;
```

只拷贝 4 字节，而非 string 的深拷贝。之前 string 版本做的是完整的堆分配 + 拷贝。

---

## 4. 差异渲染 (swap 算法)

### 核心思路

只输出"自上次 swap 以来发生变化"的虚拟屏幕区域。每次 swap 将逻辑缓冲区与物理终端同步。

### 逐行扫描

```
for (r = 0..max_row) {
    for (c = 0..max_col) {
        跳过非头部 (isHead_=false)
        当前格 (r,c) 与上一格 (pr,pc) 比较：
          ─ 相邻？ pr==r && pc+1==c
          ─ 风格相同？ sgr_id_ == prev_sid

        相邻 + 风格一致 → 只追加字符，无控制序列
        不相邻           → move_to(r,c) + SGR + 字符
        相邻但风格不同   → reset + SGR + 字符
    }
}
```

### 合并相邻同风格

```
  "AAAAABBBBB"              ← buffer 内容
  ↑  SGR_A  ↑  SGR_B        ← 只输出两个 SGR 序列
  不输出: SGR_A SGR_A ...     ← 相邻同风格格子跳过
```

`seq` 布尔值判断相邻性：

```cpp
bool seq = (r == prev_r && c == prev_c + 1);
// true  → 当前格紧跟在上一格右边，可以省略 move_to
// false → 需要 move_to 定位
```

### out_buf_ 复用

每次 swap 开头 `out_buf_.clear()`（保留 `capacity()`），
末尾 `write(out_buf_)`。两次 swap 之间：
- 如果输出量相近，不会重新分配堆内存
- 相比每次 `std::string out;`，避免反复 malloc/free

---

## 5. 宽字符保护

### isHead_ / isLong_ 双标记

每个宽字符占据两个 Cell（width=2）：
- 左格：`isHead_=true,  isLong_=true`
- 右格：`isHead_=false, isLong_=true`

普通字符（width=1）：
- `isHead_=true,  isLong_=false`

### 覆盖检测

printText / fillRegion 写入前检查目标格子：

```cpp
// 目标格子是某宽字符的尾部
if (!cell_p.isHead_ && cell_p.isLong_) {
    // 清理头部（强制变为空格）
    // 保留头部格子的原始 SGR（通过 sgr_id_ 迁移）
    auto old_sid = buf_[idx - 1].sgr_id_;
    reset_cell_(buf_[idx - 1]);
    buf_[idx - 1].sgr_id_ = old_sid;
}

// 目标格子是某宽字符的头部
if (cell_p.isHead_ && cell_p.isLong_) {
    // 清理尾部
    auto old_sid = buf_[idx + 1].sgr_id_;
    reset_cell_(buf_[idx + 1]);
    buf_[idx + 1].sgr_id_ = old_sid;
}
```

右边界放不下宽字符时，写入带当前风格的空格并继续。这是 ECMA-48 推荐的软换行行为。

### 为什么保留原始 SGR

宽字符的碎片格（被覆盖一半的字符）在大多数终端模拟器中显示为"不应该出现的东西"。
vatui 填充一个带**原始颜色**的空格，视觉上那个区域就像被"擦除"了，但颜色和周围一致。

---

## 6. 颜色系统

### 预计算 SGR

```cpp
// 用户调用 fg(Color4::GREEN)
// → 立即执行 color::fg(Color4::GREEN)
// → 返回 "\033[32m"  存入 Style::fg_sgr

// 用户调用 bg(Rgb{255,100,50})
// → 立即执行 color::bg(255,100,50)
// → 返回 "\033[48;2;255;100;50m"  存入 Style::bg_sgr
```

### fillRegion 内部

```cpp
void fillRegion(FillArgs args) {
    // 只做一次：三个 SGR 字符串拼接 + 驻留
    uint32_t sid = intern_sgr_(
        style.fg_sgr + style.bg_sgr + style.effects_sgr
    );
    // 循环内：只赋值 4 字节
    for (每个格子) cell.sgr_id_ = sid;
}
```

### 没有"颜色深度检测"

之前的设计在 swap 时检测终端能力进行转换（C4→C8→C24 / 下行有损压缩）。
现在的设计：
- 用户在构造 Style 时调用 `fg(Color4::GREEN)`，立即转为 SGR 字符串
- `terminal::detect_color_depth()` 不再被 vatui 核心调用
- 如果用户想要不同的终端适配，在调用 `fg/bg` 之前检测终端，自行选择 | 这种做法把选择权交给用户

---

## 7. 输入解析

### input_buf_ 积累

`getInput()` 每次调用：
1. 非阻塞读尽 stdin 所有可用字节，追加到 `input_buf_`
2. 从 buffer 头部尝试解析一个完整的输入事件

### 解析顺序（优先级）

```
  ┌──────────────────────────────────┐
  │ 0x1B + '[' + '<'   → SGR mouse  │  匹配 SGR 鼠标
  │ 0x1B + '[' + ...   → CSI 序列    │  方向键/Home/End/Fn
  │ 0x1B + 'O' + ...   → SS3 序列    │  F1-F4 (另一编码)
  │ 0x1B + 0x20-0x7E  → Alt+key     │
  │ 0x1B               → ESC         │
  │ 0x09               → KEY_TAB     │
  │ 0x0D               → KEY_ENTER   │
  │ 0x7F               → BACKSPACE   │
  │ 0x00               → Ctrl+Space  │
  │ 0x01-0x1A          → Ctrl+a-z    │
  │ 0x1C-0x1F          → Ctrl+\]_^   │
  │ 0x20-0x7E          → ASCII       │
  │ 0xC2-0xF7          → UTF-8 解码  │
  └──────────────────────────────────┘
```

### 不完整序列

如果 buffer 以 `\033[` 开头但尚未收到 terminator（0x40-0x7E），
`getInput()` 返回 `nullopt`。`waitInput()` 用 `poll(fd, 50ms)` 轮询等待。

CSI 序列的最大长度没有硬限制，但 buffer 超过 128 字节时被清空（防洪水攻击）。

### Alt 键检测

`\033<c>` 其中 `c` 为可打印字符（0x20-0x7E）。与 ESC 键的区别：
- ESC = 裸 `\033` 后无跟随字节
- Alt+key = `\033` 后紧跟一个可打印字符

---

## 8. 性能特征

### 内存

| 项 | 之前 (string) | 之后 (pool) |
|----|--------------|-------------|
| Cell 大小 | 40 B | 12 B |
| 80×24 缓冲区 | 77 KB | 22 KB |
| 池字符串数 | N/A | ~10-50 |
| out_buf_ | 每次新建 | 复用保留 4 KB |

### 时间复杂度

| 操作 | 之前 | 之后 |
|------|------|------|
| fillRegion (同色 N 格) | N × string 赋值 | 1 次 intern + N × uint32_t 赋值 |
| swap (N 格, S 种风格) | S 次风格比较 + 可能转换 | S 次 uint32_t 比较 |
| clear (N 格) | N × string 赋值 | N × uint32_t 赋值 |

### 运行开销

一次 `intern_sgr_` 的成本 ~ 1 次 string concat + 池内线性搜。
在典型的应用中（~10 种风格），池搜基本命中，只是几次 string 比较。

---

## 9. VaTui 单例

### Meyer's Singleton

```cpp
VaTui& VaTui::instance() {
    static VaTui inst;
    return inst;
}
```

C++11 起线程安全的函数局部静态变量初始化。
构造函数自动检测终端大小（`TIOCGWINSZ`），设定 Framebuffer 尺寸。

### 延迟初始化

构造函数**不进入 raw mode**——raw mode 由 `init()` 进入。这样设计：
- 用户可以在构造后检查 terminal 属性
- 如果用户只需要 buffer（例如单元测试），可以不调 `init()`
- `init()` 是幂等的：多次调用只第一次实际生效

### 析构

`~VaTui()` 依赖 `~terminal()` 的 RAII：
- 显示光标 → 清屏 → 恢复 termios
- 不需要用户手动调用销毁

---

## 10. 文件组织

| 文件 | 内容 |
|------|------|
| `tui-utils/include/vatui.hpp` | 类声明：Style、Framebuffer、VaTui、参数结构体 |
| `src/utils/vatui.cpp` | 实现：Framebuffer 方法、VaTui 方法 |
| `test/tui/print.cpp` | CJK 文字+WASD 覆盖测试 |
| `test/tui/paint.cpp` | 鼠标画板 |
| `test/tui/color.cpp` | 三档颜色色块检测 |
| `doc/dev/vaterm.md` | vaterm 层实现思路 |
| `doc/man/vatui.md` | vatui 用户手册 |
