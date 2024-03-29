#pragma once

#include "timer.h"

#include <array>

constexpr int kMemSize = 4096;
constexpr int kGeneralRegisterCount = 16;

struct registers {
    uint8_t delay = 0;
    uint8_t sound = 0;
    uint16_t i = 0;
    std::array<uint8_t, kGeneralRegisterCount> v;
    std::array<uint8_t, kMemSize> mem;
};

class Interpreter {
public:
    void initialize();
    void update();
    void cleanup();

    void step();

    registers* get_registers();

private:
    Timer timer;
    double last_tick = 0;

    registers regs;
};
