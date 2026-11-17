#pragma once

#include <expected>
#include <optional>
#include <string>

struct Args final {
  std::string log_level;

  static std::expected<Args, std::string> Parse(int argc, char* argv[]);
};
