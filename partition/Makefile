TARGET 		 := partition
DEBUG_TARGET := debug_partition
TEST_TARGET  := test_partition

OBJ := main.o
OBJ += partition.o
OBJ += queue.o

DEBUG_MAIN_OBJ := debug_main.o

DEBUG_OBJ := debug_partition.o
DEBUG_OBJ += debug_queue.o

TEST_OBJ := test_partition.o
TEST_OBJ += test_queue.o

DEPS := main.d
DEPS += partition.d
DEPS += queue.d
DEPS += debug_main.d
DEPS += debug_partition.d
DEPS += debug_queue.d
DEPS += test_partition.d
DEPS += test_queue.d

include ../tools/common.mk
