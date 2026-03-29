#include "parameter_server.hpp"

ParameterServer::ParameterServer(int port) : Node(NAME) {
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  rix::std_msgs::Header header;
  header.frame_id = "test";
  header.seq = 1234;
  header.stamp = Time::now().to_msg();
  if (!set_parameter("test_param", header)) {
    Log::error << "Failed to set parameter" << std::endl;
    shutdown();
  }

  rix::std_msgs::Header retrieved_header;
  if (!get_parameter("test_param", retrieved_header)) {
    Log::error << "Failed to get parameter" << std::endl;
    shutdown();
  }
  Log::info << "Retrieved parameter: " << retrieved_header.frame_id << ", " << retrieved_header.seq << std::endl;
}
