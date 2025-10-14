# Makefile for formatting native source files with clang-format
#
# 使い方:
#   WSL 上で `make format` を実行すると、C/C++ ソース全体を clang-format で整形します。
#   必要に応じて `CLANG_FORMAT` 変数で利用する clang-format バイナリを上書きしてください。

CLANG_FORMAT ?= clang-format
FORMAT_DIRS := app/src/main/cpp
FORMAT_GLOBS := -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" -o -name "*.h" -o -name "*.hpp" -o -name "*.hh"

.PHONY: format
format:
	@echo "==> Running clang-format via $(CLANG_FORMAT)"
	@set -e; \
	for dir in $(FORMAT_DIRS); do \
		if [ -d $$dir ]; then \
			find $$dir -type f \( $(FORMAT_GLOBS) \) -print0 | xargs -0 $(CLANG_FORMAT) -i; \
		else \
			echo "Warning: directory '$$dir' was not found"; \
		fi; \
	done
	@echo "==> clang-format complete"
