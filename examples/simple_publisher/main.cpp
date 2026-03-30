#include "simple_publisher.hpp"

using namespace rix;

int main(int argc, char** argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple publisher example.");
  parser.add<int>("port", "The port for the publisher server.", 'p', 8000);
  parser.add<double>("rate", "The publish rate in Hz.", 'r', 1.0);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments.";
    return 1;
  }

  double rate;
  int port;
  parser.get<double>("rate", rate);
  parser.get<int>("port", port);

  auto simple_publisher = std::make_shared<SimplePublisher>(rate, port);
  if (!simple_publisher->ok()) {
    Log::error << "Failed to create simple_publisher.";
    return 1;
  }

  simple_publisher->spin();

  return 0;
}