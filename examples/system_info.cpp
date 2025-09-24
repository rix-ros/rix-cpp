#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

using namespace rix::core;
using namespace rix::util;

const std::string NAME = "system_info_example";

int main(int argc, char **argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple publisher example.");
  parser.add<std::string>("rixhub_ip", "The IP address of the RIXHub server.", RIXHUB_IP);
  parser.add<std::string>("default_ip", "The default IP address for servers to bind to.", DEFAULT_IP);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  parser.get<std::string>("rixhub_ip", RIXHUB_IP);
  parser.get<std::string>("default_ip", DEFAULT_IP);

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
