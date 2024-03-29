#include "interpreter.h"
#include "interface.h"

#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>

static bool set_logging_level(const std::string& level) {
    if (level == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (level == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (level == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (level == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (level == "err") {
        spdlog::set_level(spdlog::level::err);
    } else if (level == "critical") {
        spdlog::set_level(spdlog::level::critical);
    } else if (level == "off") {
        spdlog::set_level(spdlog::level::off);
    } else {
        return false;
    }
    return true;
}

auto main(int argc, char* argv[]) -> int {
    spdlog::set_level(spdlog::level::info);

    argparse::ArgumentParser program("ace-chip8", "0.0.1");

    program.add_argument("--log-level")
        .help("Set the verbosity for logging")
        .default_value(std::string("info"))
        .nargs(1);

    try {
        program.parse_args(argc, argv);
    } catch(const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    const std::string level = program.get("--log-level");
    if (!set_logging_level(level)) {
        std::cerr << fmt::format("Invalid argument \"{}\" - allowed options: {{trace, debug, info, warn, err, critical, off}}", level) << std::endl;
        std::cerr << program;
        return 1;
    }

    Interpreter interpreter;
    Interface interface;

    interpreter.initialize();
    interface.initialize();

    while (true) {
        interpreter.update();
        if (interface.update()) {
            break;
        }
    }

    return 0;
}
