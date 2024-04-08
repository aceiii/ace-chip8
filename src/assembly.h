#pragma once

#include "registers.h"

class AssemblyViewer {
public:
  void initialize(const registers *regs);
  void draw();
  void cleanup();

private:
  std::string disassembled_instruction(uint16_t instr) const;
  bool auto_scroll = true;

  const registers *regs = nullptr;
};
