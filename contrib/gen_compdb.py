#!/usr/bin/env python3
"""Generate compile_commands.json entries for header-only projects.

compile_commands.json is used by clangd and other LSP servers to
determine compiler flags for each source file.

This script reads any existing compile_commands.json (e.g. produced by
compiledb from a Makefile build), then appends entries for the header-
only vaterm and vawk modules so that clangd can syntax-check headers
that are never compiled as standalone translation units.

Usage:
    compiledb make -C test all
    python3 contrib/gen_compdb.py
"""

import json, os, glob

# Project root (two levels up from this script).
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPDB = os.path.join(ROOT, "compile_commands.json")

# Common compiler flags for all header entries.
INC_PATHS = ["-Iinclude", "-Itui-utils/include"]
BASE_ARGS = [
    "g++", "--std=c++23",
    "-Wall", "-Wextra",
    "-fsyntax-only", "-x", "c++-header",
] + INC_PATHS

# Glob patterns for all headers that should have entries.
HEADER_PATTERNS = [
    "include/vawk.hpp",
    "include/vawk/*.hpp",
    "tui-utils/include/vaterm.hpp",
    "tui-utils/include/vaterm/*.hpp",
]

# --- Load any existing entries (from compiledb) ---
entries = []
try:
    with open(COMPDB) as f:
        entries = json.load(f)
except (FileNotFoundError, json.JSONDecodeError):
    pass

seen = {e["file"] for e in entries}

# --- Add missing header entries ---
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

# --- Write updated database ---
with open(COMPDB, "w") as f:
    json.dump(entries, f, indent=2)

print(f"compile_commands.json: {len(entries)} entries")
