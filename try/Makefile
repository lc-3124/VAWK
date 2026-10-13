CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC
LDFLAGS := -L../tui-utils/build -lvaterm

# 包含路径
INCLUDES := -I../tui-utils/include

# 源文件与目标文件
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)
HEADERS := $(wildcard include/*.hpp)

# 目标
TARGETS := bin/input_reader_demo bin/combined_demo

# 默认目标
all: dirs $(TARGETS)

# 创建必要的目录
dirs:
	@mkdir -p build bin

# 编译目标文件
build/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 链接 input_reader_demo
bin/input_reader_demo: build/input_reader.o build/input_reader_demo.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 链接 combined_demo
bin/combined_demo: build/input_reader.o build/combined_demo.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 清理
clean:
	rm -rf build bin

# 伪目标
.PHONY: all dirs clean
