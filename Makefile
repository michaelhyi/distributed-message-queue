CC = gcc
# TODO: shouldn't optimize when trying to debug. separate flags when debugging
CFLAGS = -Iinclude -O3 -Wall -Wextra -pedantic -Werror -g -std=c11
CFLAGS += -D_POSIX_C_SOURCE=200809L # allows struct sigaction to compile
TEST_CFLAGS = -lcriterion

GDB = gdb

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=yes

BUILD_DIR = build
TARGET = $(BUILD_DIR)/run
TEST_TARGET := $(BUILD_DIR)/test-run

SRC = $(shell find src -name "*.c") 
TEST_SRC = $(shell find test -name "*.c") 

OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

.PHONY: test debug debug-test memcheck clean

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(filter-out $(BUILD_DIR)/src/main.o,$(OBJ)) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) $^ -o $(TEST_TARGET)

$(BUILD_DIR)/test/%.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

debug: $(TARGET)
	$(GDB) $(TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

memcheck: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

clean:
	@rm -rf $(BUILD_DIR)
