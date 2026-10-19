CXX      ?= g++
CXXSTD   := --std=c++23
WARN     := -Wall -Wextra
HARDWARN := $(WARN) -Werror
CXXFLAGS ?= -g -fPIC
ROOT     ?= .
INCFLAGS := -I$(ROOT)/include

# 所有公共头，供 make check 逐个做 -fsyntax-only 检查。
HDRS := $(shell find $(ROOT)/include -name '*.hpp' 2>/dev/null)

# 自动头依赖：-MMD 让每个 .o 只依赖它真正 include 的头，-MP 生成防呆伪目标。
DEPFLAGS := -MMD -MP
