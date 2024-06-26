#pragma once

#include "registers.h"
#include "timer.h"

#include <memory>
#include <optional>

const double kTimerFrequency = 1.0 / 60.0;
const int kDefaultPlayingUpdateRate = 1200;

class Interpreter {
public:
  int update_play_rate = kDefaultPlayingUpdateRate;

public:
  explicit Interpreter(std::shared_ptr<registers> regs);

  void initialize();
  void update();
  void cleanup();

  void load_rom_bytes(const std::vector<uint8_t> &bytes);

  void reset();
  void step();
  void play();
  void stop();

  bool is_playing() const;

private:
  void update_keyboard();
  void update_timers(double dt);
  void stack_push(uint16_t val);
  uint16_t stack_pop();
  void screen_clear();
  void screen_draw_sprite(int x, int y, int n);
  void screen_flip_pixel_at(int x, int y);
  std::optional<uint8_t> get_pressed_key();
  bool is_key_pressed(uint8_t key);
  void init_font_sprites();
  uint16_t get_font_sprite_addr(uint8_t c);

  Timer timer;
  double last_tick = 0;
  double last_update = 0;
  bool playing = false;

  std::shared_ptr<registers> regs;
  std::array<bool, kKeyboardSize> key_down{false};
  std::array<bool, kKeyboardSize> key_released{false};
};
