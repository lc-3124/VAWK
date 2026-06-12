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

> **当前状态：聚焦 TUI 工具库开发，Widget 框架暂停。**

---

## 📦 项目结构

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
├── Makefile
├── README.md
└── LICENSE
```

## 🔨 构建

```bash
make          # 当前为 header-only，仅提示
make check    # 验证所有头文件编译通过
make clean    # 清理
```

## 📜 许可

MIT License.

---

<p align="center">
  <sub>
    Built by <a href="https://github.com/lc-3124">lc3124</a>
    · Design assisted by <b>DeepSeek</b>
  </sub>
</p>

