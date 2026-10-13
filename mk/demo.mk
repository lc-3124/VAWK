NAME := $(notdir $(CURDIR))
OUT  := $(ROOT)/build/demo/$(NAME)
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(OUT)/%.o,$(SRCS))
BIN  := $(OUT)/$(NAME)

all: $(BIN)

$(BIN): $(OBJS) $(ROOT)/lib/libvawk.a
	@mkdir -p $(dir $@)
	$(CXX) $(CXXSTD) $(WARN) $(OBJS) $(ROOT)/lib/libvawk.a -o $@ -pthread
	@echo "  已生成 $@"

$(OUT)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXSTD) $(WARN) $(INCFLAGS) -c $< -o $@

clean:
	@rm -rf $(OUT)

.PHONY: all clean
