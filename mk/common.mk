CXX      ?= g++
CXXSTD   := --std=c++23
WARN     := -Wall -Wextra
HARDWARN := $(WARN) -Werror
CXXFLAGS ?= -g -fPIC
ROOT     ?= .
INCFLAGS := -I$(ROOT)/include

# 自动头依赖：-MMD 让每个 .o 只依赖它真正 include 的头，-MP 生成防呆伪目标。
DEPFLAGS := -MMD -MP
