#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

using namespace rix;

const std::string NAME = "param_server_example";

int main(int argc, char **argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  Node node(NAME);
  if (!node.ok()) {
    Log::error << "Failed to initialize node" << std::endl;
    return 1;
  }

  rix::msg::standard::Header header;
  header.frame_id = "test";
  header.seq = 1234;
  header.stamp = Time::now().to_msg();
  if (!node.set_parameter("test_param", header)) {
    Log::error << "Failed to set parameter" << std::endl;
    return 1;
  }

  rix::msg::standard::Header retrieved_header;
  if (!node.get_parameter("test_param", retrieved_header)) {
    Log::error << "Failed to get parameter" << std::endl;
    return 1;
  }
  Log::info << "Retrieved parameter: " << retrieved_header.frame_id << ", " << retrieved_header.seq << std::endl;

  return 0;
}
