#include "rix/core/service_client.hpp"

namespace rix::core {

ServiceClient::~ServiceClient() { shutdown(); }

bool ServiceClient::ok() const { return !shutdown_flag_; }
void ServiceClient::shutdown() { shutdown_flag_ = true; }
void ServiceClient::spin_once() { return; }

bool ServiceClient::call(const std::vector<uint8_t> &request,
                         std::vector<uint8_t> &response) {
  auto client = factory_();
  if (!client->connect(endpoint_)) {
    return false;
  }

  if (!client->is_writable()) {
    return false;
  }

  ssize_t bytes = client->write(request.data(), request.size());
  if (bytes != request.size()) {
    return false;
  }

  rix::msg::standard::UInt32 size;
  response.resize(size.size());
  bytes = client->read(response.data(), response.size());
  size_t offset = 0;
  if (!size.deserialize(response.data(), bytes, offset)) {
    rix::util::Log::warn << "Failed to read from rixhub." << std::endl;
    return false;
  }

  response.resize(size.data);
  bytes = client->read(response.data(), response.size());
  if (bytes != size.data) {
    return false;
  }

  return true;
}

ServiceClient::ServiceClient(const rix::msg::mediator::SrvRequest &request,
                             ClientFactory factory,
                             const rix::ipc::Endpoint &rixhub_endpoint)
    : request_(request), factory_(factory), shutdown_flag_(false) {
  rix::msg::mediator::SrvResponse response;
  if (!send_message_with_opcode_and_response(factory_(), request, response,
                                             OPCODE::SRV_REQUEST,
                                             rixhub_endpoint)) {
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

} // namespace rix::core