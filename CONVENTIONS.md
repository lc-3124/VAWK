# VAWK 约定

简短约定，供贡献者与 AI 参照；不追求覆盖所有细节。

## 目录

- `include/` 是**唯一公共头根**（编译时统一 `-Iinclude`），含 `vaterm` / `vatui` / `vawk` 三个模块。
- `src/` 与 `include/` **镜像**：`include/a/b.hpp` 的实现是 `src/a/b.cpp`。
- `demo/` 下**每个一级子目录 = 一个独立演示项目**（各自带 Makefile）。
- `test/` 用例集中存放，**一个 Makefile 管全部**。
- `lib/`、`build/` 是构建产物，不入版本库。

## 命名

- 命名空间：`vaterm` / `vatui` / `vawk`；`core`、`widget` 是**目录**，不是命名空间。
- 类型：`PascalCase`；函数 / 变量：`snake_case`。
- 宏：`UPPER_SNAKE`。
- 文件名：`snake_case.hpp` / `snake_case.cpp`。
- 头卫兵：`UPPER_SNAKE_HPP`。
- 枚举定义集中在 `vaterm/enums.hpp`。

## 头与实现的分界

一句话判据：**函数体含分支 / 循环 / 查表 / syscall → 头出声明、cpp 出定义；纯转发或纯字符串拼装 → 留在头里 inline。**

- 头里只有声明，加上「1~3 行的纯转发」（例如 `fg(Rgb)` → `fg(r,g,b)`）。
- `cursor.hpp` 整体是纯 `\033[..` 拼串，按判据保持 header-only。
- 模板、宏、纯枚举**只能**留在头里（`vawk/core/event.hpp` 属此类）。
- 头必须**显式**声明自己用到的依赖（不要依赖别的头顺带传递，例如 `term.hpp` 直接 `#include "vaterm/enums.hpp"`）。

## 注释

- 统一用**中文**。
- 技术名词与标识符保留英文：ANSI、SGR、UTF-8、RGB、RAII、CSI 等。

## 构建

- **只有顶层 `Makefile`** 不写具体文件名、全自动发现；项目级 Makefile 可以点名文件。
- 库产物：`libvawk.a` + `libvawk.so`（对象一律 `-fPIC`）。
- demo / test 默认链接静态库 `.a`。

## 编译失败策略

- `src/`（库）失败即**整体失败**。
- `demo/`、`test/` 单个文件失败则**跳过**，并在末尾明确打印，不中断整体。

## 许可

- **GPL-3.0**：强 copyleft，下游链接 / 分发即需开源。
