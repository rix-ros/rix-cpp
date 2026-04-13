#include "rix/core/service_client.hpp"

#include "rix/sys_msgs/SrvResponse.hpp"

namespace rix {
namespace detail {

ServiceClientImpl::ServiceClientImpl(const sys_msgs::SrvRequest& request, const Endpoint& rixhub_endpoint)
    : request_(request), factory_(get_transport_factory(static_cast<Protocol>(request.protocol))) {
  auto client = factory_.create_stream(rixhub_endpoint, true);
  if (!client) {
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

ServiceClientImpl::~ServiceClientImpl() {
  if (MULTITHREADED) {
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

void ServiceClientImpl::on_spin() {}

bool ServiceClientImpl::call(const Message& request, Message& response) {
  if (!ok()) {
    return false;
  }

  auto client = factory_.create_stream(endpoint_, true);
  if (!client) {
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

} // namespace detail
} // namespace rix