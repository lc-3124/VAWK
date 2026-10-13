# =========================================================================
#  test/tui/ — vatui 测试程序（自动检测所有 .cpp）
# =========================================================================

CXX        := g++
CXX_STD    := --std=c++23
CXX_FLAGS  := -Wall -Wextra -g -fPIC -Wno-missing-field-initializers \
              -fdiagnostics-color=always
INC_FLAGS  := -I../../include -I../../tui-utils/include
LIB_OBJ    := ../../build/bin.o/vatui.o ../../build/bin.o/input.o
LDFLAGS    := $(LIB_OBJ)

SRCS       := $(wildcard *.cpp)
TARGETS    := $(SRCS:.cpp=)

.PHONY: all clean check

all: $(TARGETS)

%: %.cpp $(LIB_OBJ)
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< $(LDFLAGS) -o $@

check: all
	@echo "tui tests require a tty; run manually: $(TARGETS)"

clean:
	rm -f $(TARGETS)
