CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -pedantic -Werror -std=c11
CFLAGS += -D_POSIX_C_SOURCE=200809L # allows struct sigaction to compile
OPT_CFLAGS = -O3
TEST_CFLAGS = -lcriterion
DEBUG_CFLAGS = -O0 -g

GDB = gdb

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=yes

BUILD_DIR = build
TARGET = $(BUILD_DIR)/run
TEST_TARGET := $(BUILD_DIR)/test-run
DEBUG_TARGET := $(BUILD_DIR)/debug

SRC = $(shell find src -name "*.c") 
TEST_SRC = $(shell find test -name "*.c") 

OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

.PHONY: test debug debug-test memcheck clean

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OPT_CFLAGS) $(OBJ) -o $(TARGET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(filter-out $(BUILD_DIR)/src/main.o,$(OBJ)) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $(DEBUG_CFLAGS) $^ -o $(TEST_TARGET)

$(BUILD_DIR)/test/%.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

$(DEBUG_TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(OBJ) -o $(DEBUG_TARGET)

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
