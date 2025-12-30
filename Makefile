MODULES := lib partition

.PHONY: all $(MODULES) format clean help

all: $(MODULES)
$(MODULES):
	@$(MAKE) --no-print-directory -C $@

format:
	@find . \( -name "*.c" -o -name "*.h" \) -exec clang-format -style='{BasedOnStyle: llvm, IndentWidth: 4}' -i {} +

clean:
	@for module in $(MODULES); do \
		$(MAKE) --no-print-directory -C $$module clean; \
	done

help:
	@echo "Available targets:"
	@echo "	all       - Build all modules"
	@echo "	format    - Format source code"
	@echo "	clean     - Remove all build artifacts"
	@echo "	help      - Display this message"
