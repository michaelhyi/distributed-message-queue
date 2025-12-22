CC 	   := gcc
CFLAGS := -I../include
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -pedantic
CFLAGS += -std=c11
CFLAGS += -MMD -MP

RELEASE_CFLAGS := -O2 -DNDEBUG
DEBUG_CFLAGS   := -g -O0 -fno-inline -DDEBUG
TEST_CFLAGS    := -I.

ifndef OVERRIDE_LDFLAGS
LDFLAGS 	   := -L../lib -l:lib.a
endif

TEST_LDFLAGS   := -lcriterion

AR 		  	   := ar
AR_FLAGS  	   := rcs

GDB_SERVER 		 := gdbserver
GDB_SERVER_FLAGS := localhost:1234
GDB 		   	 := gdb
GDB_FLAGS 	     := -ex "target remote localhost:1234"
GDB_TEST_FLAGS 	 := -x ../tools/test.gdb

VALGRIND  	   := valgrind
VALGRIND_FLAGS := --leak-check=full
VALGRIND_FLAGS += --show-leak-kinds=all
VALGRIND_FLAGS += --track-origins=yes
VALGRIND_FLAGS += --trace-children=yes

TEST_DIR 		 := tests
TEST_DEBUG_FLAGS := --debug=gdb

.DELETE_ON_ERROR:
.PHONY: all release debug test gdb-server gdb gdb-server-test gdb-test \
		valgrind valgrind-test clean help

all: release
release: $(TARGET)

ifndef OVERRIDE_TARGET
$(TARGET): $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS) 
endif

$(OBJ): CFLAGS += $(RELEASE_CFLAGS)
$(OBJ): %.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(DEBUG_TARGET)
$(DEBUG_TARGET): $(DEBUG_MAIN_OBJ) $(DEBUG_OBJ) 
	@$(CC) $(DEBUG_MAIN_OBJ) $(DEBUG_OBJ) -o $(DEBUG_TARGET) $(LDFLAGS) 

$(DEBUG_MAIN_OBJ): CFLAGS += $(DEBUG_CFLAGS)
$(DEBUG_MAIN_OBJ): debug_%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

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

gdb-server: $(DEBUG_TARGET)
	@$(GDB_SERVER) $(GDB_SERVER_FLAGS) $(DEBUG_TARGET)

gdb: $(DEBUG_TARGET)
	@$(GDB) $(GDB_FLAGS) $(DEBUG_TARGET)

gdb-server-test: $(TEST_TARGET)
	@./$(TEST_TARGET) $(TEST_DEBUG_FLAGS)

gdb-test: $(TEST_TARGET)
	@$(GDB) $(TEST_TARGET) $(GDB_TEST_FLAGS)

valgrind: $(DEBUG_TARGET)
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(DEBUG_TARGET)

valgrind-test: $(TEST_TARGET)
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(TEST_TARGET)

clean:
	@rm -f $(OBJ) $(DEBUG_OBJ) $(TEST_OBJ)
	@rm -f $(TARGET) $(DEBUG_TARGET) $(TEST_TARGET)
	@rm -f $(DEPS)

ifndef OVERRIDE_HELP
help:
	@echo "Available targets:"
	@echo "	release           - Build release executable"
	@echo "	debug             - Build debug objects"
	@echo "	test              - Build test executable"
	@echo "	gdb-server        - Start GDB server for executable"
	@echo "	gdb               - Attach to GDB server for executable"
	@echo "	gdb-server-test   - Start GDB server for tests"
	@echo "	gdb-test          - Attach to GDB server for tests"
	@echo "	valgrind          - Run executable under Valgrind"
	@echo "	valgrind-test     - Run tests under Valgrind"
	@echo "	clean             - Remove all build artifacts"
	@echo "	help              - Display this message"
endif

-include $(DEPS)
