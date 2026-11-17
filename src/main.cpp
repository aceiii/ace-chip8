#include "args.hpp"
#include "interface.h"
#include "interpreter.h"
#include "registers.h"

#include <format>
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

static bool set_logging_level(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level::level_enum>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

auto main(int argc, char *argv[]) -> int {
  const auto &args = Args::Parse(argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error();
    exit(1);
  }

  set_logging_level(args->log_level);

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
