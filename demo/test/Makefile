OBJ := $(wildcard ../../build/obj/*.o)
SRCS := $(wildcard *.cpp)
TARGETS := $(patsubst %.cpp,build/%.run,$(SRCS))

all: $(TARGETS)

build/%.run: %.cpp | build
	g++ --std=c++20 -I. -I../../inc $< $(OBJ) -o $@

build:
	mkdir -p build

clean:
	rm -rf build
