# =========================================================================
#  test/utils/ — vaterm demo programs
#
#  Builds and runs every non-interactive demo; interactive demos
#  (cursor_demo, term_demo) are compiled but skipped in batch check.
#
#  Targets:
#    all     — compile all 6 demos
#    check   — run sys_demo, utf_demo, mouse_demo (non-interactive)
#    clean   — remove binaries
# =========================================================================

CXX       := g++
CXX_STD   := --std=c++23
CXX_FLAGS := -Wall -Wextra -g -fPIC -Werror -fdiagnostics-color=always
INC_FLAGS := -I../../tui-utils/include

TARGETS   := color_demo cursor_demo term_demo sys_demo utf_demo mouse_demo

.PHONY: all clean check

all: $(TARGETS)

color_demo: color_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

cursor_demo: cursor_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

term_demo: term_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

sys_demo: sys_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

utf_demo: utf_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

mouse_demo: mouse_demo.cpp
	$(CXX) $(CXX_STD) $(CXX_FLAGS) $(INC_FLAGS) $< -o $@

check: all
	@echo "Running sys_demo..."
	./sys_demo
	@echo "Running utf_demo..."
	./utf_demo
	@echo "Running mouse_demo..."
	./mouse_demo
	@echo "Terminal/cursor demos require a tty; skipped in batch check."
	@echo "To run interactive demos:"
	@echo "  cd test/utils && ./color_demo | less -R"
	@echo "  cd test/utils && ./cursor_demo"
	@echo "  cd test/utils && ./term_demo"

clean:
	rm -f $(TARGETS)
