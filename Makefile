# TODO: this is such a mess

CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -Werror -pedantic -std=c11
CFLAGS += -D_POSIX_C_SOURCE=200809L # allows struct sigaction to compile
C_OPT_FLAGS = -O3
C_TEST_FLAGS = -lcriterion
C_DEBUG_FLAGS = -O0 -g

GDB = gdb

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=yes

BUILD_DIR = build
TARGET = $(BUILD_DIR)/bin/run
TEST_TARGET := $(BUILD_DIR)/bin/test
DEBUG_TARGET := $(BUILD_DIR)/bin/debug

SRC = $(shell find src -name "*.c") 
TEST_SRC = $(shell find test -name "*.c") 

OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))
DEBUG_OBJ := $(patsubst %.c,$(BUILD_DIR)/debug/%.o,$(SRC))

.PHONY: test debug debug-test memcheck clean

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(TEST_TARGET): $(filter-out $(BUILD_DIR)/debug/src/main.o,$(DEBUG_OBJ)) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_TEST_FLAGS) $^ -o $(TEST_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_OBJ) -o $(DEBUG_TARGET)

$(BUILD_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_OPT_FLAGS) -c $< -o $@

$(BUILD_DIR)/test/%.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_DEBUG_FLAGS) -c $< -o $@

$(BUILD_DIR)/debug/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_DEBUG_FLAGS) -c $< -o $@

test: $(TEST_TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TEST_TARGET)

debug: $(DEBUG_TARGET)
	$(GDB) $(DEBUG_TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

memcheck: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

clean:
	@rm -rf $(BUILD_DIR)
