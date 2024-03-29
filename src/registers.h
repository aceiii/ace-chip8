#pragma once

#include <array>

constexpr int kMemSize = 4096;
constexpr int kGeneralRegisterCount = 16;

struct registers {
    uint16_t pc = 0;
    uint16_t i = 0;
    uint8_t sp = 0;
    uint8_t dt = 0;
    uint8_t st = 0;
    std::array<uint8_t, kGeneralRegisterCount> v;
    std::array<uint8_t, kMemSize> mem;
};
