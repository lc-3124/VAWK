LIB_SRCS := $(shell find $(ROOT)/src -name '*.cpp' 2>/dev/null)
LIB_OBJS := $(patsubst $(ROOT)/src/%.cpp,$(ROOT)/build/obj/src/%.o,$(LIB_SRCS))
LIB_A    := $(ROOT)/lib/libvawk.a
LIB_SO   := $(ROOT)/lib/libvawk.so
HDRS     := $(shell find $(ROOT)/include -name '*.hpp' 2>/dev/null)

$(LIB_A): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^
	@echo "  已生成 $@"

$(LIB_SO): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) -shared -o $@ $^
	@echo "  已生成 $@"

$(ROOT)/build/obj/src/%.o: $(ROOT)/src/%.cpp $(HDRS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXSTD) $(HARDWARN) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@
