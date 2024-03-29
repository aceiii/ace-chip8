#include "interpreter.h"

#include <spdlog/spdlog.h>

constexpr double kTimerFrequency = 1 / 60.0;

Interpreter::Interpreter(std::shared_ptr<registers> regs) : regs(regs) {}

void Interpreter::initialize() {
    timer.tick();
}

void Interpreter::update() {
    update_timers();
}

void Interpreter::cleanup() {
}

void Interpreter::step() {
}

void Interpreter::update_timers() {
    last_tick += timer.tick();
    while (last_tick > kTimerFrequency) {
        if (regs->dt > 0) {
            regs->dt -= 1;
        }
        if (regs->st > 0) {
            regs->st -= 1;
        }

        last_tick -= kTimerFrequency;
    }
}
