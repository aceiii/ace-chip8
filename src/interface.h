#pragma once

#include "config.h"
#include "interpreter.h"
#include "applog.h"
#include "screen.h"
#include "assembly.h"
#include "keyboard.h"

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <memory>
#include <raylib.h>
#include <vector>

struct interface_settings {
  int window_width = 1200;
  int window_height = 800;
  bool lock_fps = true;
  bool show_demo = false;
  bool show_fps = false;
  bool show_screen = true;
  bool show_memory = true;
  bool show_registers = true;
  bool show_logs = true;
  bool show_emulation = true;
  bool show_misc = false;
  bool show_instructions = true;
  bool show_keyboard = true;
};

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
  AssemblyViewer assembly;
  Keyboard keyboard;

  std::vector<uint8_t> rom;

  Config<interface_settings> config;

  bool should_close = false;
  bool init_dock = true;
};
