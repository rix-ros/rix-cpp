#include <iostream>
#include <thread>

#include "rix/core/node.hpp"
#include "rix/ipc/signal.hpp"
#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"

class SimpleService : public rix::core::Node {
public:
  SimpleService()
      : Node("simple_service",
             rix::ipc::Endpoint("127.0.0.1", rix::core::RIXHUB_PORT)) {
    auto srv =
        create_service<rix::msg::standard::UInt32, rix::msg::standard::String>(
            "/alphabet",
            std::bind(&SimpleService::callback, this, std::placeholders::_1,
                      std::placeholders::_2));
    if (!srv->ok()) {
      rix::util::Log::error << "Failed to create service." << std::endl;
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::msg::standard::UInt32 &request,
                rix::msg::standard::String &response) {
    response.data = std::string(1, 'a' + (request.data % 26));
  }
};

int main() {
  auto simple_service = std::make_shared<SimpleService>();
  if (!simple_service->ok()) {
    rix::util::Log::error << "Failed to create simple_service." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  simple_service->spin(std::move(sig));

  return 0;
}