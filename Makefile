BUILD_DIR	= build
SRC_DIR		= .
TARGET		= build/ace-chip8

.PHONY: all configure configure-debug build clean run run-debug

all: configure build

build:
	cmake --build $(BUILD_DIR)

configure: CMakeLists.txt
	cmake -B $(BUILD_DIR) -G Ninja -S $(SRC_DIR) -DCMAKE_BUILD_TYPE=RelWithDebInfo

configure-debug: CMakeLists.txt
	cmake -B $(BUILD_DIR) -G Ninja -S $(SRC_DIR) -DCMAKE_BUILD_TYPE=Debug

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

run-debug: $(TARGET)
	./$(TARGET) --log-level=debug

$(TARGET): build

