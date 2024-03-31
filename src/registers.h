#pragma once

#include <array>
#include <algorithm>

constexpr int kMemSize = 4096;
constexpr int kGeneralRegisterCount = 16;
constexpr int kStackSize = 16;
constexpr int kKeyboardSize = 16;

constexpr int kScreenWidth = 64;
constexpr int kScreenHeight = 32;
constexpr int kScreenPixelCount = kScreenWidth * kScreenHeight;

struct registers {
    uint16_t pc = 0;
    uint16_t i = 0;
    uint8_t sp = 0;
    uint8_t dt = 0;
    uint8_t st = 0;
    std::array<uint8_t, kGeneralRegisterCount> v;
    std::array<uint8_t, kMemSize> mem;
    std::array<uint16_t, kStackSize> stack;
    std::array<bool, kKeyboardSize> kbd;
    std::array<bool, kScreenPixelCount> screen;

    inline void reset() {
        pc = 0;
        i = 0;
        sp = 0;
        dt = 0;
        st = 0;

        std::fill(v.begin(), v.end(), 0);
        std::fill(mem.begin(), mem.end(), 0);
        std::fill(stack.begin(), stack.end(), 0);
        std::fill(kbd.begin(), kbd.end(), 0);
        std::fill(screen.begin(), screen.end(), 0);
    }
};
