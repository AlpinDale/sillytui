BUILD_DIR ?= build
SRC_FILES := $(shell find src tests -name '*.c' -o -name '*.h')

# Detect platform
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    SIMD_ASM := src/tokenizer/simd_x86_64.S
else
    SIMD_ASM := src/tokenizer/simd_arm64.S
endif

TOKENIZE_SRCS := examples/tokenize.c src/tokenizer/tiktoken.c src/tokenizer/gpt2bpe.c \
	src/tokenizer/sentencepiece.c src/tokenizer/simd.c $(SIMD_ASM) \
	src/tokenizer/unicode_tables.c

.PHONY: all configure build build-all run clean distclean format format-check tokenize example test

all: build

configure:
	cmake -S $(CURDIR) -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) --target sillytui

build-all: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/sillytui

clean:
	[ ! -d $(BUILD_DIR) ] || cmake --build $(BUILD_DIR) --target clean

distclean:
	rm -rf $(BUILD_DIR)

format:
	clang-format -i $(SRC_FILES)

format-check:
	@echo "Checking code formatting..."
	@clang-format --dry-run --Werror $(SRC_FILES) && echo "All files properly formatted!" || (echo "Some files need formatting. Run 'make format' to fix." && exit 1)

tokenize: $(BUILD_DIR)/tokenize

$(BUILD_DIR)/tokenize: $(TOKENIZE_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) -O3 -o $@ $(TOKENIZE_SRCS) -Isrc

example: tokenize
	@./$(BUILD_DIR)/tokenize $(ARGS)

test: build-all
	@./$(BUILD_DIR)/run_all_tests
