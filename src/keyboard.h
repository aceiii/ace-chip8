#pragma once

#include "registers.h"
#include <vector>
#include <string>

struct key_mapping {
  std::string label;
  int keycode;
  uint8_t key;
};

class Keyboard {
public:
  Keyboard();

  void initialize(std::shared_ptr<registers> regs);
  void update();
  void draw();

private:
  std::vector<key_mapping> mapping;
  std::shared_ptr<registers> regs;
};
