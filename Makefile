# VAGUI  :  Static lib(libVAGUI.a) , Shared lib(libVAGUI.so) and demostration

# lc3124@aliyun.com

# You should run this Makefile in a posix-shell
# Before load this Makfile , Please ensure:
# 1. g++(c++23) 
# 2. g++ , binutils , make . can be drectly called in your terminal
# 3. your terminal support colorful (ANSI 8bit) output ,and unicode 
# 	(if doesn't , it don't mean that compiling will be failed  , however 
# 	it may makes your compiling outputs look like a shit.)
# 4. There is a bash script in program root "outInfo", 
# used to output compiling information don't touch it!


# Basic configuration
SHELL        := bash
CXX          := g++
CXX_STD      := --std=c++23
BUILD_PREFIX := build
LIB_PREFIX   := lib
LIB_NAME     := VAUI

# Directory configuration
INC_DIR      := inc
SRC_DIR      := src
OBJ_DIR      := $(BUILD_PREFIX)/obj
LIB_DIR      := $(BUILD_PREFIX)/$(LIB_PREFIX)
DEP_DIR      := $(BUILD_PREFIX)/dep
DEMO_DIR     := demo

# Compilation flags 
CXX_FLAGS    := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS    := -I. -I$(INC_DIR)/

# File collections
SRCS         := $(wildcard $(SRC_DIR)/*.cpp)
OBJS         := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS         := $(patsubst $(SRC_DIR)/%.cpp,$(DEP_DIR)/%.d,$(SRCS))
STATIC_LIB   := $(LIB_DIR)/lib$(LIB_NAME).a
DYNAMIC_LIB  := $(LIB_DIR)/lib$(LIB_NAME).so
DEMO_DIR     := demo
DEMO_SUBDIRS := $(wildcard $(DEMO_DIR)/*)
DEMO_MAKEFILES := $(wildcard \
                  $(DEMO_DIR)/*/Makefile \
                  $(DEMO_DIR)/*/makefile \
                  $(DEMO_DIR)/*/GNUmakefile)
DEMO_DIRS    := $(patsubst %/,%,$(sort $(dir $(DEMO_MAKEFILES))))
DEMO_TARGETS := $(patsubst $(DEMO_DIR)/%/Makefile,%,$(DEMO_MAKEFILES))
DEMO_TARGETS := $(patsubst $(DEMO_DIR)/%/makefile,%,$(DEMO_TARGETS))
DEMO_TARGETS := $(patsubst $(DEMO_DIR)/%/GNUmakefile,%,$(DEMO_TARGETS))

# including dependence , for triggering any re-compiling 
-include $(DEPS)

# Default target: build both static and shared libraries
all: static dynamic demo 
	@./outInfo -Ba

# Static library target
static: $(STATIC_LIB)

# Shared library target
dynamic: $(DYNAMIC_LIB)

# Demo building targets
demo: static dynamic demo_start $(DEMO_DIRS)
demo_start:	
	@echo $(DEMO_DIRS)
	@./outInfo -Bt DemoMakefiles

# Trigger Demo's Makefile 
$(DEMO_DIRS):
	@./outInfo -DBt $@
	@make -C $@ 
	@./outInfo -Bo $@
	@echo -e "\n"
    
# Compile source files 
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D) $(DEP_DIR)
	@./outInfo -Ct $< $@
	@$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@ && \
		./outInfo -Co  || \
		(./outInfo -Cf $<; exit 1)
    
# Build static library
$(STATIC_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	@./outInfo -Lt static $(STATIC_LIB)
	@ar rcs $@ $^ && \
		./outInfo -Lo static $@ || \
		(./outInfo -Lf static $@; exit 1)
    
# Build shared library 
$(DYNAMIC_LIB): $(OBJS)
	@mkdir -p $(LIB_DIR)
	@./outInfo -Lt shared $(DYNAMIC_LIB)
	@$(CXX) -shared $^ -o $@ && \
		./outInfo -Lo shared $@ || \
		(./outInfo -Lf shared $@; exit 1)

# Clean build artifacts
clean: clean-demo
	@./outInfo -Cc $(BUILD_PREFIX)
	@rm -rf $(BUILD_PREFIX)
	@./outInfo -Clo

clean-demo:
	@./outInfo -Cc "demo projects"
	@for target in $(DEMO_TARGETS); do \
		if [ -f $(DEMO_DIR)/$$target/Makefile ]; then \
		$(MAKE) -C $(DEMO_DIR)/$$target clean || true; \
		fi \
		done
	@./outInfo -Clo

# directories' dependening 
$(LIB_DIR):
	@mkdir -p $@
    
$(DEP_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

$(STATIC_LIB): | $(LIB_DIR)
$(DYNAMIC_LIB): | $(LIB_DIR)
$(OBJS): | $(OBJ_DIR) $(DEP_DIR)

# PHONY
.PHONY: all static dynamic clean clean-demo demo $(DEMO_DIRS)
