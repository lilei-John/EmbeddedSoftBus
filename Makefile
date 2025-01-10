# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include -pthread -D_WIN32

# Windows specific flags
LDFLAGS = -lws2_32

# 是否启用socket组播功能
ENABLE_SOCKET_MULTICAST ?= 1
ifeq ($(ENABLE_SOCKET_MULTICAST),1)
    CFLAGS += -DENABLE_SOCKET_MULTICAST=1
else
    CFLAGS += -DENABLE_SOCKET_MULTICAST=0
endif

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)\obj

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/message_queue.c \
       $(SRC_DIR)/softbus/device_manager.c \
       $(SRC_DIR)/softbus/rbtree.c \
       $(SRC_DIR)/softbus/softbus.c \
       $(SRC_DIR)/softbus/softbus_api.c \
       $(SRC_DIR)/softbus/softbus_socket.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Target executable
TARGET = $(BUILD_DIR)\softbus_demo.exe

# Default target
all: win32

# Windows target
win32: directories $(TARGET)

# Create build directories
directories:
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	@if not exist $(OBJ_DIR)\softbus mkdir $(OBJ_DIR)\softbus

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@if not exist $(dir $@) mkdir $(dir $@)
	$(CC) $(CFLAGS) -D_WIN32 -c $< -o $@

# Link object files
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Clean build files
.PHONY: clean
clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)

# Run the demo
.PHONY: run
run: all
	$(TARGET)

# Debug target
.PHONY: debug
debug:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        - Build the software bus system (default)"
	@echo "  clean      - Remove build files"
	@echo "  run        - Build and run the demo"
	@echo "  debug      - Show debug information"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Configuration options:"
	@echo "  ENABLE_SOCKET_MULTICAST=1|0  - Enable/disable socket multicast support (default: 1)"
