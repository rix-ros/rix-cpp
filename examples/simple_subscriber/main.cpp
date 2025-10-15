#include "simple_subscriber.hpp"

int main(int argc, char** argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple subscriber example.");
  parser.add<int>("port", "The port for the subscriber server.", 'p', 8000);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  int port;
  parser.get<int>("port", port);

  auto simple_subscriber = std::make_shared<SimpleSubscriber>(port);
  if (!simple_subscriber->ok()) {
    Log::error << "Failed to create simple_subscriber." << std::endl;
    return 1;
  }

  simple_subscriber->spin();

  return 0;
}