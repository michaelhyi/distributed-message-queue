CC 	   := gcc
CFLAGS := -I../include \
		  -Wall -Wextra -Werror \
		  -pedantic \
		  -std=c11 \
		  -MMD -MP
CFLAGS += -D_GNU_SOURCE # allows usleep(), kill()
CFLAGS += -DTHREADED 	# allows zoo_create()

RELEASE_CFLAGS := -O2
DEBUG_CFLAGS   := -g -O0 -fno-inline -DDEBUG
TEST_CFLAGS    := -I.
LDFLAGS 	   := -lzookeeper_mt

AR 		 := ar
AR_FLAGS := rcs

GDB := gdb

VALGRIND  	   := valgrind
VALGRIND_FLAGS := --leak-check=full \
				  --show-leak-kinds=all \
				  --track-origins=yes \
				  --trace-children=yes

TARGET 		:= libmessageq.a
TEST_TARGET := test_api \
			   test_network \
			   test_zookeeper_util 

OBJ 	  := api.o \
	   		 network.o \
	   		 zookeeper_util.o
DEBUG_OBJ := $(OBJ:%.o=debug_%.o)
TEST_OBJ  := $(OBJ:%.o=test_%.o)

DEPS := $(OBJ:.o=.d) $(DEBUG_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

GDB_TEST_TARGET      := $(TEST_TARGET:%=gdb-%)
VALGRIND_TEST_TARGET := $(TEST_TARGET:%=valgrind-%)

.DELETE_ON_ERROR:
.PHONY: all release debug test $(GDB_TEST_TARGET) $(VALGRIND_TEST_TARGET) \
		format clean help

all: release
release: $(TARGET)

$(TARGET): $(OBJ)
	@$(AR) $(AR_FLAGS) $@ $^

$(OBJ): CFLAGS += $(RELEASE_CFLAGS)
$(OBJ): %.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

debug: $(DEBUG_OBJ)
$(DEBUG_OBJ): CFLAGS += $(DEBUG_CFLAGS)
$(DEBUG_OBJ): debug_%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
$(TEST_TARGET): %: %.o $(DEBUG_OBJ)
	@$(CC) $^ -o $@ $(LDFLAGS) 

$(TEST_OBJ): CFLAGS += $(TEST_CFLAGS) $(DEBUG_CFLAGS)
$(TEST_OBJ): %.o: tests/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(GDB_TEST_TARGET): gdb-%: %
	@$(GDB) $<

$(VALGRIND_TEST_TARGET): valgrind-%: %
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$<

format:
	@find . \( -name "*.c" -o -name "*.h" \) -exec clang-format -style='{BasedOnStyle: llvm, IndentWidth: 4}' -i {} +

clean:
	@rm -f $(TARGET) $(TEST_TARGET)
	@rm -f $(OBJ) $(DEBUG_OBJ) $(TEST_OBJ)
	@rm -f $(DEPS)

help:
	@echo "Available targets:"
	@echo "	release           - Build release library"
	@echo "	debug             - Build debug objects"
	@echo "	test              - Build test binaries"
	@echo "	gdb-{binary}      - Run GDB on binary"
	@echo "	valgrind-{binary} - Run Valgrind on binary"
	@echo "	format            - Format source code"
	@echo "	clean             - Remove all build artifacts"
	@echo "	help              - Display this message"

-include $(DEPS)
