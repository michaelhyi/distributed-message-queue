# TODO: give project a name + make this an actual system library
# TODO: test_api target not deleting on `clean`, maybe just separate common mk file?

LDFLAGS 		:= -lzookeeper_mt
TARGET 			:= lib.a
TEST_TARGET 	:= test_lib
TEST_API_TARGET := test_api

TEST_NETWORK_TARGET := test_network

OBJ := api.o \
	   network.o \
	   zookeeper_util.o

DEBUG_OBJ := $(OBJ:%.o=debug_%.o)

TEST_OBJ := test_api.o \
 			test_network.o \
			test_zookeeper_util.o

DEPS := $(OBJ:.o=.d) $(DEBUG_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

OVERRIDE_LDFLAGS := 1
OVERRIDE_TARGET  := 1
OVERRIDE_HELP    := 1

include ../tools/common.mk

CFLAGS += -D_GNU_SOURCE # allows usleep() and kill()
CFLAGS += -DTHREADED 	# allows zoo_create()

.PHONY: $(TEST_API_TARGET) $(TEST_NETWORK_TARGET)

$(TARGET): $(OBJ)
	@$(AR) $(AR_FLAGS) $(TARGET) $(OBJ)

$(TEST_API_TARGET): $(DEBUG_OBJ) $(TEST_OBJ)
	@$(CC) debug_api.o debug_zookeeper_util.o test_api.o -o $(TEST_API_TARGET) $(LDFLAGS) 

$(TEST_NETWORK_TARGET): $(DEBUG_OBJ) $(TEST_OBJ)
	@$(CC) debug_network.o test_network.o -o $(TEST_NETWORK_TARGET) $(LDFLAGS) 

debug: $(DEBUG_OBJ)

# TODO:
# clean:
# 	@rm -f $(TEST_API_TARGET) $(TEST_NETWORK_TARGET)

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
