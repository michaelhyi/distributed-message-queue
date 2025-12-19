LDFLAGS 	:=
TARGET 		:= lib.a
TEST_TARGET := test_lib

OBJ := dmqp.o
OBJ += doubly_linked_list.o
OBJ += network.o

DEBUG_OBJ := debug_dmqp.o 
DEBUG_OBJ += debug_doubly_linked_list.o 
DEBUG_OBJ += debug_network.o

TEST_OBJ := test_dmqp.o
TEST_OBJ += test_doubly_linked_list.o
TEST_OBJ += test_integration_dmqp.o
TEST_OBJ += test_integration_network.o
TEST_OBJ += test_network.o

DEPS := dmqp.d
DEPS += doubly_linked_list.d
DEPS += network.d
DEPS += debug_dmqp.d
DEPS += debug_doubly_linked_list.d
DEPS += debug_network.d
DEPS += test_dmqp.d
DEPS += test_doubly_linked_list.d
DEPS += test_integration_dmqp.d
DEPS += test_integration_network.d
DEPS += test_network.d

OVERRIDE_LDFLAGS := 1
OVERRIDE_TARGET  := 1
OVERRIDE_HELP    := 1

include ../tools/common.mk

CFLAGS += -D_GNU_SOURCE # allows include <endian.h>

all: $(TARGET)
$(TARGET): $(OBJ)
	@$(AR) $(AR_FLAGS) $(TARGET) $(OBJ)

debug: $(DEBUG_OBJ)

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
