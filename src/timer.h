#pragma once

#include <chrono>

class Timer {
public:
    Timer();

    void reset();
    double duration();

private:
    std::chrono::high_resolution_clock::time_point start;
};
