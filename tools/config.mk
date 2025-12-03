# TODO: clean up

ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
MODULE := $(notdir $(CURDIR))

CC = gcc
CFLAGS = -I$(ROOT_DIR)/include -Wall -Wextra -Werror -pedantic -std=c11
CFLAGS += -D_POSIX_C_SOURCE=200809L # allows struct sigaction to compile
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
