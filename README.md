<<<<<<< HEAD
<p align="center">
  <br>
  <img src="logo.jpg" alt="VAWK" width="320">
  <br>
</p>

<h1 align="center">VAWK</h1>

<p align="center">
  <b>Visual Ansi Widget Kit</b><br>
  <i>面向 Linux 终端的 TUI 工具包</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B">
  <img src="https://img.shields.io/badge/license-MIT-green">
  <img src="https://img.shields.io/badge/platform-Linux-red?logo=linux">
  <img src="https://img.shields.io/badge/status-active-brightgreen">
</p>

---

## 项目结构

```
VAWK/
├── tui-utils/              # TUI 工具库（vaterm）
│   └── include/
│       ├── vaterm.hpp        — 统一入口
│       └── vaterm/
│           ├── enums.hpp     — 枚举定义（颜色、光标、特效）
│           ├── color.hpp     — ANSI 颜色控制
│           ├── cursor.hpp    — 光标控制
│           ├── term.hpp      — 终端管理（Terminal 类）
│           ├── system.hpp    — 系统信息
│           └── utf.hpp       — UTF-8 处理
├── include/                # VAWK 核心框架
│   ├── vawk.hpp              — 统一入口
│   └── vawk/
│       ├── event.hpp         — 事件类型系统
│       ├── entity.hpp        — Entity 基类
│       ├── event_upstream.hpp— 事件源
│       ├── event_router.hpp  — 事件路由器
│       ├── input.hpp         — 输入（占位）
│       ├── widget.hpp        — 占位
│       ├── window.hpp        — 占位
│       └── drawable.hpp      — 占位
├── CMakeLists.txt           # TODO: Day 1 创建
├── tests/                   # TODO: Day 2 创建
├── examples/                # TODO: Day 28 创建
├── Makefile
├── README.md
└── LICENSE
```


### Phase 1 — 基础设施 + vaterm 实现（Day 1-12）

**学习预备：** CMake 基础（`cmake_minimum_required`、`add_library`、`target_include_directories`）  

| Day | 日期 | 主题 | 具体任务 | 今日学习 |
|-----|------|------|---------|---------|
| 1 | 6/14 六 | CMake 接入 | 写 `CMakeLists.txt`，将 vaterm 编译为静态库，支持 `make` 一键构建 | CMake 基础语法 |
| 2 | 6/15 日 | 测试框架 | 集成 doctest（单头文件，无需安装），写第一个测试 `1+1==2` | doctest 用法 |
| 3 | 6/16 一 | color 实现 (1) | 实现 `fg()`, `bg()` 的 4-bit 和 8-bit 重载 | `std::string` 拼接、ANSI 转义原理 |
| 4 | 6/17 二 | color 实现 (2) | 实现 24-bit RGB、`effect()`, `reset()` | C++ 函数重载、`std::initializer_list` |
| 5 | 6/18 三 | cursor 实现 | 实现全部 9 个光标函数 | ANSI 光标逃逸序列 |
| 6 | 6/19 四 | term (1) | 实现 `clear_*` 静态函数、`size()`, `type()` | `ioctl()`、`$TERM` |
| 7 | 6/20 五 | term (2) | 实现 `enter_raw()` / `exit_raw()`  RAII | `termios` 结构体、原始模式 |
| 8 | 6/21 六 | term (3) | 实现 `write()`, `flush()`, `read_byte()` | `write(2)` vs `printf()`、非阻塞读取 |
| 9 | 6/22 日 | system 实现 | 实现 `user_name`, `host_name`, `env`, `cwd`, `exec` | `popen()`、环境变量 API |
| 10 | 6/23 一 | utf 实现 | 实现所有 UTF-8 工具函数 | UTF-8 编码规则、CJK 宽字符 |
| 11 | 6/24 二 | 颜色转换 | 实现 `rgb_to_256`, `_256_to_rgb`, `blend_256` 等 | 颜色空间、Cube 算法 |
| 12 | 6/25 三 | Phase 1 收尾 | vaterm 各模块写单元测试，`make test` 全绿 | 测试覆盖率思维 |

### Phase 2 — 事件系统（Day 13-20）

**学习预备：** `std::shared_ptr` / `std::weak_ptr`、虚函数与多态、`std::atomic`

