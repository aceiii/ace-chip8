#pragma once

#include "applog.h"
#include "assembly.h"
#include "config.h"
#include "interpreter.h"
#include "keyboard.h"
#include "screen.h"
#include "sound.h"
#include "spdlog/spdlog.h"

#include <deque>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <memory>
#include <raylib.h>
#include <vector>

struct interface_settings {
  static constexpr int max_recent_files = 10;

  int window_width;
  int window_height;
  float volume;
  bool lock_fps;
  bool show_demo;
  bool show_fps;
  bool show_screen;
  bool show_display;
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

  bool show_grid_lines;
  Color grid_color;
  Color pixel_on;
  Color pixel_off;

  std::deque<std::string> recent_files;

  void reset() {
    volume = 50.0f;
    lock_fps = true;
    show_fps = false;
    show_demo = false;
    show_screen = true;
    show_display = false;
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

    show_grid_lines = true;
    grid_color = BLACK;
    pixel_on = RAYWHITE;
    pixel_off = BLACK;
    recent_files.clear();
  }

  void add_recent_file(std::string_view file) {
    auto it = std::find(recent_files.begin(), recent_files.end(), file);
    if (it != recent_files.end()) {
      return;
    }

    recent_files.emplace_back(file);
    if (recent_files.size() > max_recent_files) {
      recent_files.pop_front();
    }
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
  SoundManager sounds;
  std::vector<uint8_t> rom;
  std::shared_ptr<registers> regs;
};
