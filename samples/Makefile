TARGET 		  := partition_client
OBJ    		  := partition_client.o
PARTITION_OBJ := ../partition/partition.o ../partition/queue.o
DEPS 		  := partition_client.d

OVERRIDE_TARGET := 1
OVERRIDE_HELP   := 1

include ../tools/common.mk

.PHONY: all help

all: $(TARGET)
$(TARGET): %: %.o
	@$(CC) $< $(PARTITION_OBJ) -o $@ $(LDFLAGS) 

help:
	@echo "Available targets:"
	@echo "	all    - Build all sample executables"
	@echo "	clean  - Remove all build artifacts"
	@echo "	help   - Display this message"
