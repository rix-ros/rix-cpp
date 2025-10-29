#include "simple_action.hpp"

int main(int argc, char** argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple subscriber example.");
  parser.add<int>("max_iters", "The maximum number of iterations.", 'i', 100);
  parser.add<int>("port", "The port for the subscriber server.", 'p', 8003);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  int port, max_iters;
  parser.get<int>("max_iters", max_iters);
  parser.get<int>("port", port);

  auto simple_action = std::make_shared<SimpleAction>(max_iters, port);
  if (!simple_action->ok()) {
    Log::error << "Failed to create simple_action." << std::endl;
    return 1;
  }

  simple_action->spin();

  return 0;
}