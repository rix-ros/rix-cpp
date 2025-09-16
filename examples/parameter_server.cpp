#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

using namespace rix::core;
using namespace rix::util;

int main(int argc, char **argv) {
  Node node("param_server",
            rix::ipc::Endpoint("127.0.0.1", rix::core::RIXHUB_PORT));
  if (!node.ok()) {
    Log::error << "Failed to initialize node" << std::endl;
    return 1;
  }

  rix::msg::standard::Header header;
  header.frame_id = "test";
  header.seq = 1234;
  header.stamp = rix::util::Time::now().to_msg();
  if (!node.set_parameter("test_param", header)) {
    Log::error << "Failed to set parameter" << std::endl;
    return 1;
  }

  rix::msg::standard::Header retrieved_header;
  if (!node.get_parameter("test_param", retrieved_header)) {
    Log::error << "Failed to get parameter" << std::endl;
    return 1;
  }
  Log::info << "Retrieved parameter: " << retrieved_header.frame_id << ", "
            << retrieved_header.seq << std::endl;

  Log::info << "Making System Info request ..." << std::endl;
  rix::msg::mediator::SystemInfo system_info;
  if (!node.get_system_info(system_info)) {
    Log::error << "Failed to get system info" << std::endl;
    return 1;
  }
  Log::info << "System info: " << system_info.nodes.size() << " nodes, "
            << system_info.publishers.size() << " pubs, "
            << system_info.subscribers.size() << " subs, "
            << system_info.services.size() << " srvs, "
            << system_info.actions.size() << " acts" << std::endl;
  return 0;
}
