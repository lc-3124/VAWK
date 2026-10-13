# =========================================================================
#  VAWK — Visual Ansi Widget Kit  （根 Makefile）
#
#  目标：
#    all             — 打印状态
#    check           — 用 -fsyntax-only -Werror 对所有 .hpp 做语法检查
#    lib             — 编译 src/utils/vatui.cpp → build/bin.o/vatui.o
#    test            — 构建所有测试/演示程序（委派给 test/）
#    clean           — 删除构建产物
# =========================================================================

SHELL        := bash
CXX          := g++
CXX_STD      := --std=c++23
CXX_FLAGS    := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS    := -Iinclude -Itui-utils/include

.PHONY: all clean check test lib

all:
	@echo "Use 'make lib' to build the vatui library object."
	@echo "Use 'make check' to verify header compilation."
	@echo "Use 'make test' to build all test programs."

# 编译检查每个头文件以捕获语法错误。
check:
	@echo "Checking headers..."
	@for hdr in include/vawk.hpp include/vawk/*.hpp \
	            tui-utils/include/vaterm.hpp \
	            tui-utils/include/vaterm/*.hpp \
	            tui-utils/include/vatui.hpp; do \
	    echo "  $$hdr"; \
	    $(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -fsyntax-only -x c++-header "$$hdr" || \
	        { echo "FAIL: $$hdr"; exit 1; }; \
	done
	@echo "All headers OK."

# 构建 vatui 库对象。
lib: build/bin.o/vatui.o build/bin.o/entity.o build/bin.o/event_upstream.o build/bin.o/event_router.o build/bin.o/input.o

build/bin.o/vatui.o: src/utils/vatui.cpp tui-utils/include/vatui.hpp
	@mkdir -p build/bin.o
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -c $< -o $@

build/bin.o/input.o: src/vaterm/input.cpp tui-utils/include/vaterm/input.hpp
	@mkdir -p build/bin.o
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -c $< -o $@

build/bin.o/entity.o: src/vawk/entity.cpp include/vawk/entity.hpp include/vawk/event_upstream.hpp
	@mkdir -p build/bin.o
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -c $< -o $@

build/bin.o/event_upstream.o: src/vawk/event_upstream.cpp include/vawk/event_upstream.hpp
	@mkdir -p build/bin.o
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -c $< -o $@

build/bin.o/event_router.o: src/vawk/event_router.cpp include/vawk/event_router.hpp
	@mkdir -p build/bin.o
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -c $< -o $@

# 构建所有测试/演示程序。
test: lib
	$(MAKE) -C test all

clean:
	@rm -rf build/bin.o
	$(MAKE) -C test clean
	@echo "Cleaned."
