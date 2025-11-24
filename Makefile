BUILD_DIR ?= build

.PHONY: all configure build run clean distclean

all: build

configure:
	cmake -S $(CURDIR) -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/sillytui

clean:
	[ ! -d $(BUILD_DIR) ] || cmake --build $(BUILD_DIR) --target clean

distclean:
	rm -rf $(BUILD_DIR)

