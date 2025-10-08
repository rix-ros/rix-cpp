#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

#include <sstream>

using namespace rix;

const std::string NAME = "simple_subscriber";
int               PORT = 8000;

class SimpleSubscriber : public Node {
public:
  // Initialize the Node with a name and the RixHub endpoint
  SimpleSubscriber() : Node(NAME) {

    // If the Node failed to initialize, then ok() will return false
    if (!ok()) {
      Log::error << "Failed to create node." << std::endl;
      return;
    }

    // Create a subscriber on topic /chatter
    // Pass member function pointer and 'this' - no lambda or std::bind needed!
    auto sub = create_subscriber(
        "/chatter",
        &SimpleSubscriber::callback,
        this,
        Endpoint(DEFAULT_IP, PORT));

    // If the subscriber failed to initialize, then ok() will return false
    if (!sub->ok()) {
      Log::error << "Failed to create subscriber." << std::endl;
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::msg::standard::Header& msg) {
    std::stringstream ss;
    ss << "Received message: \n"
       << "seq: " << msg.seq << "\n"
       << "stamp: " << msg.stamp.sec << "." << msg.stamp.nsec << "\n"
       << "frame_id: " << msg.frame_id << "\n";
    Log::info << ss.str() << std::endl;
  }
};

int main(int argc, char** argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple subscriber example.");
  parser.add<int>("port", "The port for the subscriber server.", 'p', PORT);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }
  parser.get<int>("port", PORT);

  auto simple_subscriber = std::make_shared<SimpleSubscriber>();
  if (!simple_subscriber->ok()) {
    Log::error << "Failed to create simple_subscriber." << std::endl;
    return 1;
  }

  simple_subscriber->spin();

  return 0;
}