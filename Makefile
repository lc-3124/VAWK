# =========================================================================
#  VAWK — 顶层构建（全自动发现，不含具体文件名）
#    make        库 + 全部演示 + 头文件检查（不含测试）
#    make lib    lib/libvawk.a 与 lib/libvawk.so
#    make demo   全部演示项目（单个失败跳过）
#    make test   测试（单个失败跳过）
#    make check  语法检查所有公共头文件
#    make clean
# =========================================================================
SHELL := bash
ROOT  := $(CURDIR)

# mk/lib.mk 的库规则先于 all 出现，显式固定默认目标为 all。
.DEFAULT_GOAL := all
include mk/common.mk
include mk/lib.mk

DEMO_PROJECTS := $(patsubst %/Makefile,%,$(shell find demo -mindepth 2 -maxdepth 2 -name Makefile 2>/dev/null))

.PHONY: all lib demo test check clean

all: lib
	@$(MAKE) --no-print-directory demo
	@$(MAKE) --no-print-directory check
	@echo "== 全部完成（自动测试请单独执行）=="

lib: $(LIB_A) $(LIB_SO)

demo:
	@echo "== 构建演示程序开始 =="
	@skip=0; for d in $(DEMO_PROJECTS); do \
	  echo ">>> $$d"; \
	  $(MAKE) --no-print-directory -C $$d ROOT=$(ROOT) || { echo "  !! 跳过（编译失败）：$$d"; skip=1; }; \
	done; \
	if [ $$skip -eq 0 ]; then echo "== 演示全部构建成功 =="; else echo "== 有演示被跳过（见上）=="; fi; \
	exit 0

test:
	@$(MAKE) --no-print-directory -C test ROOT=$(ROOT)

check:
	@echo "== 头文件检查开始=="
	@for h in $(HDRS); do \
	  $(CXX) $(CXXSTD) $(HARDWARN) $(INCFLAGS) -fsyntax-only -x c++-header $$h || { echo "  FAIL $$h"; exit 1; }; \
	done
	@echo "== 头文件检查通过（共 $(words $(HDRS)) 个）=="

clean:
	@echo "== 清理开始 =="
	@rm -rf build lib
	@for d in $(DEMO_PROJECTS); do $(MAKE) --no-print-directory -C $$d clean >/dev/null 2>&1 || true; done
	@echo "== 已清理 build/ 与 lib/ =="
