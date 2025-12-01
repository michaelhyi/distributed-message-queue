CC = gcc
CFLAGS = -Iinclude -O3 -Wall -Wextra -pedantic -Werror -g -std=c11
CFLAGS += -D_POSIX_C_SOURCE=200809L # allows struct sigaction to compile

GDB = gdb

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=yes

BUILD_DIR = build

SRC = $(shell find src -name "*.c") 
TEST_SRC = $(shell find test -name "*.c") 

OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

TARGET = $(BUILD_DIR)/distributed-message-queue
TEST_TARGET := $(patsubst %.c,$(BUILD_DIR)/%,$(TEST_SRC))

.PHONY: test debug debug-test memcheck clean

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test/%: $(OBJ) $(BUILD_DIR)/test/%.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

# TODO: test ouput sucks
test: $(TEST_TARGET)
	@status=0; \
	for test in $^; do \
		if timeout 5s $$test && $(VALGRIND) $(VALGRIND_FLAGS) $$test; then \
			echo "$$test passed"; \
		else \
			echo "$$test failed or timed out"; \
			status=1; \
		fi; \
	done; \
	exit $$status

debug: $(TARGET)
	$(GDB) $(TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

memcheck: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

clean:
	@rm -rf $(BUILD_DIR)
