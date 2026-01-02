CC 	   := gcc
CFLAGS := -I../include \
		   -Wall -Wextra -Werror \
		   -pedantic \
		   -std=c11 \
		   -MMD -MP
CFLAGS += -D_GNU_SOURCE # allows pthread_kill()
CFLAGS += -DTHREADED 	# allows for zoo_create()

RELEASE_CFLAGS := -O2
DEBUG_CFLAGS   := -g -O0 -fno-inline -DDEBUG
TEST_CFLAGS    := -I.
LDFLAGS 	   := -L../lib -lmessageq -lzookeeper_mt

GDB := gdb

VALGRIND  	   := valgrind
VALGRIND_FLAGS := --leak-check=full \
				  --show-leak-kinds=all \
				  --track-origins=yes \
				  --trace-children=yes

TARGET 		 := partition
DEBUG_TARGET := debug_partition
TEST_TARGET  := test_partition \
				test_queue

OBJ 	   := main.o \
			  partition.o \
	   		  queue.o
DEBUG_OBJ := $(OBJ:%.o=debug_%.o)
TEST_OBJ  := $(filter-out test_main.o, $(OBJ:%.o=test_%.o))

DEPS := $(OBJ:.o=.d) $(DEBUG_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

GDB_TEST_TARGET      := $(TEST_TARGET:%=gdb-%)
VALGRIND_TEST_TARGET := $(TEST_TARGET:%=valgrind-%)

.DELETE_ON_ERROR:
.PHONY: all release debug test gdb $(GDB_TEST_TARGET) valgrind \
		$(VALGRIND_TEST_TARGET) format clean help

all: release
release: $(TARGET)

$(TARGET): $(OBJ)
	@$(CC) $^ -o $@ $(LDFLAGS) 

$(OBJ): CFLAGS += $(RELEASE_CFLAGS)
$(OBJ): %.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(DEBUG_TARGET)
$(DEBUG_TARGET): $(DEBUG_OBJ) 
	@$(CC) $^ -o $@ $(LDFLAGS) 

$(DEBUG_OBJ): CFLAGS += $(DEBUG_CFLAGS)
$(DEBUG_OBJ): debug_%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
$(TEST_TARGET): %: %.o $(filter-out debug_main.o, $(DEBUG_OBJ))
	@$(CC) $^ -o $@ $(LDFLAGS) 

$(TEST_OBJ): CFLAGS += $(TEST_CFLAGS) $(DEBUG_CFLAGS)
$(TEST_OBJ): %.o: tests/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

gdb: $(DEBUG_TARGET)
	@$(GDB) $<

$(GDB_TEST_TARGET): gdb-%: %
	@$(GDB) $<

valgrind: $(DEBUG_TARGET)
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$<

$(VALGRIND_TEST_TARGET): valgrind-%: %
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$<

format:
	@find . \( -name "*.c" -o -name "*.h" \) -exec clang-format -style='{BasedOnStyle: llvm, IndentWidth: 4}' -i {} +

clean:
	@rm -f $(TARGET) $(DEBUG_TARGET) $(TEST_TARGET)
	@rm -f $(OBJ) $(DEBUG_OBJ) $(TEST_OBJ)
	@rm -f $(DEPS)

help:
	@echo "Available targets:"
	@echo "	release       - Build release binary"
	@echo "	debug         - Build debug binary"
	@echo "	test          - Build test binary"
	@echo "	gdb           - Run GDB on debug binary"
	@echo "	gdb-test      - Run GDB on test binary"
	@echo "	valgrind      - Run Valgrind on debug binary"
	@echo "	valgrind-test - Run Valgrind on test binary"
	@echo "	clean         - Remove all build artifacts"
	@echo "	help          - Display this message"

-include $(DEPS)
