CXX      ?= g++
CXXSTD   := --std=c++23
WARN     := -Wall -Wextra
HARDWARN := $(WARN) -Werror
CXXFLAGS ?= -g -fPIC
ROOT     ?= .
INCFLAGS := -I$(ROOT)/include
