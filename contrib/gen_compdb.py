#!/usr/bin/env python3
"""为 header-only 项目生成 compile_commands.json 条目。

compile_commands.json 供 clangd 及其他 LSP 服务器用来
确定每个源文件的编译选项。

本脚本会读取现有的 compile_commands.json（例如由
compiledb 从 Makefile 构建生成的结果），然后为 header-only 的
vaterm 与 vawk 模块追加条目，使 clangd 能够对
那些从不作为独立翻译单元编译的头文件做语法检查。

用法：
    compiledb make -C test all
    python3 contrib/gen_compdb.py
"""

import json, os, glob

# 项目根目录（从本脚本向上两级）。
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPDB = os.path.join(ROOT, "compile_commands.json")

# 所有头文件条目共用的编译选项。
INC_PATHS = ["-Iinclude"]
BASE_ARGS = [
    "g++", "--std=c++23",
    "-Wall", "-Wextra",
    "-fsyntax-only", "-x", "c++-header",
] + INC_PATHS

# 应为哪些头文件生成条目所使用的 glob 模式。
HEADER_PATTERNS = [
    "include/vatui.hpp",
    "include/vaterm.hpp",
    "include/vaterm/*.hpp",
    "include/vawk.hpp",
    "include/vawk/core/*.hpp",
    "include/vawk/widget/*.hpp",
]

# --- 加载已有条目（来自 compiledb）---
entries = []
try:
    with open(COMPDB) as f:
        entries = json.load(f)
except (FileNotFoundError, json.JSONDecodeError):
    pass

seen = {e["file"] for e in entries}

# --- 补充缺失的头文件条目 ---
for pat in HEADER_PATTERNS:
    for hdr in sorted(glob.glob(os.path.join(ROOT, pat))):
        rel = os.path.relpath(hdr, ROOT)
        if rel not in seen:
            entries.append({
                "directory": ROOT,
                "arguments": BASE_ARGS + [rel],
                "file": rel,
            })
            seen.add(rel)

# --- 写出更新后的数据库 ---
with open(COMPDB, "w") as f:
    json.dump(entries, f, indent=2)

print(f"compile_commands.json: {len(entries)} entries")
