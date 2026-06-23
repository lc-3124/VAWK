# =========================================================================
#  VAWK — Visual Ansi Widget Kit  (root Makefile)
#
#  Top-level build orchestrator.  Delegates test builds to test/ and
#  provides header-syntax-check for the header-only library modules.
#
#  Targets:
#    all             — print status (header-only, nothing to build here)
#    check           — syntax-check every .hpp with -fsyntax-only -Werror
#    test            — build all test/demo binaries (delegates to test/)
#    clean           — remove build artifacts
# =========================================================================

SHELL        := bash
CXX          := g++
CXX_STD      := --std=c++23
CXX_FLAGS    := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS    := -Iinclude -Itui-utils/include

.PHONY: all clean check test

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

# Build all test/demo programs.
test:
	$(MAKE) -C test all

clean:
	@rm -rf build
	$(MAKE) -C test clean
	@echo "Cleaned."
