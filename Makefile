MODULES := lib partition samples

.PHONY: all format $(MODULES) clean help

all: $(MODULES)
$(MODULES):
	@$(MAKE) --no-print-directory -C $@

format:
	@for module in $(MODULES); do \
		$(MAKE) --no-print-directory -C $$module format; \
	done

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
