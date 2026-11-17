#include "args.hpp"
#include "app.hpp"
#include "interface.h"
#include "interpreter.h"
#include "registers.h"


auto main(int argc, char *argv[]) -> int {
  auto args = Args::Parse(argc, argv);
  if (!args.has_value()) {
    std::cerr << args.error();
    exit(1);
  }

  App app(std::move(args.value()));

  app.Run();

  spdlog::info("Exiting.");

  return 0;
}
