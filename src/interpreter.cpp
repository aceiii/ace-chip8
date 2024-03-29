#include "interpreter.h"

#include <spdlog/spdlog.h>

constexpr double kTimerFrequency = 1 / 60.0;

void Interpreter::initialize() {
    timer.tick();
}

void Interpreter::update() {
    last_tick += timer.tick();
    while (last_tick > kTimerFrequency) {
        if (regs.delay > 0) {
            regs.delay -= 1;
        }
        if (regs.sound > 0) {
            regs.sound -= 1;
        }

        last_tick -= kTimerFrequency;
    }
}

void Interpreter::cleanup() {
}

void Interpreter::step() {
}

registers* Interpreter::get_registers() {
    return &regs;
}
