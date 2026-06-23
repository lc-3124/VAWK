# 编译指南

## Makefile 体系

项目包含三层 Makefile，各司其职：

```
VAWK/
├── Makefile              # 顶层入口
├── test/
│   ├── Makefile          # 递归调度器，自动遍历子目录
│   └── utils/
│       └── Makefile      # 实际编译规则
```

### 1. 顶层 Makefile (`VAWK/Makefile`)

**作用**：项目唯一入口，提供统一命令。

| 目标 | 功能 |
|------|------|
| `make` / `make all` | 项目为 header-only，提示使用 `make check` |
| `make check` | 语法检查所有头文件（`-fsyntax-only -x c++-header`） |
| `make test` | 委托给 `test/`，编译所有测试程序 |
| `make clean` | 清理 `build/` + 委托 `test/` 清理 |

编译参数：
```makefile
CXX       := g++
CXX_STD   := --std=c++23
CXX_FLAGS := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS := -Iinclude -Itui-utils/include
```

### 2. 测试调度 Makefile (`VAWK/test/Makefile`)

**作用**：自动发现 `test/` 下所有子目录（`$(wildcard */.)`），将 `all/clean/check` 委托给各子目录。

```makefile
SUB_DIRS := $(wildcard */.)

$(SUB_DIRS):
    $(MAKE) -C $@ $(MAKECMDGOALS)
```

**扩展方式**：在 `test/` 下创建新子目录（如 `test/foo/`），放入自己的 Makefile，顶层自动拾取。

### 3. 测试程序 Makefile (`VAWK/test/utils/Makefile`)

**作用**：编译 5 个测试 demo。

| 目标 | 功能 |
|------|------|
| `make all` | 编译所有 5 个 demo |
| `make check` | 运行非交互式测试（sys_demo + utf_demo） |
| `make clean` | 删除编译产物 |

编译时头文件路径：
```makefile
INC_FLAGS := -I../../tui-utils/include
```

---

## 编译流程

### 日常开发

```bash
# 1. 语法检查所有头文件（修改头文件后必须执行）
make check

# 2. 编译所有测试（验证 .cpp 能正确链接头文件）
make test

# 3. 运行非交互式测试
make -C test check

# 4. 清理
make clean
```

### 运行交互式 Demo

```bash
# 颜色演示（管道安全）
./test/utils/color_demo | less -R

# 光标演示（需 tty）
./test/utils/cursor_demo

# 终端控制演示（需 tty）
./test/utils/term_demo
```

---

## compile_commands.json 生成

`compile_commands.json` 为 clangd 提供编译参数，实现代码补全、跳转、诊断。

### 使用 compiledb

项目使用 [compiledb](https://github.com/nickdiego/compiledb) 工具从 Makefile 构建日志生成数据库。

```bash
# 安装
pip install compiledb

# 生成（拦截一次完整构建）
compiledb make -C test all

# 追加头文件条目（使 clangd 可索引头文件）
python3 contrib/gen_compdb.py
```

### 文件结构

`compile_commands.json` 包含 21 个条目：

| 条目类型 | 数量 | 示例 |
|----------|------|------|
| 测试 .cpp | 5 | `test/utils/color_demo.cpp` |
| 框架头文件 | 10 | `include/vawk/event.hpp` |
| 工具头文件 | 6 | `tui-utils/include/vaterm/color.hpp` |

### clangd 配置

项目根目录已有 `.clangd` 配置（可选），clangd 自动读取 `compile_commands.json`。

验证 clangd 工作正常：
```bash
# 检查 clangd 能否解析头文件
clangd --check=include/vawk/event.hpp
```

### 重新生成

修改 Makefile 或新增源文件后需重新生成：

```bash
rm compile_commands.json
compiledb make -C test all
python3 contrib/gen_compdb.py
```

---

## 依赖

| 工具 | 用途 | 安装 |
|------|------|------|
| `g++` (≥14) | C++23 编译器 | `pacman -S gcc` |
| `make` | 构建系统 | `pacman -S make` |
| `compiledb` | 生成 compile_commands.json | `pip install compiledb` |
| `clangd` | LSP 语言服务器 | `pacman -S clang` |
