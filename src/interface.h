#pragma once

#include "config.h"
#include "interpreter.h"
#include "applog.h"
#include "screen.h"
#include "assembly.h"
#include "keyboard.h"
#include "sound.h"

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <memory>
#include <raylib.h>
#include <vector>

struct interface_settings {
  int window_width;
  int window_height;
  float volume;
  bool lock_fps;
  bool show_demo;
  bool show_fps;
  bool show_screen;
  bool show_memory;
  bool show_registers;
  bool show_logs;
  bool show_emulation;
  bool show_misc;
  bool show_instructions;
  bool show_keyboard;
  bool show_audio;
  bool show_timers;
  bool auto_play;

  void reset() {
    volume = 50.0f;
    lock_fps = true;
    show_fps = false;
    show_demo = false;
    show_screen = true;
    show_memory = true;
    show_registers = true;
    show_logs = true;
    show_emulation = true;
    show_misc = false;
    show_instructions = true;
    show_keyboard = false;
    show_audio = false;
    show_timers = false;
    auto_play = true;
  }
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
  void set_window_title(const std::string &string);

private:
  Interpreter *interpreter;
  MemoryEditor mem_editor;
  AppLog app_log;
  Screen screen;
  AssemblyViewer assembly;
  Keyboard keyboard;
  Config<interface_settings> config;
  std::vector<uint8_t> rom;
  std::shared_ptr<registers> regs;
  std::vector<std::unique_ptr<SoundSource>> sounds;
};
