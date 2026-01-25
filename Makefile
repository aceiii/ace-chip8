BUILD_DIR	= build
SRC_DIR		= .
TARGET		= build/ace-chip8

.PHONY: all configure build clean run

all: configure build

build:
	cmake --build $(BUILD_DIR)

configure: CMakeLists.txt
	cmake -B $(BUILD_DIR) -G Ninja -S $(SRC_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

$(TARGET): build

