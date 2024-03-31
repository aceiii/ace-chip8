#include "random.h"
#include <random>

uint8_t random_byte() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, 255);
    return dist(gen);
}
