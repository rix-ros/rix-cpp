#include <iostream>

#include "rix/core/mediator.hpp"
#include "rix/ipc/signal.hpp"
#include "rix/rix.hpp"

int main(int argc, char **argv) {
  auto parser = rix::util::ArgumentParser(
      "rixhub", "The RIXHub is a central mediator for RIX nodes to register with and discover each other.");
  parser.add_parser<rix::ipc::Endpoint>(rix::core::parse_endpoint);
  parser.add<rix::ipc::Endpoint>("endpoint", "The RIXHub endpoint.", 'e', rix::ipc::Endpoint("127.0.0.1", 48104));
  if (!parser.parse(argc, argv)) {
    rix::util::Log::error << "Failed to parse arguments.\n" << parser.help() << std::endl;
    return 1;
  }

  rix::ipc::Endpoint endpoint;
  if (!parser.get<rix::ipc::Endpoint>("endpoint", endpoint)) {
    rix::util::Log::error << "Failed to get endpoint argument." << std::endl;
    return 1;
  }

  auto mediator = std::make_shared<rix::core::Mediator>(endpoint);
  if (!mediator->ok()) {
    rix::util::Log::error << "Failed to create mediator." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  mediator->spin(std::move(sig));

  return 0;
}