#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix::util;
using namespace rix::core;
using namespace rix::ipc;

const std::string NAME = "simple_service_client";
double RATE = 1.0; // Hz

int main(int argc, char **argv) {
  Log::init(NAME);

  auto parser = ArgumentParser(NAME, "A simple publisher example.");
  parser.add<double>("rate", "The publish rate in Hz.", 'r', RATE);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }
  parser.get<double>("rate", RATE);

  Node node("simple_service_client");
  if (!node.ok()) {
    Log::error << "Failed to create node." << std::endl;
    return 1;
  }

  auto srv_cli = node.create_service_client<rix::msg::standard::UInt32, rix::msg::standard::String>("/alphabet");
  if (!srv_cli->ok()) {
    Log::error << "Failed to create service client." << std::endl;
    return 1;
  }

  uint32_t i = 0;
  auto timer = node.create_timer(Duration(1.0 / RATE), [srv_cli, &i](const rix::core::Timer::Event &) {
    rix::msg::standard::UInt32 req;
    req.data = i;
    rix::msg::standard::String res;
    srv_cli->call(req, res);
    Log::info << "Received response: " << res.data << std::endl;
    i++;
  });
  if (!timer->ok()) {
    Log::error << "Failed to create timer." << std::endl;
    return 1;
  }

  node.spin();
}
