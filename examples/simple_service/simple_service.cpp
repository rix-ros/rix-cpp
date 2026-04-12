#include "simple_service.hpp"

SimpleService::SimpleService(int port) : Node(NAME) {
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  auto srv = create_service("/alphabet", &SimpleService::callback, this, Endpoint(DEFAULT_IP, port));
  if (!srv->ok()) {
    Log::error << "Failed to create service." << std::endl;
    shutdown();
    return;
  }
}

void SimpleService::callback(const rix::std_msgs::UInt32& request, rix::std_msgs::String& response) {
  response.data = std::string(1, 'a' + (request.data % 26));
}
