OBJ := $(wildcard ../../build/obj/*.o)
SRCS := $(wildcard *.cpp)
TARGETS := $(patsubst %.cpp,build/%.run,$(SRCS))

all: $(TARGETS)

build/%.run: %.cpp | build
	@../../outInfo -Ct $< $@
	@g++ --std=c++23 -g -O0 -I. -I../../inc $< ../../build/lib/libVAUI.a  \
		-Werror -fdiagnostics-color=always \
		-o $@
	@../../outInfo -Co #$< $@

build:
	mkdir -p build

clean:
	rm -rf build
