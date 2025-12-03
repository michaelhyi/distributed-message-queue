ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
include $(ROOT_DIR)/tools/config.mk

# TODO: this is such a mess
# TODO: add formatting

.PHONY: test debug debug-test memcheck clean

# TODO: link with lib/build/src

$(TARGET): $(OBJ) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) $(LIB_OBJ) -o $(TARGET)

$(TEST_TARGET): $(filter-out $(BUILD_DIR)/debug/src/main.o,$(DEBUG_OBJ)) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_TEST_FLAGS) $^ $(LIB_OBJ)-o $(TEST_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_OBJ) $(LIB_OBJ) -o $(DEBUG_TARGET)

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

debug: $(DEBUG_TARGET)
	$(GDB) $(DEBUG_TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(TEST_TARGET)

memcheck: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

clean:
	@rm -rf $(BUILD_DIR)
