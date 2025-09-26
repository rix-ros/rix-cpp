#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

using namespace rix::core;
using namespace rix::util;

const std::string NAME = "system_info_example";

int main(int argc, char **argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  Node node(NAME);
  if (!node.ok()) {
    Log::error << "Failed to initialize node" << std::endl;
    return 1;
  }

  rix::msg::mediator::SystemInfo system_info;
  if (!node.get_system_info(system_info)) {
    Log::error << "Failed to get system info" << std::endl;
    return 1;
  }

  Log::info << "System info: " << system_info.nodes.size() << " nodes, " << system_info.publishers.size() << " pubs, "
            << system_info.subscribers.size() << " subs, " << system_info.services.size() << " srvs, "
            << system_info.actions.size() << " acts" << std::endl;
  return 0;
}
