#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix::util;
using namespace rix::core;

int main(int argc, char **argv) {
  Node node("simple_service_client",
            rix::ipc::Endpoint("127.0.0.1", rix::core::RIXHUB_PORT));
  if (!node.ok()) {
    Log::error << "Failed to initialize node." << std::endl;
    return 1;
  }

  auto srv_cli =
      node.create_service_client<rix::msg::standard::UInt32,
                                 rix::msg::standard::String>("/alphabet");
  if (!srv_cli->ok()) {
    Log::error << "Failed to create service client." << std::endl;
    return 1;
  }

  Rate rate(1);
  for (uint32_t i = 0; i < 26 && node.ok(); i++) {
    rix::msg::standard::UInt32 req;
    req.data = i;
    rix::msg::standard::String res;
    srv_cli->call(req, res);
    Log::info << "Received response: " << res.data << std::endl;
    rate.sleep();
  }
}
