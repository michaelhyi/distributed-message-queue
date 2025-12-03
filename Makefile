.PHONY: format

format:
	find . \( -name "*.c" -o -name "*.h" \) -exec clang-format -style=file -i {} +
