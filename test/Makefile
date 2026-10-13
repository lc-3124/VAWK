# =========================================================================
#  test/ — 递归 Makefile
#
#  自动发现所有直接子目录（例如 utils/），
#  并把 make 目标（all、clean、check）转发给每个子目录。
#
#  每个子目录必须提供自己的 Makefile，至少包含：
#    all     — 构建全部
#    clean   — 删除构建产物
#    check   — 运行测试
# =========================================================================

SUB_DIRS := $(wildcard */.)

.PHONY: all clean check $(SUB_DIRS)

all: $(SUB_DIRS)
	@echo "All test subdirectories built."

$(SUB_DIRS):
	@echo ">>> Entering $@"
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: $(SUB_DIRS)
	@echo "All test subdirectories cleaned."

check: $(SUB_DIRS)
	@echo "All test checks passed."

# 当没有任何子目录时给出的友好提示。
ifeq ($(strip $(SUB_DIRS)),)
all clean check:
	@echo "No test subdirectories found. Create a subdirectory with a Makefile."
endif