| Day | 日期 | 主题 | 具体任务 | 今日学习 |
|-----|------|------|---------|---------|
| 13 | 6/26 四 | event 评审 | 理解现有 `event_type_id<T>()` + `VAWK_EVENT_DEFINE` 机制，加文档注释 | C++ 模板函数 `static local`、`type_id` 惯用法 |
| 14 | 6/27 五 | entity 实现 | 写 `entity.cpp`：`push_event`、`process_one_event`、互斥锁保护 | `std::mutex` + `std::lock_guard` |
| 15 | 6/28 六 | entity 生命周期 | 实现 `subscribe`/`unsubscribe`、防止野指针 | 原始指针 vs 智能指针 |
| 16 | 6/29 日 | event_upstream | 写 `event_upstream.cpp`：listener 注册表、多类型索引 | `std::unordered_map`、`std::vector` 嵌套 |
| 17 | 6/30 一 | 事件循环 | 实现 `dispatch_loop`、`start/stop_event_loop` | `std::thread` + `std::condition_variable` |
| 18 | 7/1 二 | event_router | 实现 `EventRouter::push` + `dispatch_once` | 生产者-消费者队列 |
| 19 | 7/2 三 | 线程安全加固 | 审查所有锁竞争、加 `Thread Safety Analysis` 注解 | 死锁预防、RAII lock |
| 20 | 7/3 四 | Phase 2 收尾 | 事件系统集成测试：多 Entity 并发收发 | 多线程测试技巧 |

### Phase 3 — 输入系统（Day 21-25）

**学习预备：** 状态机设计模式、Linux `read(2)` + `poll(2)`

| Day | 日期 | 主题 | 具体任务 | 今日学习 |
|-----|------|------|---------|---------|
| 21 | 7/4 五 | 输入架构 | 定义 `KeyEvent`/`MouseEvent`/`ResizeEvent`，设计转义序列状态机 | ANSI 转义序列格式（CSI/SS3/OSC） |
| 22 | 7/5 六 | 解析器核心 | 实现逐字节解析状态机：`ESC [` → CSI 序列 | 状态图转代码 |
| 23 | 7/6 日 | 键盘事件 | 映射修饰键 + 功能键，支持 bracketed paste 模式 | xterm key codes |
| 24 | 7/7 一 | 鼠标事件 | SGR 编码模式（1000/1002/1006），点击/拖动/滚轮 | X10 / SGR 鼠标协议 |
| 25 | 7/8 二 | Phase 3 收尾 | Input 与 EventRouter 对接，写输入测试 | `expect` 模拟输入 |

### Phase 4 — Widget 框架起步（Day 26-28）

**学习预备：** 双缓冲渲染、事件冒泡机制、简单的布局算法

| Day | 日期 | 主题 | 具体任务 | 今日学习 |
|-----|------|------|---------|---------|
| 26 | 7/9 三 | Widget 基类 | 设计 `Widget`（Rect, Visible, Focus, Parent/Child 树） | 组合模式、树遍历 |
| 27 | 7/10 四 | Drawable | 实现屏幕缓冲区 + 脏矩形 + 局部刷新 | `std::vector` 做画布、diff 算法 |
| 28 | 7/11 五 | 焦点 + 事件冒泡 | 实现 capture → target → bubble 三阶段分发 | 事件传播模型 |

### Phase 5 — 打磨（Day 29-30）

| Day | 日期 | 主题 | 具体任务 | 今日学习 |
|-----|------|------|---------|---------|
| 29 | 7/12 日 | 示例 + 文档 | 写 2-3 个完整可运行的 example，更新 README | 教是最好的学 |
| 30 | 7/13 一 | 检查 + tag | ASan 内存检查、静态分析、打 v0.1.0 tag | Sanitizer 用法 |

---

```
Week 1 ─── CMake 基础 →  doctest 用法  →  std::string  →  ANSI 转义
                                              ↓
Week 2 ─── termios  →  UNIX 系统调用  →  UTF-8 编码  →  颜色空间
                                              ↓
Week 3 ─── 模板与 type_id  →  智能指针  →  互斥锁  →  线程与条件变量
                                              ↓
Week 4 ─── 状态机  →  xterm 协议  →  组合模式  →  双缓冲渲染
```


| 主题 | 免费资源 |
|------|---------|
| CMake | 官网教程 `cmake --help-full` 或 <https://cmake.org/cmake/help/latest/guide/tutorial/> |
| doctest | 单头文件测试库：<https://github.com/doctest/doctest> |
| C++ 基础 | <https://learncpp.com>（中文：<https://learncpp-cn.github.io>） |
| ANSI 转义 | <https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797> |
| Linux termios | `man termios` 或 <https://man7.org/linux/man-pages/man3/termios.3.html> |
| xterm 鼠标 | <https://invisible-island.net/xterm/ctlseqs/ctlseqs.html> |
| C++ 线程 | <https://cppreference.com/w/cpp/thread> |


---

## 许可

MIT License.
