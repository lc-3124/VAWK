#这是VAWK的Makefile文件
#使用 make 来编译VAWK的动态链接库

#动态链接库相关变量声明

PROJECT_NAME = VAWK
CC = g++
CCFLAGS = -std=c++11 -Wall -Wextra -fPIC
INCLUDES = -Iinclude 

SRC_DIR = src
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_DIR = obj
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

LIB_DIR = lib
LIB_NAME = lib$(PROJECT_NAME).so
LIB_PATH = $(LIB_DIR)/$(LIB_NAME)

#obj规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@
#lib规则
$(LIB_PATH): $(OBJ_FILES)
	mkdir -p $(LIB_DIR)
	$(CC) -shared $(OBJ_FILES) -o $@

#启动编译
all: $(LIB_PATH)

#删光光 
clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(LIB_DIR)

#火车 
train:
	sl

.PHONY: all clean train

#关联更新，为了使我的lsp正常工作
$(OBJ_DIR)/Va_config.o: include/core/Va_config.hpp
$(OBJ_DIR)/Va.o: include/core/Va.hpp
$(OBJ_DIR)/VaDisplayingBuffer.o: include/core/display/VaDisplayingBuffer.hpp
$(OBJ_DIR)/VaEntity.o: include/core/VaEntity.hpp
$(OBJ_DIR)/VaMessage.o: include/core/handle/VaMessage.hpp
