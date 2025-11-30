CC = gcc
CFLAGS = -Wall -g
BUILD_DIR = build
TARGET = $(BUILD_DIR)/distributed-message-queue
INCLUDE = -Iinclude
SOURCES = $(shell find . -name "*.c") 
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all debug clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

debug: $(TARGET)
	gdb $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
