#pragma once

#include <fstream>
#include <functional>
#include <string>
#include <iostream>
#include <system_error>
#include <tl/expected.hpp>
#include <toml++/toml.hpp>

template <typename Settings>
class Config {
public:
  Settings settings;

  tl::expected<void, std::string> load(const std::string& filename) {
    auto result = toml::parse_file(filename);
    if (!result) {
      return tl::unexpected(std::string { result.error().description() });
    }
    table = std::move(result).table();
    return {};
  }

  tl::expected<void, std::string> save(const std::string& filename) const {
    try {
      std::ofstream file(filename);
      file << toml::toml_formatter { table };
      file.close();
    } catch (std::exception e) {
      return tl::unexpected(e.what());
    }
    return {};
  }

  tl::expected<void, std::string> serialize(std::function<void(const Settings &settings, toml::table&)> serialize_func) {
    try {
      serialize_func(settings, table);
    } catch (std::exception e) {
      return tl::unexpected(e.what());
    }
    return {};
  }

  tl::expected<void, std::string> deserialize(std::function<void(const toml::table&, Settings &settings)> deserialize_func) {
    try {
      deserialize_func(table, settings);
    } catch (std::exception e) {
      return tl::unexpected(e.what());
    }
    return {};
  }

private:
  toml::table table;
};
