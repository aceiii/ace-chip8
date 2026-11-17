#include "args.hpp"

#include <format>
#include <argparse/argparse.hpp>


std::expected<Args, std::string> Args::Parse(int argc, char* argv[]) {
  argparse::ArgumentParser program("ace-chip8", "0.0.1");

  program.add_argument("--log-level")
    .help("Set the verbosity for logging")
    .default_value("info")
    .choices("trace", "debug", "info", "warn", "err", "critical", "off")
    .nargs(1);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    return std::unexpected{std::format("{}\n\n{}", err.what(), program.help().str())};
  }

  return Args{
    .log_level = program.get("--log-level"),
  };
}
