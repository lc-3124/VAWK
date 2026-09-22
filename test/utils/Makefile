# =========================================================================
#  test/utils/ — vaterm demo programs
#
#  Targets:
#    all     — compile all demos
#    check   — run sys_demo, utf_demo (non-interactive)
#    clean   — remove binaries
# =========================================================================

CXX       := g++
CXX_STD   := --std=c++23
CXX_FLAGS := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS := -I../../tui-utils/include

TARGETS   := color_demo sys_demo utf_demo

.PHONY: all clean check

all: $(TARGETS)

color_demo: color_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

sys_demo: sys_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

utf_demo: utf_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

check: all
	@echo "Running sys_demo..."
	./sys_demo
	@echo "Running utf_demo..."
	./utf_demo
	@echo "To view colour output: cd test/utils && ./color_demo | less -R"

clean:
	rm -f $(TARGETS)
