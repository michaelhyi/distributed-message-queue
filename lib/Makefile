CC 	   := gcc
CFLAGS := -I../include
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -pedantic
CFLAGS += -std=c11
CFLAGS += -MMD -MP
CFLAGS += -D_GNU_SOURCE # allows include <endian.h>

RELEASE_CFLAGS := -O3 -DNDEBUG
DEBUG_CFLAGS   := -g -O0 -fno-inline -DDEBUG
TEST_CFLAGS    := -I.
LDFLAGS 	 :=
TEST_LDFLAGS := -lcriterion

AR 		  	   := ar
AR_FLAGS  	   := rcs
GDB 		   := gdb
GDB_FLAGS 	   := -x ../tools/criterion.gdb
VALGRIND  	   := valgrind
VALGRIND_FLAGS := --leak-check=full
VALGRIND_FLAGS += --show-leak-kinds=all
VALGRIND_FLAGS += --track-origins=yes
VALGRIND_FLAGS += --trace-children=yes

TEST_DIR 		 := tests
TARGET 			 := lib.a
TEST_TARGET 	 := test_lib
TEST_DEBUG_FLAGS := --debug=gdb

OBJ 	  := dmqp.o doubly_linked_list.o network.o
DEBUG_OBJ := debug_dmqp.o debug_doubly_linked_list.o debug_network.o
TEST_OBJ  := test_dmqp.o test_doubly_linked_list.o test_integration_dmqp.o \
			 test_integration_network.o test_network.o

DEPS := $(OBJ:.o=.d) $(DEBUG_OBJ:.o=.d) $(TEST_OBJ:.o=.d)
-include $(DEPS)

.PHONY: all release debug test gdb-server-test gdb-test valgrind-test clean help

all: release
release: $(TARGET)
$(TARGET): $(OBJ)
	@$(AR) $(AR_FLAGS) $(TARGET) $(OBJ)

$(OBJ): CFLAGS += $(RELEASE_CFLAGS)
$(OBJ): %.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(DEBUG_OBJ)
$(DEBUG_OBJ): CFLAGS += $(DEBUG_CFLAGS)
$(DEBUG_OBJ): debug_%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
$(TEST_TARGET): LDFLAGS += $(TEST_LDFLAGS)
$(TEST_TARGET): $(TEST_OBJ) $(DEBUG_OBJ)
	@$(CC) $(TEST_OBJ) $(DEBUG_OBJ) -o $(TEST_TARGET) $(LDFLAGS) 

$(TEST_OBJ): CFLAGS += $(TEST_CFLAGS) $(DEBUG_CFLAGS)
$(TEST_OBJ): %.o: $(TEST_DIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

gdb-server-test: $(TEST_TARGET)
	@./$(TEST_TARGET) $(TEST_DEBUG_FLAGS)

gdb-test: $(TEST_TARGET)
	@$(GDB) ./$(TEST_TARGET) $(GDB_FLAGS)

valgrind-test: $(TEST_TARGET)
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(TEST_TARGET)

clean:
	@rm -f $(OBJ) $(DEBUG_OBJ) $(TEST_OBJ)
	@rm -f $(TARGET) $(TEST_TARGET)
	@rm -f $(DEPS)

help:
	@echo "Available targets:"
	@echo "	release           - Build release library"
	@echo "	debug             - Build debug objects"
	@echo "	test              - Build test executable"
	@echo "	gdb-server-test   - Start GDB server for tests"
	@echo "	gdb-test          - Attach to GDB server for tests"
	@echo "	valgrind-test     - Run tests under Valgrind"
	@echo "	clean             - Remove all build artifacts"
	@echo "	help              - Display this message"
