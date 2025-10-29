#include "simple_subscriber.hpp"

// Initialize the Node with a name and the RixHub endpoint
SimpleSubscriber::SimpleSubscriber(int port) : Node(NAME) {

  // If the Node failed to initialize, then ok() will return false
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  // Create a subscriber on topic /chatter
  // Pass member function pointer and 'this' - no lambda or std::bind needed!
  auto sub = create_subscriber("/chatter", &SimpleSubscriber::callback, this, Endpoint(DEFAULT_IP, port));

  // If the subscriber failed to initialize, then ok() will return false
  if (!sub->ok()) {
    Log::error << "Failed to create subscriber." << std::endl;
    shutdown();
    return;
  }
}

void SimpleSubscriber::callback(const rix::std_msgs::Header& msg) {
  std::stringstream ss;
  ss << "Received message: \n"
     << "seq: " << msg.seq << "\n"
     << "stamp: " << msg.stamp.sec << "." << msg.stamp.nsec << "\n"
     << "frame_id: " << msg.frame_id << "\n";
  Log::info << ss.str() << std::endl;
}