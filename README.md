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
├── tui-utils/              # TUI 工具库（vaterm + vatui）
│   ├── include/
│   │   ├── vaterm.hpp        — vaterm 统一入口
│   │   ├── vatui.hpp         — Framebuffer（SGR 池、差异渲染、宽字符保护）
│   │   │                       + VaTui 单例（统一输入）
│   │   └── vaterm/
│   │       ├── enums.hpp     — 枚举定义（Color4/Color8/Rgb、光标、特效）
│   │       ├── color.hpp     — ANSI 颜色控制 + 颜色空间转换
│   │       ├── cursor.hpp    — 光标控制
│   │       ├── term.hpp      — 终端管理（terminal 类：raw mode RAII、I/O）
│   │       ├── mouse.hpp     — SGR 鼠标追踪（MouseState、parse、capture）
│   │       ├── system.hpp    — 系统信息（用户名、主机名、环境变量）
│   │       └── utf.hpp       — UTF-8 处理
│   └── MANUAL_zh.md          — 中文综合手册（vaterm + vatui）
├── src/
│   └── utils/
│       └── vatui.cpp         — VaTui 实现（需编译为 build/bin.o/vatui.o）
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
├── doc/                    # 文档
│   ├── dev/
│   │   ├── compile.md        — 编译指南
│   │   ├── vaterm.md         — vaterm 实现思路
│   │   └── vatui.md          — vatui 高级设计
│   └── man/
│       └── vatui.md          — vatui 用户手册
├── test/
│   ├── tui/
│   │   ├── color.cpp         — 4-bit/8-bit/24-bit 色块渲染
│   │   ├── print.cpp         — CJK 文字 + WASD 覆盖
│   │   └── paint.cpp         — 鼠标画板（全部键事件演示）
│   └── utils/
│       ├── sys_demo.cpp      — 系统信息测试
│       └── utf_demo.cpp      — UTF-8 测试
├── contrib/
│   └── gen_compdb.py         — compile_commands.json 辅助生成
├── Makefile
├── README.md
└── LICENSE
```

## 构建

```bash
# 编译 lib
make lib

# 语法检查所有头文件
make check

# 编译所有测试
make test

# 运行非交互测试
make -C test check
```

## 文档

| 文档 | 说明 |
|------|------|
| `tui-utils/MANUAL_zh.md` | 完整的中文手册（vaterm + vatui） |
| `doc/man/vatui.md` | vatui 用户手册 |
| `doc/dev/vaterm.md` | vaterm 实现思路 |
| `doc/dev/vatui.md` | vatui 高级设计 |
| `doc/dev/compile.md` | 编译指南 |

## 许可

MIT License. 详见 [LICENSE](LICENSE)。

---

<p align="center">
  <sub>
    Built by <a href="https://github.com/lc-3124">lc3124</a>
    · Design assisted by <b>DeepSeek</b>
  </sub>
</p>
