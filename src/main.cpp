#include "interface.h"
#include "interpreter.h"
#include "registers.h"

#include <argparse/argparse.hpp>
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>

static bool set_logging_level(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level::level_enum>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

auto main(int argc, char *argv[]) -> int {
  spdlog::set_level(spdlog::level::info);

  argparse::ArgumentParser program("ace-chip8", "0.0.1");

  program.add_argument("--log-level")
      .help("Set the verbosity for logging")
      .default_value(std::string("info"))
      .nargs(1);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  const std::string level = program.get("--log-level");
  if (!set_logging_level(level)) {
    std::cerr << fmt::format("Invalid argument \"{}\" - allowed options: "
                             "{{trace, debug, info, warn, err, critical, off}}",
                             level)
              << std::endl;
    std::cerr << program;
    return 1;
  }

  auto regs = std::make_shared<registers>();

  Interpreter interpreter(regs);
  Interface interface(regs, &interpreter);

  interface.initialize();
  interpreter.initialize();

  do {
    interpreter.update();
  } while (!interface.update());

  interface.cleanup();
  interpreter.cleanup();

  spdlog::info("Exiting.");

  return 0;
}
