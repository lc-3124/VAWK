# =========================================================================
#  test/tui/ — vatui test programs
# =========================================================================

CXX        := g++
CXX_STD    := --std=c++23
CXX_FLAGS  := -Wall -Wextra -g -fPIC -Werror -Wno-missing-field-initializers -fdiagnostics-color=always
INC_FLAGS  := -I../../include -I../../tui-utils/include
LIB_OBJ    := ../../build/bin.o/vatui.o
LDFLAGS    := $(LIB_OBJ)

TARGETS    := color print paint

.PHONY: all clean check

all: $(TARGETS)

color: color.cpp $(LIB_OBJ)
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< $(LDFLAGS) -o $@

print: print.cpp $(LIB_OBJ)
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< $(LDFLAGS) -o $@

paint: paint.cpp $(LIB_OBJ)
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< $(LDFLAGS) -o $@

check: all
	@echo "tui tests require a tty; run ./print manually."

clean:
	rm -f $(TARGETS)
