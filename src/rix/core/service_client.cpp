#include "rix/core/service_client.hpp"
#include "rix/sys_msgs/SrvResponse.hpp"

namespace rix {

ServiceClient::ServiceClient(const sys_msgs::SrvRequest& request,
                             SocketFactory socket_factory,
                             const Endpoint& rixhub_endpoint)
    : request_(request), socket_factory_(socket_factory) {
  auto client = socket_factory_();
  if (!client) {
    shutdown();
    return;
  }

  if (!client->connect(rixhub_endpoint)) {
    shutdown();
    return;
  }

  if (!client->send_message(OPCODE::SRV_REQUEST, request)) {
    shutdown();
    return;
  }

  sys_msgs::SrvResponse response;
  sys_msgs::Operation operation;
  if (!client->recv_message(operation, response)) {
    shutdown();
    return;
  }

  if (operation.opcode != OPCODE::SRV_RESPONSE) {
    shutdown();
    return;
  }

  if (response.error) {
    shutdown();
    return;
  }

  endpoint_.address = response.srv_info.endpoint.address;
  endpoint_.port = response.srv_info.endpoint.port;

  if (MULTITHREADED) {
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

ServiceClient::~ServiceClient() {
  if (MULTITHREADED) {
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

void ServiceClient::on_spin() {}

bool ServiceClient::call(const Message& request, Message& response) {
  if (!ok()) {
    return false;
  }

  auto client = socket_factory_();
  if (!client) {
    return false;
  }

  if (!client->connect(endpoint_)) {
    return false;
  }

  if (!client->send_message(OPCODE::SRV_REQUEST_MESSAGE, request)) {
    return false;
  }

  sys_msgs::Operation operation;
  if (!client->recv_message(operation, response)) {
    return false;
  }

  if (operation.opcode != OPCODE::SRV_RESPONSE_MESSAGE) {
    return false;
  }

  return true;
}

} // namespace rix