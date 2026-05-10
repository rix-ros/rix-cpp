#include "simple_subscriber.hpp"

SimpleSubscriber::SimpleSubscriber(int port) : Node(NAME) {

  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  auto sub = create_subscriber("/chatter", &SimpleSubscriber::callback, this, Endpoint(DEFAULT_IP, port));

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