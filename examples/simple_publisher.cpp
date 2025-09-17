#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

const std::string name = "simple_publisher";

class SimplePublisher : public rix::core::Node {
public:
  // Initialize the Node with a name and the RixHub endpoint
  SimplePublisher(const rix::ipc::Endpoint &rixhub_endpoint,
                  const rix::ipc::Endpoint &publisher_endpoint, double rate)
      : Node(name, rixhub_endpoint) {

    // If the Node failed to initialize, then ok() will return false
    if (!ok()) {
      rix::util::Log::error << "Failed to create node." << std::endl;
      shutdown();
      return;
    }

    // Create a publisher on topic /chatter with message type Header
    pub = create_publisher<rix::msg::standard::Header>("/chatter",
                                                       publisher_endpoint);

    // If the publisher failed to initialize, then ok() will return false
    if (!pub->ok()) {
      rix::util::Log::error << "Failed to create publisher." << std::endl;
      shutdown();
      return;
    }

    // Initialize our message parameters
    message.frame_id = "Hello, world!";
    message.seq = 0;

    timer = create_timer(rix::util::Duration(1.0 / rate),
                         std::bind(&SimplePublisher::timer_callback, this,
                                   std::placeholders::_1));
  }

private:
  std::shared_ptr<rix::core::Publisher> pub;
  std::shared_ptr<rix::core::Timer> timer;
  rix::msg::standard::Header message;

  /**
   * @brief Timer callback that is invoked by the Node at 1.0 Hz during spin
   *
   */
  void timer_callback(const rix::core::Timer::Event &event) {
    message.frame_id = "Hello, world!";
    message.seq += 1;
    message.stamp = rix::util::Time::now().to_msg();
    pub->publish(message);
  }
};

int main(int argc, char **argv) {
  rix::util::Log::init(name);

  auto parser = rix::util::ArgumentParser(name, "A simple publisher example.");
  parser.add<std::string>("rixhub", "The RixHub endpoint.", "127.0.0.1:48104");
  parser.add<std::string>("pub_endpoint", "The publisher endpoint.", 'e',
                          "127.0.0.1:8001");
  parser.add<double>("rate", "The publish rate in Hz.", 'r', 1);

  if (!parser.parse(argc, argv)) {
    rix::util::Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  std::string rixhub_endpoint_string;
  if (!parser.get<std::string>("rixhub", rixhub_endpoint_string)) {
    rix::util::Log::error << "Failed to get rixhub argument." << std::endl;
    return 1;
  }

  std::string pub_endpoint_string;
  if (!parser.get<std::string>("pub_endpoint", pub_endpoint_string)) {
    rix::util::Log::error << "Failed to get pub_endpoint argument."
                          << std::endl;
    return 1;
  }

  double rate;
  if (!parser.get<double>("rate", rate)) {
    rix::util::Log::error << "Failed to get rate argument." << std::endl;
    return 1;
  }

  auto simple_publisher = std::make_shared<SimplePublisher>(
      rix::ipc::Endpoint(rixhub_endpoint_string),
      rix::ipc::Endpoint(pub_endpoint_string), rate);
  if (!simple_publisher->ok()) {
    rix::util::Log::error << "Failed to create simple_publisher." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  simple_publisher->spin(std::move(sig));

  return 0;
}