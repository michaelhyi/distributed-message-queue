CC = gcc
CFLAGS = -Wall -g
INCLUDE = -Iinclude

GDB = gdb

VALGRIND = valgrind
VALGRIND_FLAGS = --leak-check=yes

BUILD_DIR = build
TARGET = $(BUILD_DIR)/distributed-message-queue
TEST_TARGET = $(BUILD_DIR)/test-distributed-message-queue

SOURCES = $(shell find src -name "*.c") 
TEST_SOURCES = $(shell find test -name "*.c") 

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
TEST_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

.PHONY: all test debug clean

all: $(TARGET)

test: $(TEST_TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(TEST_TARGET): $(OBJECTS) $(TEST_OBJECTS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

debug: $(TARGET)
	$(GDB) $(TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

memcheck: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

memcheck-test: $(TEST_TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR)
