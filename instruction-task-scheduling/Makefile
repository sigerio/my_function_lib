# ============================================================
# 任务池调度系统 Makefile
# ============================================================

CC ?= cc
CFLAGS_BASE := -std=c11 -Wall -Wextra -O2
CFLAGS_DEBUG := -g -DDEBUG

# 目录结构
INC_DIR := Usr/include
SRC_DIR := Usr/src
TEST_DIR := Usr/test
BUILD_DIR := Usr/build

# 源文件
SRCS_CORE := \
	$(SRC_DIR)/task_sync.c \
	$(SRC_DIR)/task_sync_baremetal.c \
	$(SRC_DIR)/task_sync_pthread.c \
	$(SRC_DIR)/task_pool.c \
	$(SRC_DIR)/task_api_compat.c

SRCS_TEST := \
	$(TEST_DIR)/task_test_api.c \
	$(TEST_DIR)/task_test_main.c \
	$(TEST_DIR)/task_tdd_cases.c

# 目标文件
OBJS_CORE := $(SRCS_CORE:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJS_TEST := $(SRCS_TEST:$(TEST_DIR)/%.c=$(BUILD_DIR)/test_%.o)

# ============================================================
# 编译模式选择
# ============================================================

# 默认：裸机模式
SYNC_IMPL ?= BAREMETAL

# pthread 模式：make SYNC_IMPL=PTHREAD
ifeq ($(SYNC_IMPL),PTHREAD)
	CFLAGS := $(CFLAGS_BASE) -pthread -DTASK_SYNC_IMPL_PTHREAD
	TARGET := $(BUILD_DIR)/task_pool_pthread
else
	CFLAGS := $(CFLAGS_BASE)
	TARGET := $(BUILD_DIR)/task_pool_baremetal
endif

# ============================================================
# 规则定义
# ============================================================

.PHONY: all build pthread baremetal run run_rtos clean rebuild debug help info

all: build

# 构建主程序（测试入口）
build: $(TARGET)

# pthread 模式
pthread: clean
	$(MAKE) SYNC_IMPL=PTHREAD build

# 裸机模式（默认）
baremetal: clean
	$(MAKE) SYNC_IMPL=BAREMETAL build

# 运行裸机版本
run: build
	$(TARGET)

# 运行 RTOS(pthread) 版本
run_rtos:
	$(MAKE) SYNC_IMPL=PTHREAD build
	$(BUILD_DIR)/task_pool_pthread

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 核心目标文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# 测试目标文件
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(TEST_DIR) -c $< -o $@

# 链接主程序
$(TARGET): $(OBJS_CORE) $(OBJS_TEST) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 重新构建
rebuild: clean build

# 调试模式
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: rebuild

# ============================================================
# 辅助信息
# ============================================================

help:
	@echo "任务池调度系统 Makefile"
	@echo ""
	@echo "使用方法:"
	@echo "  make              - 构建默认模式（裸机）"
	@echo "  make pthread      - 构建 pthread 模式"
	@echo "  make baremetal    - 构建裸机模式"
	@echo "  make run          - 运行裸机版本"
	@echo "  make run_rtos     - 运行 RTOS(pthread) 版本"
	@echo "  make clean        - 清理构建目录"
	@echo "  make rebuild      - 重新构建"
	@echo "  make debug        - 调试模式构建"
	@echo "  make info         - 显示配置信息"

info:
	@echo "编译器:    $(CC)"
	@echo "编译选项:  $(CFLAGS)"
	@echo "同步实现:  $(SYNC_IMPL)"
	@echo "输出目录:  $(BUILD_DIR)"
	@echo "目标文件:  $(TARGET)"
