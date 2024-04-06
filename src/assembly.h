#pragma once

#include "registers.h"

class AssemblyViewer {
public:
  void initialize(const registers *regs);
  void draw();
  void cleanup();

private:
  bool auto_scroll = true;

  const registers *regs = nullptr;
};
