CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC
LDFLAGS := -L../tui-utils/build -lvaterm

# Include paths
INCLUDES := -I../tui-utils/include

# Source and object files
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)
HEADERS := $(wildcard include/*.hpp)

# Targets
TARGETS := bin/input_reader_demo bin/combined_demo

# Default target
all: dirs $(TARGETS)

# Create necessary directories
dirs:
	@mkdir -p build bin

# Compile object files
build/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link input_reader_demo
bin/input_reader_demo: build/input_reader.o build/input_reader_demo.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Link combined_demo
bin/combined_demo: build/input_reader.o build/combined_demo.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Clean
clean:
	rm -rf build bin

# Phony targets
.PHONY: all dirs clean
