ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
GLOBAL_INCLUDE_DIR := $(ROOT_DIR)/include
MODULE := $(notdir $(CURDIR))

CC = gcc
CFLAGS = -I$(GLOBAL_INCLUDE_DIR) -Iinclude -Wall -Wextra -Werror -pedantic -std=c11

C_OPT_FLAGS = -O3
C_TEST_FLAGS = -lcriterion
C_DEBUG_FLAGS = -O0 -g

GDB = gdb

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=yes

BUILD_DIR = build
TARGET := $(BUILD_DIR)/bin/$(MODULE)
TEST_TARGET := $(BUILD_DIR)/bin/test
DEBUG_TARGET := $(BUILD_DIR)/bin/debug

SRC = $(shell find src -name "*.c") 
TEST_SRC = $(shell find test -name "*.c") 

OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))
DEBUG_OBJ := $(patsubst %.c,$(BUILD_DIR)/debug/%.o,$(SRC))
LIB_OBJ := $(shell find $(ROOT_DIR)/lib/build/src -name "*.o")
