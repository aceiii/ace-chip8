#pragma once

#include <chrono>

class Timer {
public:
    Timer();

    double tick();

private:
    std::chrono::high_resolution_clock::time_point start;
};
