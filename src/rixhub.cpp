#include <iostream>

#include "rix/core/mediator.hpp"
#include "rix/ipc/signal.hpp"
#include "rix/rix.hpp"

using namespace rix;
using namespace rix;
using namespace rix;

int main(int argc, char **argv) {
  auto parser = ArgumentParser("rixhub", "The RIXHub is a central mediator for RIX nodes to discover each other.");
  parser.add<std::string>("default_ip", "The default IP address for servers to bind to.", DEFAULT_IP);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  Endpoint endpoint(DEFAULT_IP, RIXHUB_PORT);
  parser.get<std::string>("default_ip", endpoint.address);

  auto mediator = std::make_shared<Mediator>(endpoint);
  if (!mediator->ok()) {
    Log::error << "Failed to create mediator." << std::endl;
    return 1;
  }

  mediator->spin();

  return 0;
}