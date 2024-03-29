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

    Timer timer;
    double last_tick = 0;

    std::shared_ptr<registers> regs;
};
