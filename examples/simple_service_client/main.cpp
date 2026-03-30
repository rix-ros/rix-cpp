#include "simple_service_client.hpp"

using namespace rix;

int main(int argc, char** argv) {
  Log::init(NAME);

  auto parser = ArgumentParser(NAME, "A simple publisher example.");
  parser.add<double>("rate", "The publish rate in Hz.", 'r', 1.0);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments.";
    return 1;
  }

  double rate;
  parser.get<double>("rate", rate);

  SimpleServiceClient node(rate);
  if (!node.ok()) {
    Log::error << "Failed to create node.";
    return 1;
  }
  node.spin();
  return 0;
}
