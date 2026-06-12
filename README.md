# VAWK — Visual Ansi Widget Kit

> **当前状态：聚焦 TUI 工具库开发，Widget 框架暂停。**

VAWK 是一个面向 Linux 终端的 TUI 工具包，提供底层终端控制能力和上层事件框架。

## 仓库结构

```
VAWK/
├── Tui-Utils/              # TUI 工具库（vaterm）
│   ├── include/
│   │   ├── vaterm.hpp        — 统一入口头文件
│   │   └── vaterm/
│   │       ├── enums.hpp     — 枚举定义（颜色、光标、特效等）
│   │       ├── color.hpp     — ANSI 颜色控制
│   │       ├── cursor.hpp    — 光标控制
│   │       ├── term.hpp      — 终端管理（Terminal 类）
│   │       ├── system.hpp    — 系统信息
│   │       └── utf.hpp       — UTF-8 处理
│   └── src/                  — 实现文件
├── inc/core/                # VAWK 核心框架头文件
│   ├── VaEntity.hpp          — 事件实体基类
│   ├── VaEvent.hpp           — 事件定义宏与工具
│   ├── VaEventRouter.hpp     — 事件路由器
│   ├── VaEventUpstream.hpp   — 事件上游分发器
│   └── VaInput.hpp           — 输入处理
└── src/                     # VAWK 核心框架实现
```

## 构建

```bash
make          # 编译库和 demo
make clean    # 清理
```

## 许可

MIT License.

