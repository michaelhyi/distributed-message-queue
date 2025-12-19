CC 	   := gcc
CFLAGS := -I../include
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -pedantic
CFLAGS += -std=c11
CFLAGS += -MMD -MP

RELEASE_CFLAGS := -O3 -DNDEBUG
LDFLAGS 	 := -L../lib -l:lib.a

TARGET 			 := partition_client
OBJ    			 := partition_client.o
PARTITION_OBJ    := ../partition/partition.o ../partition/queue.o
DEPS   			 := $(OBJ:.o=.d)
-include $(DEPS)

.PHONY: all clean help

all: $(TARGET)
$(TARGET): %: %.o
	@$(CC) $< $(PARTITION_OBJ) -o $@ $(LDFLAGS)

$(OBJ): CFLAGS += $(RELEASE_CFLAGS)
$(OBJ): %.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(TARGET) $(OBJ) $(DEPS)

help:
	@echo "Available targets:"
	@echo "	all    - Build all sample executables"
	@echo "	clean  - Remove all build artifacts"
	@echo "	help   - Display this message"
