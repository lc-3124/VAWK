<p align="center">
  <br>
  <img src="logo.jpg" alt="VAWK" width="320">
  <br>
</p>

<h1 align="center">VAWK</h1>

<p align="center">
  <b>Visual Ansi Widget Kit</b><br>
  <i>带有浓厚「静态」「索引」风格的 TUI 框架</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue?logo=c%2B%2B">
  <img src="https://img.shields.io/badge/license-MIT-green">
  <img src="https://img.shields.io/badge/platform-Linux-red?logo=linux">
  <img src="https://img.shields.io/badge/status-WIP-orange">
</p>

---

## 状态

> **开发中** — 底层工具与事件框架已可用，组件/窗口/导航层仍在设计推进。

---

## 这是什么

VAWK 是一套面向 Linux 终端的 TUI 框架，用 C++23 写成，**零外部依赖**（只用 libc / POSIX）。

它分为三个自下而上的层：

| 层 | 职责 | 形态 |
|----|------|------|
| **vaterm** | 终端原语：颜色 / 光标 / UTF-8 / 系统信息 / 输入引擎 | header-only |
| **vatui** | 渲染：帧缓冲、差异刷新、宽字符保护、单例会话 | hpp + cpp |
| **vawk** | 装配：事件系统、实体、事件总线、组件 | hpp + cpp |

---

## 设计理念：静态 · 索引

VAWK 最鲜明的特点，是它**几乎不依赖任何动态机制**：

- **类型是静态的** —— 事件类型用宏在编译期定义（`VAWK_EVENT_DEFINE`），系统里没有反射、没有运行期类型注册。
- **导航是静态的** —— 页面预先声明一张二维导航图，方向键在图上移动游标，游标落点即聚焦对象。
- **状态是静态的** —— 输入缓冲是定长环形队列，颜色走中央 SGR 池，一切以「中央表 + 索引」组织。

而把这一切串起来的统一动作，是 **「移动游标 / 持有索引，在中央表里寻址」**：

```
事件系统    event_type_id<T>() ──► listeners_[id]        用类型 ID 索引监听表
输入引擎    dumper.cursor_      ──► 环形缓冲             用游标定位事件流
渲染        sgr_id_             ──► sgr_pool_            用索引指向中央 SGR 池
用户导航    导航图坐标           ──► 页面对象             用游标在二维图里聚焦
```

**聚焦、事件、渲染、输入，在 VAWK 里是同一件事——游标走索引。**

由此带来的好处是确定、可推理、缓存友好、易于测试：没有隐藏的对象图，没有意外堆分配，行为完全由静态结构决定。

---

## 项目结构

```
VAWK/
├── tui-utils/                  # 终端 TUI 工具库（vaterm + vatui）
│   ├── include/
│   │   ├── vaterm.hpp          # vaterm 统一入口
│   │   ├── vatui.hpp           # Framebuffer + VaTui 单例
│   │   └── vaterm/
│   │       ├── enums.hpp       # Color4 / Color8 / Rgb、光标、特效枚举
│   │       ├── color.hpp       # ANSI 颜色 + 色彩空间转换
│   │       ├── cursor.hpp      # 光标控制
│   │       ├── input.hpp       # 输入引擎（状态机 + 环形缓冲 + 消费者游标）
│   │       ├── term.hpp        # 终端管理（raw mode RAII、尺寸、阻塞/非阻塞 I/O）
│   │       ├── system.hpp      # 系统信息（用户名、主机名、环境变量等）
│   │       └── utf.hpp         # UTF-8 处理（码点操作 + 显示宽度）
│   └── LICENSE
├── include/                    # VAWK 核心框架
│   ├── vawk.hpp                # 统一入口
│   └── vawk/
│       ├── event.hpp           # 事件类型系统（静态类型 ID）
│       ├── entity.hpp          # Entity 基类（事件缓冲 + 订阅）
│       ├── event_upstream.hpp  # 事件源基类（监听表 + 后台循环）
│       ├── event_router.hpp    # 事件路由器（中央事件总线）
│       ├── input.hpp           # 终端输入 → 事件桥接（开发中）
│       ├── widget.hpp          # 组件框架（占位，暂缓）
│       ├── window.hpp          # 窗口管理（占位，暂缓）
│       └── drawable.hpp        # 绘制接口（占位，暂缓）
├── src/
│   ├── utils/vatui.cpp         # vatui 实现
│   └── vawk/                   # entity / event_upstream / event_router 实现
├── test/
│   ├── tui/input.cpp           # vatui 层输入测试
│   └── utils/                  # color / sys / utf / input 演示
├── contrib/gen_compdb.py       # compile_commands.json 辅助生成
├── input-uml.md                # vaterm::input 类图
├── Makefile
├── README.md
└── LICENSE
```

---

## 快速开始

定义事件、创建实体、订阅、派发：

```cpp
#include <vawk.hpp>
#include <cstdio>

// 1. 编译期定义事件类型
VAWK_EVENT_DEFINE(MyEvent)
    int value;
VAWK_EVENT_DEFINE_END

// 2. 定义实体并处理事件
class MyEntity : public vawk::Entity {
  public:
    void on_event(std::shared_ptr<vawk::event::Base> evt) override {
        if (auto* e = vawk::event::cast<MyEvent>(*evt)) {
            std::printf("got value %d\n", e->value);
        }
    }
};

int main() {
    vawk::EventRouter router;              // 中央事件总线

    MyEntity entity;
    entity.subscribe(&router, {vawk::event_type_id<MyEvent>()});

    router.push(std::make_shared<MyEvent>(MyEvent{42}));
    router.dispatch_once();                // 手动派发一次
}
```

终端绘制则走 vatui：

```cpp
#include <vatui.hpp>

auto& tui = vatui::VaTui::instance();
tui.init();
auto& fb = tui.buffer();
fb.printText({.col = 0, .row = 0, .text = "Hello, VAWK!", .style = {}});
fb.swap();                                  // 差异刷新到终端
```

---

## 构建

```bash
# 语法检查所有头文件（-Werror）
make check

# 编译 vatui / vawk 实现 → build/bin.o/
make lib

# 编译全部测试与演示
make test
```

依赖：支持 C++23 的 `g++`（或 `clang++`），Linux 终端环境。

---

## 路线图

- [x] vaterm：颜色 / 光标 / UTF-8 / 系统信息
- [x] vatui：帧缓冲、差异刷新、颜色深度自适应
- [x] vaterm::input：输入引擎（状态机解析 + 环形缓冲 + 消费游标）
- [x] vawk：事件类型系统、实体、事件总线
- [ ] vawk::input：终端输入 → 事件桥接
- [ ] 二维导航图：方向键移动游标实现聚焦
- [ ] widget / window / drawable：组件与窗口层（暂缓）

---

## 许可

MIT License. 详见 [LICENSE](LICENSE)。

---

<p align="center">
  <sub>
    Built by <a href="https://github.com/lc-3124">lc3124</a>
    · Design assisted by <b>DeepSeek</b>
  </sub>
</p>
