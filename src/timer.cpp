#include "timer.h"

using namespace std::chrono;

Timer::Timer() : start(high_resolution_clock::now()) {}

double Timer::tick() {
  const auto now = high_resolution_clock::now();
  duration<double> diff = duration_cast<duration<double>>(now - start);
  start = now;
  return diff.count();
}
