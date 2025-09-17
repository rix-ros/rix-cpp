#include <iostream>

#include "rix/rix.hpp"
#include "rix/core/mediator.hpp"
#include "rix/ipc/signal.hpp"

int main(int argc, char **argv) {
  auto parser = rix::util::ArgumentParser("simple_publisher",
                                          "A simple publisher example.");
  parser.add<std::string>("endpoint", "The RixHub endpoint.", 'e', "127.0.0.1:48104");

  if (!parser.parse(argc, argv)) {
    rix::util::Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  std::string rixhub_endpoint_string;
  if (!parser.get<std::string>("endpoint", rixhub_endpoint_string)) {
    rix::util::Log::error << "Failed to get endpoint argument." << std::endl;
    return 1;
  }

  auto mediator = std::make_shared<rix::core::Mediator>(rix::ipc::Endpoint(rixhub_endpoint_string));
  if (!mediator->ok()) {
    rix::util::Log::error << "Failed to create mediator." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  mediator->spin(std::move(sig));

  return 0;
}