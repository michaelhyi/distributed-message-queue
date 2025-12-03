ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
include $(ROOT_DIR)/tools/config.mk

.PHONY: test debug-test clean

$(OBJ): $(OBJ)

$(TEST_TARGET): $(TEST_OBJ) $(DEBUG_OBJ) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_TEST_FLAGS) $^ -o $(TEST_TARGET)

$(BUILD_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_OPT_FLAGS) -c $< -o $@

$(BUILD_DIR)/test/%.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_DEBUG_FLAGS) -c $< -o $@

$(BUILD_DIR)/debug/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_DEBUG_FLAGS) -c $< -o $@

test: $(TEST_TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TEST_TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

clean:
	@rm -rf $(BUILD_DIR)
