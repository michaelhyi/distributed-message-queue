.PHONY: test debug debug-test memcheck clean

# link src obj files
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJ) $(LIB_OBJ) -o $(TARGET)

# link test obj files
$(TEST_TARGET): $(TEST_OBJ) $(filter-out $(BUILD_DIR)/debug/src/main.o,$(DEBUG_OBJ)) 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C_TEST_FLAGS) $^ $(LIB_OBJ) -o $(TEST_TARGET)

# link debug obj files
$(DEBUG_TARGET): $(DEBUG_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_OBJ) $(LIB_OBJ) -o $(DEBUG_TARGET)

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

debug: $(DEBUG_TARGET)
	$(GDB) $(GDB_FLAGS) $(DEBUG_TARGET)

debug-test: $(TEST_TARGET)
	$(GDB) $(GDB_FLAGS) $(TEST_TARGET)

memcheck: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

clean:
	@rm -rf $(BUILD_DIR)