#pragma once

#include <algorithm>
#include <array>

const int kMemSize = 4096;
const int kGeneralRegisterCount = 16;
const int kKeyboardSize = 16;

const int kStackPtrIndex = 0x0;
const int kStackStartIndex = 0x10;
const int kFontStartIndex = 0x50;
const int kRomStartIndex = 0x200;

const int kScreenWidth = 64;
const int kScreenHeight = 32;
const int kScreenPixelCount = kScreenWidth * kScreenHeight;

struct registers {
  uint16_t pc = 0;
  uint16_t i = 0;
  uint8_t dt = 0;
  uint8_t st = 0;
  std::array<uint8_t, kGeneralRegisterCount> v{0};
  std::array<uint8_t, kMemSize> mem{0};
  std::array<bool, kKeyboardSize> kbd{false};
  std::array<bool, kScreenPixelCount> screen{false};

  inline void reset() {
    pc = 0;
    i = 0;
    dt = 0;
    st = 0;

    std::fill(v.begin(), v.end(), 0);
    std::fill(mem.begin(), mem.end(), 0);
    std::fill(kbd.begin(), kbd.end(), 0);
    std::fill(screen.begin(), screen.end(), 0);
  }
};
