.PHONY: test debug debug-test debug-test-attach valgrind valgrind-test clean

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
	$(CC) $(CFLAGS) $(DEBUG_OBJ) $(DEBUG_LIB_OBJ) -o $(DEBUG_TARGET)

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

debug: $(DEBUG_TARGET)
	$(GDB) $(GDB_FLAGS) $(DEBUG_TARGET)

debug-test: $(TEST_TARGET)
	./$(TEST_TARGET) $(DEBUG_TEST_FLAGS)

debug-test-attach: $(TEST_TARGET)
	$(GDB) ./$(TEST_TARGET) $(GDB_FLAGS) 

valgrind: $(TARGET)
	$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)

valgrind-test:
	$(VALGRIND) $(VALGRIND_FLAGS) $(TEST_TARGET)

clean:
	@rm -rf $(BUILD_DIR)