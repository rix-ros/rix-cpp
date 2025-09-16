#include <iostream>

#include "rix/core/mediator.hpp"
#include "rix/ipc/signal.hpp"

int main() {
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", rix::core::RIXHUB_PORT);

  auto mediator = std::make_shared<rix::core::Mediator>(rixhub_endpoint);
  if (!mediator->ok()) {
    rix::util::Log::error << "Failed to create mediator." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  mediator->spin(std::move(sig));

  return 0;
}