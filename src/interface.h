#pragma once

#include "interpreter.h"
#include "applog.h"
#include "screen.h"

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <memory>
#include <raylib.h>
#include <vector>

class Interface {
public:
  Interface(std::shared_ptr<registers> regs, Interpreter *interpreter);

  void initialize();
  bool update();
  void cleanup();

private:
  void open_load_rom_dialog();
  void load_rom(const std::string &string);

  void render_main_menu();
  void reset_windows();

  Interpreter *interpreter;
  std::shared_ptr<registers> regs;
  MemoryEditor mem_editor;
  AppLog app_log;
  Screen screen;

  ImFont *font_default;
  ImFont *font_icons;

  std::vector<uint8_t> rom;

  bool init_dock = true;
  bool should_close = false;
  bool show_demo = false;
  bool show_fps = false;
  bool show_screen = true;
  bool show_memory = true;
  bool show_registers = true;
  bool show_logs = true;
  bool show_emulation = true;
  bool show_misc = false;
};
