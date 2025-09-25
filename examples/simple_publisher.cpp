#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

using namespace rix::core;
using namespace rix::util;
using namespace rix::ipc;

const std::string NAME = "simple_publisher";
int PORT = 8001;
double RATE = 1.0; // Hz

class SimplePublisher : public Node {
public:
  // Initialize the Node with a name and the RixHub endpoint
  SimplePublisher() : Node(NAME) {

    // If the Node failed to initialize, then ok() will return false
    if (!ok()) {
      Log::error << "Failed to create node." << std::endl;
      return;
    }

    // Create a publisher on topic /chatter with message type Header
    pub = create_publisher<rix::msg::standard::Header>("/chatter", Endpoint(DEFAULT_IP, PORT));

    // If the publisher failed to initialize, then ok() will return false
    if (!pub->ok()) {
      Log::error << "Failed to create publisher." << std::endl;
      shutdown();
      return;
    }

    // Initialize our message parameters
    message.frame_id = "Hello, world!";
    message.seq = 0;

    timer =
        create_timer(Duration(1.0 / RATE), std::bind(&SimplePublisher::timer_callback, this, std::placeholders::_1));
  }

private:
  std::shared_ptr<Publisher> pub;
  std::shared_ptr<rix::core::Timer> timer;
  rix::msg::standard::Header message;

  /**
   * @brief Timer callback that is invoked by the Node at 1.0 Hz during spin
   *
   */
  void timer_callback(const rix::core::Timer::Event &event) {
    message.frame_id = "Hello, world!";
    message.seq += 1;
    message.stamp = Time::now().to_msg();
    pub->publish(message);
  }
};

int main(int argc, char **argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple publisher example.");
  parser.add<std::string>("rixhub_ip", "The IP address of the RIXHub server.", RIXHUB_IP);
  parser.add<std::string>("default_ip", "The default IP address for servers to bind to.", DEFAULT_IP);
  parser.add<int>("port", "The port for the publisher server.", 'p', PORT);
  parser.add<double>("rate", "The publish rate in Hz.", 'r', RATE);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  parser.get<std::string>("rixhub_ip", RIXHUB_IP);
  parser.get<std::string>("default_ip", DEFAULT_IP);
  parser.get<double>("rate", RATE);
  parser.get<int>("port", PORT);

  auto simple_publisher = std::make_shared<SimplePublisher>();
  if (!simple_publisher->ok()) {
    Log::error << "Failed to create simple_publisher." << std::endl;
    return 1;
  }

  auto sig = create_signal(SIGINT);
  simple_publisher->spin(std::move(sig));

  return 0;
}