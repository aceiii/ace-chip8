#pragma once

#include "interpreter.h"

#include <raylib.h>
#include <memory>

class Interface {
public:
    Interface(std::shared_ptr<registers> regs, Interpreter* interpreter);

    void initialize();
    bool update();
    void cleanup();

private:
    void open_load_rom_dialog();
    void load_rom(const std::string& string);

    Interpreter* interpreter;
    std::shared_ptr<registers> regs;
    RenderTexture2D screen_texture;

    std::vector<uint8_t> rom;

    bool should_close = false;
    bool show_screen = true;
    bool show_memory = true;
    bool show_registers = true;
};
