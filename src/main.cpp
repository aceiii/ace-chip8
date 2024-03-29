#include "interpreter.h"
#include "interface.h"

#include <spdlog/spdlog.h>

auto main() -> int {
    spdlog::info("Hello, world!!!");

    Interpreter interpreter;
    Interface interface;

    interpreter.initialize();
    interface.initialize();

    while (true) {
        interpreter.update();
        if (interface.update() {
            break;
        }
    }

    return 0;
}
