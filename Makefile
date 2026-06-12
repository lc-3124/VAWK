# VAWK  —  Makefile
#
# Current state: header-only (no .cpp files to compile).
# The Makefile is kept for future use when implementations are added.

SHELL        := bash
CXX          := g++
CXX_STD      := --std=c++23
CXX_FLAGS    := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS    := -Iinclude -Itui-utils/include

# ── Targets ────────────────────────────────────────────────────────────────

.PHONY: all clean check

all:
	@echo "VAWK is currently header-only. Nothing to build."
	@echo "Use 'make check' to verify header compilation."

# Compile-check every header to catch syntax errors.
check:
	@echo "Checking headers..."
	@for hdr in include/vawk.hpp include/vawk/*.hpp tui-utils/include/vaterm.hpp \
	            tui-utils/include/vaterm/*.hpp; do \
	    echo "  $$hdr"; \
	    $(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -fsyntax-only -x c++-header "$$hdr" || \
	        { echo "FAIL: $$hdr"; exit 1; }; \
	done
	@echo "All headers OK."

clean:
	@rm -rf build
	@echo "Cleaned."
