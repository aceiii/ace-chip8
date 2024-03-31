#pragma once

#include "timer.h"
#include "registers.h"

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

    Timer timer;
    double last_tick = 0;

    std::shared_ptr<registers> regs;
};
