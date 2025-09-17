#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

const std::string name = "simple_service";

class SimpleService : public rix::core::Node {
public:
  SimpleService(const rix::ipc::Endpoint &rixhub_endpoint,
                const rix::ipc::Endpoint &service_endpoint)
      : Node(name, rixhub_endpoint) {
    auto srv =
        create_service<rix::msg::standard::UInt32, rix::msg::standard::String>(
            "/alphabet",
            std::bind(&SimpleService::callback, this, std::placeholders::_1,
                      std::placeholders::_2),
            service_endpoint);
    if (!srv->ok()) {
      rix::util::Log::error << "Failed to create service." << std::endl;
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::msg::standard::UInt32 &request,
                rix::msg::standard::String &response) {
    response.data = std::string(1, 'a' + (request.data % 26));
  }
};

int main(int argc, char **argv) {
  rix::util::Log::init(name);

  auto parser = rix::util::ArgumentParser(name, "A simple service example.");
  parser.add<std::string>("rixhub", "The RixHub endpoint.", "127.0.0.1:48104");
  parser.add<std::string>("srv_endpoint", "The service endpoint.", 'e',
                          "127.0.0.1:8002");

  if (!parser.parse(argc, argv)) {
    rix::util::Log::error << "Failed to parse arguments." << std::endl;
    return 1;
  }

  std::string rixhub_endpoint_string;
  if (!parser.get<std::string>("rixhub", rixhub_endpoint_string)) {
    rix::util::Log::error << "Failed to get rixhub argument." << std::endl;
    return 1;
  }

  std::string srv_endpoint_string;
  if (!parser.get<std::string>("srv_endpoint", srv_endpoint_string)) {
    rix::util::Log::error << "Failed to get srv_endpoint argument."
                          << std::endl;
    return 1;
  }

  auto simple_service = std::make_shared<SimpleService>(
      rix::ipc::Endpoint(rixhub_endpoint_string),
      rix::ipc::Endpoint(srv_endpoint_string));
  if (!simple_service->ok()) {
    rix::util::Log::error << "Failed to create simple_service." << std::endl;
    return 1;
  }

  auto sig = rix::ipc::create_signal(SIGINT);
  simple_service->spin(std::move(sig));

  return 0;
}