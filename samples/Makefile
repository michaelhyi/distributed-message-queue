ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
GLOBAL_INCLUDE_DIR := $(ROOT_DIR)/include

CC = gcc
CFLAGS = -I$(GLOBAL_INCLUDE_DIR) -Wall -Wextra -Werror -pedantic -std=c11
C_OPT_FLAGS = -O3

GDB = gdb

BUILD_DIR = build

SRC = $(shell find . -name "*.c") 

OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
LIB_OBJ := $(shell find $(ROOT_DIR)/lib/build/src -name "*.o")
PARTITION_OBJ := $(filter-out $(ROOT_DIR)/partition/$(BUILD_DIR)/src/main.o,$(shell find $(ROOT_DIR)/partition/build/src -name "*.o"))

BIN := $(patsubst %.c,$(BUILD_DIR)/bin/%,$(SRC))

.PHONY: clean

# link src obj files
$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) $(LIB_OBJ) $(PARTITION_OBJ) -o $(BIN)

# compile src files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_OPT_FLAGS) -c $< -o $@

clean:
	rm -rf build
