#include "rix/core/service_client.hpp"

namespace rix::core {

ServiceClient::ServiceClient(const rix::msg::mediator::SrvRequest &request, SocketFactory socket_factory,
                             const rix::ipc::Endpoint &rixhub_endpoint)
    : request_(request), socket_factory_(socket_factory) {
  rix::msg::mediator::SrvResponse response;

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint)) {
    shutdown();
    return;
  }

  if (!client->send_message(OPCODE::SRV_REQUEST, request)) {
    shutdown();
    return;
  }

  rix::msg::mediator::Operation op;
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
}

ServiceClient::~ServiceClient() {}

void ServiceClient::spin_once() {}

bool ServiceClient::call(const rix::msg::Message &request, rix::msg::Message &response) {
  auto client = socket_factory_();
  if (!client->connect(endpoint_))
    return false;

  if (!client->send_message(OPCODE::SRV_REQUEST_MESSAGE, request))
    return false;

  rix::msg::mediator::Operation op;
  if (!client->recv_message(op, response))
    return false;

  if (op.opcode != OPCODE::SRV_RESPONSE_MESSAGE)
    return false;

  return true;
}

} // namespace rix::core