#include "simple_service.hpp"

int main(int argc, char** argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple subscriber example.");
  parser.add<int>("port", "The port for the subscriber server.", 'p', 8002);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  int port;
  parser.get<int>("port", port);

  auto simple_service = std::make_shared<SimpleService>(port);
  if (!simple_service->ok()) {
    Log::error << "Failed to create simple_service." << std::endl;
    return 1;
  }

  simple_service->spin();

  return 0;
}