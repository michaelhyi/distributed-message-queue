ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
include $(ROOT_DIR)/tools/config.mk

CFLAGS += -D_POSIX_C_SOURCE=200809L # allows struct sigaction to compile

# copy of /tools/target.mk, with slight changes to targets since this doesn't
# have a `main` function

.PHONY: all test debug-test clean

all: $(OBJ)

# link test obj files
$(TEST_TARGET): $(TEST_OBJ) $(DEBUG_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_TEST_FLAGS) $^ -o $(TEST_TARGET)

# compile src files
$(BUILD_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_OPT_FLAGS) -c $< -o $@

# compile test files
$(BUILD_DIR)/test/%.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_DEBUG_FLAGS) -c $< -o $@

# compile debug files
$(BUILD_DIR)/debug/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_DEBUG_FLAGS) -c $< -o $@

test: $(TEST_TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TEST_TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

clean:
	@rm -rf $(BUILD_DIR)
