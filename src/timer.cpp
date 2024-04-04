#include "timer.h"

using namespace std::chrono;
using duration_type = duration<double>;

Timer::Timer() : start(high_resolution_clock::now()) {}

void Timer::reset() {
  start = high_resolution_clock::now();;
}

double Timer::duration() {
  const auto now = high_resolution_clock::now();
  duration_type diff = duration_cast<duration_type>(now - start);
  return diff.count();
}
