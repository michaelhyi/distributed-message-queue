MODULES := lib partition samples

.PHONY: all format $(MODULES) clean

all: $(MODULES)
$(MODULES):
	$(MAKE) -C $@

format:
	find . \( -name "*.c" -o -name "*.h" \) -exec clang-format -style='{BasedOnStyle: llvm, IndentWidth: 4}' -i {} +

clean:
	for module in $(MODULES); do \
		$(MAKE) -C $$module clean; \
	done
