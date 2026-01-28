CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -pthread
SRC_DIR := Usr
BUILD_DIR := Usr/build
TARGET := $(BUILD_DIR)/app
SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/task_api.c $(SRC_DIR)/task_test.c
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all build run rebuild clean

all: build

build: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: build
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build
