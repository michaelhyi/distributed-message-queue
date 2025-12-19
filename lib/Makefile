LDFLAGS 	:=
TARGET 		:= lib.a
TEST_TARGET := test_lib

OBJ 	  := dmqp.o doubly_linked_list.o network.o
DEBUG_OBJ := debug_dmqp.o debug_doubly_linked_list.o debug_network.o
TEST_OBJ  := test_dmqp.o test_doubly_linked_list.o test_integration_dmqp.o \
			 test_integration_network.o test_network.o
DEPS      := dmqp.d doubly_linked_list.d network.d \
			 debug_dmqp.d debug_doubly_linked_list.d debug_network.d \
			 test_dmqp.d test_doubly_linked_list.d test_integration_dmqp.d \
			 test_integration_network.d test_network.d

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
