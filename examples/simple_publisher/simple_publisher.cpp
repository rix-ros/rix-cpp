#include "simple_publisher.hpp"

using namespace rix;

SimplePublisher::SimplePublisher(double rate, int port) : Node(NAME) {

  // If the Node failed to initialize, then ok() will return false
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  // Create a publisher on topic /chatter with message type Header
  pub = create_publisher<std_msgs::Header>("/chatter", Endpoint(DEFAULT_IP, port));

  // If the publisher failed to initialize, then ok() will return false
  if (!pub->ok()) {
    Log::error << "Failed to create publisher." << std::endl;
    shutdown();
    return;
  }

  // Initialize our message parameters
  message.frame_id = "Hello, world!";
  message.seq = 0;
  timer = create_timer(Duration(1.0 / rate), &SimplePublisher::timer_callback, this);
}

void SimplePublisher::timer_callback(const TimerCallback::Event& event) {
  message.frame_id = "Hello, world!";
  message.seq += 1;
  message.stamp = Time::now().to_msg();
  pub->publish(message);
}