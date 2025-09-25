#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix::core;
using namespace rix::util;
using namespace rix::ipc;

const std::string NAME = "simple_service";
int PORT = 8002;

class SimpleService : public rix::core::Node {
public:
  SimpleService() : Node(NAME) {
    auto srv = create_service<rix::msg::standard::UInt32, rix::msg::standard::String>(
        "/alphabet", std::bind(&SimpleService::callback, this, std::placeholders::_1, std::placeholders::_2),
        Endpoint(DEFAULT_IP, PORT));
    if (!srv->ok()) {
      Log::error << "Failed to create service." << std::endl;
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::msg::standard::UInt32 &request, rix::msg::standard::String &response) {
    response.data = std::string(1, 'a' + (request.data % 26));
  }
};

int main(int argc, char **argv) {
  Log::init(NAME);
  Log::set_log_level(Log::Level::DEBUG);

  auto parser = ArgumentParser(NAME, "A simple subscriber example.");
  parser.add<std::string>("rixhub_ip", "The IP address of the RIXHub server.", RIXHUB_IP);
  parser.add<std::string>("default_ip", "The default IP address for servers to bind to.", DEFAULT_IP);
  parser.add<int>("port", "The port for the subscriber server.", 'p', PORT);

  if (!parser.parse(argc, argv)) {
    Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  parser.get<std::string>("rixhub_ip", RIXHUB_IP);
  parser.get<std::string>("default_ip", DEFAULT_IP);
  parser.get<int>("port", PORT);

  auto simple_service = std::make_shared<SimpleService>();
  if (!simple_service->ok()) {
    Log::error << "Failed to create simple_service." << std::endl;
    return 1;
  }

  auto sig = create_signal(SIGINT);
  simple_service->spin(std::move(sig));

  return 0;
}