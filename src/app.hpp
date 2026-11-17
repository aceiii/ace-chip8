#pragma once

#include <memory>
#include "args.hpp"
#include "registers.h"
#include "interface.h"
#include "interpreter.h"

class App final {
public:
  App() = delete;

  explicit App(Args&& args);
  ~App();

  void Run();

private:
  Args args_;
  std::shared_ptr<registers> regs_;
  std::unique_ptr<Interface> interface_;
  std::unique_ptr<Interpreter> interpreter_;
};
