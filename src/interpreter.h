#pragma once

#include "timer.h"
#include "registers.h"

#include <tl/optional.hpp>
#include <memory>

class Interpreter {
public:
    Interpreter(std::shared_ptr<registers> regs);

    void initialize();
    void update();
    void cleanup();

    void step();

private:
    void update_timers();
    void stack_push(uint16_t val);
    uint16_t stack_pop();
    void screen_clear();
    void screen_draw_sprite(uint8_t x, uint8_t y, uint8_t n);
    void screen_flip_pixel_at(uint8_t x, uint8_t y);
    tl::optional<uint8_t> get_pressed_key();
    bool is_key_pressed(uint8_t key);
    void init_font_sprites();
    uint16_t get_font_sprite_addr(uint8_t c);

    Timer timer;
    double last_tick = 0;

    std::shared_ptr<registers> regs;
};
