TARGET 		 := partition
DEBUG_TARGET := debug_partition
TEST_TARGET  := test_partition

OBJ := main.o \
	   partition.o \
	   queue.o

DEBUG_MAIN_OBJ := debug_main.o

DEBUG_OBJ := debug_partition.o \
			 debug_queue.o

TEST_OBJ := test_partition.o \
			test_queue.o

DEPS := main.d \
		partition.d \
		queue.d \
		debug_main.d \
		debug_partition.d \
		debug_queue.d \
		test_partition.d \
		test_queue.d \

include ../tools/common.mk
