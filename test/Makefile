# =========================================================================
#  test/ — recursive Makefile
#
#  Automatically discovers all immediate subdirectories (e.g. utils/)
#  and forwards make targets (all, clean, check) to each one.
#
#  Each subdirectory must provide its own Makefile with at least:
#    all     — build everything
#    clean   — remove build artifacts
#    check   — run tests
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

# Graceful message when no subdirectories exist.
ifeq ($(strip $(SUB_DIRS)),)
all clean check:
	@echo "No test subdirectories found. Create a subdirectory with a Makefile."
endif
