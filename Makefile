# VAGUI  :  Static lib(libVAGUI.a) and Shared lib(libVAGUI.so)

# lc3124@aliyun.com

# You should run this Makefile in console 
# Before load this Makfile , Please ensure:
# 1. g++(c++20) , sfml(ver >= 3.0) has been correctly installed 
# in your system
# 2. g++ , binutils , make , and bin of sfml etc. can be drectly
# called in your env PATH
# 3. pip is well installed and your internet connection is good
# when it first time to make compilecommand.json 
# 4. your terminal support colorful (ANSI 8bit) output ,and unicode 
# (if doesn't , it don't mean that compiling will be failed )


# Colors definition
RED          := \033[1;31m
GREEN        := \033[1;32m
YELLOW       := \033[1;33m
BLUE         := \033[1;34m
MAGENTA      := \033[1;35m
CYAN         := \033[1;36m
WHITE        := \033[1;37m
RESET        := \033[0m

# Basic configuration
SHELL        := bash
CXX          := g++
CXX_STD      := --std=c++20
BUILD_PREFIX := build
LIB_PREFIX   := lib
LIB_NAME     := VAGUI

# Directory configuration
INC_DIR      := inc
SRC_DIR      := src
DEMO_DIR     := demo
OBJ_DIR      := $(BUILD_PREFIX)/obj
LIB_DIR      := $(BUILD_PREFIX)/$(LIB_PREFIX)

# Compilation flags
CXX_FLAGS    := -Wall -Wextra -g -fPIC
INC_FLAGS    := -I. -I$(INC_DIR)/
SFML_LIBS    := -lsfml-system -lsfml-graphics -lsfml-window

# File collections
SRCS         := $(wildcard $(SRC_DIR)/*.cpp)
OBJS         := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
STATIC_LIB   := $(LIB_DIR)/lib$(LIB_NAME).a
DYNAMIC_LIB  := $(LIB_DIR)/lib$(LIB_NAME).so

# Default target: build both static and shared libraries
all: static dynamic
	@echo -e "$(GREEN)[✓] All targets built successfully$(RESET)"

# Static library target
static: $(STATIC_LIB)
	@echo -e "$(GREEN)[✓] Static library ready: $(YELLOW)$(STATIC_LIB)$(RESET)"

# Shared library target
dynamic: $(DYNAMIC_LIB)
	@echo -e "$(GREEN)[✓] Shared library ready: $(YELLOW)$(DYNAMIC_LIB)$(RESET)"

# Compile source files with colorful output
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo -e "$(BLUE)[⚙] Compiling $(MAGENTA)$<$(BLUE) -> $(CYAN)$@$(RESET)"
	@$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -c $< -o $@ && \
	echo -e "$(GREEN)[✓] Successfully compiled $(MAGENTA)$<$(RESET)" || \
	(echo -e "$(RED)[✗] Failed to compile $(MAGENTA)$<$(RESET)"; exit 1)

# Build static library
$(STATIC_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	@echo -e "$(BLUE)[⚙] Creating static library $(YELLOW)$@$(BLUE) from $(CYAN)$^$(RESET)"
	@ar rcs $@ $^ && \
	echo -e "$(GREEN)[✓] Static library created: $(YELLOW)$@$(RESET)" || \
	(echo -e "$(RED)[✗] Failed to create static library$(RESET)"; exit 1)

# Build shared library
$(DYNAMIC_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	@echo -e "$(BLUE)[⚙] Creating shared library $(YELLOW)$@$(BLUE) from $(CYAN)$^$(RESET)"
	@$(CXX) -shared $^ -o $@ $(SFML_LIBS) && \
	echo -e "$(GREEN)[✓] Shared library created: $(YELLOW)$@$(RESET)" || \
	(echo -e "$(RED)[✗] Failed to create shared library$(RESET)"; exit 1)

# Build all demos
DEMO_TARGETS := $(addprefix demo-,$(notdir $(wildcard $(DEMO_DIR)/*)))

demo: $(DEMO_TARGETS)
	@echo -e "$(GREEN)[✓] All demos built successfully$(RESET)"

demo-%:
	@echo -e "$(BLUE)[⚙] Building demo: $(CYAN)$*$(RESET)"
	@$(MAKE) -C $(DEMO_DIR)/$* && \
	echo -e "$(GREEN)[✓] Demo built: $(CYAN)$*$(RESET)" || \
	(echo -e "$(RED)[✗] Failed to build demo: $(CYAN)$*$(RESET)"; exit 1)

# Clean build artifacts
clean:
	@echo -e "$(YELLOW)[!] Cleaning build artifacts...$(RESET)"
	@rm -rf $(BUILD_PREFIX)
	@for dir in $(wildcard $(DEMO_DIR)/*); do \
		echo -e "$(BLUE)[⚙] Cleaning demo: $(CYAN)$$(basename $$dir)$(RESET)"; \
		$(MAKE) -C $$dir clean; \
	done
	@echo -e "$(GREEN)[✓] All build artifacts cleaned$(RESET)"

# check pip and compiledb and make compilecommand
define compiledb_check
	@if ! command -v "compiledb" &>/dev/null; then \
		echo -e "$(RED)[✗] compiledb not installed!$(RESET)"; \
		echo -e "$(YELLOW)[!] Attempting to install...$(RESET)"; \
		if ! command -v "pip" &>/dev/null; then \
			echo -e "$(RED)[✗] pip is also not installed, aborting!$(RESET)"; \
			exit 1; \
		else \
			pip install compiledb; \
		fi; \
		if ! command -v "compiledb" &>/dev/null; then \
			echo -e "$(RED)[✗] Installation failed!$(RESET)"; \
			exit 1; \
		else \
			echo -e "$(GREEN)[✓] compiledb installed successfully$(RESET)"; \
			compiledb make all; \
		fi; \
	else \
		echo -e "$(BLUE)[⚙] Generating compilation database...$(RESET)"; \
		compiledb make all; \
		echo -e "$(GREEN)[✓] Compilation database generated$(RESET)"; \
	fi
endef

mkcompilejson:
	$(call compiledb_check)

.PHONY: all static dynamic demo clean mkcompilejson $(DEMO_TARGETS)
