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

DEPS := $(OBJ:.o=.d) \
		$(DEBUG_MAIN_OBJ:.o=.d) \
		$(DEBUG_OBJ:.o=.d) \
		$(TEST_OBJ:.o=.d)

include ../tools/common.mk
