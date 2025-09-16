#include <iostream>
#include <thread>

#include "rix/core/node.hpp"
#include "rix/ipc/signal.hpp"
#include "rix/msg/standard/Header.hpp"

class SimpleSubscriber : public rix::core::Node {
public:
  // Initialize the Node with a name and the RixHub endpoint
  SimpleSubscriber()
      : Node("simple_subscriber",
             rix::ipc::Endpoint("127.0.0.1", rix::core::RIXHUB_PORT)) {

    // If the Node failed to initialize, then ok() will return false
    if (!ok()) {
      rix::util::Log::error << "Failed to create node." << std::endl;
      shutdown();
      return;
    }

    // Create a subscriber on topic /chatter with message type Header
    auto sub = create_subscriber<rix::msg::standard::Header>(
        "/chatter",
        std::bind(&SimpleSubscriber::callback, this, std::placeholders::_1));

    // If the subscriber failed to initialize, then ok() will return false
    if (!sub->ok()) {
      rix::util::Log::error << "Failed to create subscriber." << std::endl;
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::msg::standard::Header &msg) {
    rix::util::Log::info << "Received message " << msg.seq << " : "
                         << msg.frame_id << std::endl;
  }
};

int main() {
  auto simple_subscriber = std::make_shared<SimpleSubscriber>();
  if (!simple_subscriber->ok()) {
    rix::util::Log::error << "Failed to create simple_subscriber." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  simple_subscriber->spin(std::move(sig));

  return 0;
}