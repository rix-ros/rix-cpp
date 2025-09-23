#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

#include <sstream>

const std::string name = "simple_subscriber";

class SimpleSubscriber : public rix::core::Node {
public:
  // Initialize the Node with a name and the RixHub endpoint
  SimpleSubscriber(const rix::ipc::Endpoint &rixhub_endpoint, const rix::ipc::Endpoint &subscriber_endpoint)
      : Node(name, rixhub_endpoint) {

    // If the Node failed to initialize, then ok() will return false
    if (!ok()) {
      rix::util::Log::error << "Failed to create node." << std::endl;
      shutdown();
      return;
    }

    // Create a subscriber on topic /chatter with message type Header
    auto sub = create_subscriber<rix::msg::standard::Header>(
        "/chatter", std::bind(&SimpleSubscriber::callback, this, std::placeholders::_1), subscriber_endpoint);

    // If the subscriber failed to initialize, then ok() will return false
    if (!sub->ok()) {
      rix::util::Log::error << "Failed to create subscriber." << std::endl;
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::msg::standard::Header &msg) {
    // static uint32_t prev_seq = 0;
    // static size_t msg_count = 0;
    // static size_t lost_count = 0;

    // msg_count++;

    // if (prev_seq == 0) {
    //   prev_seq = msg.seq;
    //   return;
    // }

    // if (msg.seq > prev_seq + 1) {
    //   lost_count += (msg.seq - prev_seq - 1);
    //   rix::util::Log::warn << "Lost " << lost_count << " / " << msg_count << " messages." << std::endl;
    // }

    // prev_seq = msg.seq;

    std::stringstream ss;
    ss << "Received message: \n"
       << "seq: " << msg.seq << "\n"
       << "stamp: " << msg.stamp.sec << "." << msg.stamp.nsec << "\n"
       << "frame_id: " << msg.frame_id << "\n";
    rix::util::Log::info << ss.str() << std::endl;
  }
};

int main(int argc, char **argv) {
  rix::util::Log::init(name);

  auto parser = rix::util::ArgumentParser(name, "A simple subscriber example.");
  parser.add_parser<rix::ipc::Endpoint>(rix::core::parse_endpoint);
  parser.add<rix::ipc::Endpoint>("rixhub", "The RixHub endpoint.", rix::ipc::Endpoint("127.0.0.1", 48104));
  parser.add<rix::ipc::Endpoint>("sub_endpoint", "The subscriber endpoint.", 'e',
                                 rix::ipc::Endpoint("127.0.0.1", 8000));

  if (!parser.parse(argc, argv)) {
    rix::util::Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  rix::ipc::Endpoint rixhub_ep;
  if (!parser.get<rix::ipc::Endpoint>("rixhub", rixhub_ep)) {
    rix::util::Log::error << "Failed to get rixhub argument." << std::endl;
    return 1;
  }

  rix::ipc::Endpoint sub_ep;
  if (!parser.get<rix::ipc::Endpoint>("sub_endpoint", sub_ep)) {
    rix::util::Log::error << "Failed to get sub_endpoint argument." << std::endl;
    return 1;
  }

  auto simple_subscriber = std::make_shared<SimpleSubscriber>(rixhub_ep, sub_ep);
  if (!simple_subscriber->ok()) {
    rix::util::Log::error << "Failed to create simple_subscriber." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  simple_subscriber->spin(std::move(sig));

  return 0;
}