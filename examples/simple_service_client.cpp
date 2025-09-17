#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix::util;
using namespace rix::core;

const std::string name = "simple_service_client";

int main(int argc, char **argv) {
  rix::util::Log::init(name);

  auto parser = rix::util::ArgumentParser(name, "A simple service example.");
  parser.add<std::string>("rixhub", "The RixHub endpoint.", "127.0.0.1:48104");
  parser.add<double>("rate", "The call rate in Hz.", 'r', 1);

  if (!parser.parse(argc, argv)) {
    rix::util::Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  std::string rixhub_endpoint_string;
  if (!parser.get<std::string>("rixhub", rixhub_endpoint_string)) {
    rix::util::Log::error << "Failed to get rixhub argument." << std::endl;
    return 1;
  }

  double rate;
  if (!parser.get<double>("rate", rate)) {
    rix::util::Log::error << "Failed to get rate argument." << std::endl;
    return 1;
  }

  Node node("simple_service_client",
            rix::ipc::Endpoint(rixhub_endpoint_string));
  if (!node.ok()) {
    Log::error << "Failed to create node." << std::endl;
    return 1;
  }

  auto srv_cli =
      node.create_service_client<rix::msg::standard::UInt32,
                                 rix::msg::standard::String>("/alphabet");
  if (!srv_cli->ok()) {
    Log::error << "Failed to create service client." << std::endl;
    return 1;
  }

  Rate r(rate);
  for (uint32_t i = 0; i < 26 && node.ok(); i++) {
    rix::msg::standard::UInt32 req;
    req.data = i;
    rix::msg::standard::String res;
    srv_cli->call(req, res);
    Log::info << "Received response: " << res.data << std::endl;
    r.sleep();
  }
}
