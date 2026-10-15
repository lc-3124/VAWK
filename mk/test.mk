TEST_SRCS := $(shell find $(ROOT)/test -name '*.cpp' 2>/dev/null)
TEST_OUT  := $(ROOT)/build/test

test:
	@echo "== 自动测试程序构建开始 =="
	@mkdir -p $(TEST_OUT)
	@if [ -z "$(TEST_SRCS)" ]; then echo "  （test/ 下暂无用例）"; exit 0; fi; \
	 skip=0; for s in $(TEST_SRCS); do \
	   n=$$(basename $$s .cpp); \
	   echo "  [test] $$n"; \
	   $(CXX) $(CXXSTD) $(WARN) $(INCFLAGS) $$s $(ROOT)/lib/libvawk.a -o $(TEST_OUT)/$$n -pthread \
	     || { echo "     !! 跳过（编译失败）：$$n"; skip=1; }; \
	 done; \
	 if [ $$skip -eq 0 ]; then echo "  == 测试全部构建成功 =="; else echo "  == 有测试被跳过（见上）=="; fi

clean:
	@rm -rf $(TEST_OUT)

.PHONY: test clean
