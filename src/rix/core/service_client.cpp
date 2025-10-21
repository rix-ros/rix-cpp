#include "rix/core/service_client.hpp"

namespace rix {

ServiceClient::ServiceClient(const msg::mediator::SrvRequest& request,
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

  msg::mediator::SrvResponse response;
  msg::mediator::Operation op;
  if (!client->recv_message(op, response)) {
    shutdown();
    return;
  }

  if (op.opcode != OPCODE::SRV_RESPONSE) {
    shutdown();
    return;
  }

  if (response.error) {
    shutdown();
    return;
  }

  endpoint_.address = response.srv_info.endpoint.address;
  endpoint_.port = response.srv_info.endpoint.port;

#ifdef RIX_MULTITHREADED
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

ServiceClient::~ServiceClient() {
#ifdef RIX_MULTITHREADED
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif
}

void ServiceClient::on_spin() {}

bool ServiceClient::call(const msg::Message& request, msg::Message& response) {
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

  msg::mediator::Operation op;
  if (!client->recv_message(op, response)) {
    return false;
  }

  if (op.opcode != OPCODE::SRV_RESPONSE_MESSAGE) {
    return false;
  }

  return true;
}

} // namespace rix