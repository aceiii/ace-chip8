#include "app.hpp"
#include <spdlog/spdlog.h>
#include <magic_enum.hpp>


static bool set_logging_level(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level::level_enum>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

App::App(Args&& args): args_{std::move(args)} {
  set_logging_level(args.log_level);

  regs_ = std::make_shared<registers>();

  interpreter_ = std::make_unique<Interpreter>(regs_);
  interface_ = std::make_unique<Interface>(regs_, interpreter_.get());

  interface_->initialize();
  interpreter_->initialize();
}

App::~App() {
  interface_->cleanup();
  interpreter_->cleanup();
}

void App::Run() {
  do {
    interpreter_->update();
  } while (!interface_->update());
}
