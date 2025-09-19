#include "rix/core/service.hpp"

namespace rix::core {

Service::Service(const rix::msg::mediator::SrvInfo &info, SocketFactory socket_factory,
                 const rix::ipc::Endpoint &rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), rixhub_endpoint_(rixhub_endpoint), shutdown_flag_(true),
      registered_flag_(false), request_instance_(nullptr), response_instance_(nullptr) {

  server_ = socket_factory_();
  server_->set_reuse_address(true);
  server_->bind(rix::ipc::Endpoint(info_.endpoint.address, info_.endpoint.port));
  server_->listen(rix::ipc::MAX_CONN);

  // Ensure server was intitialized properly
  if (server_->is_exception())
    return;

  auto server_endpoint = server_->local_endpoint();
  // Update the endpoint in case the port was set to 0 (ephemeral)
  info_.endpoint.address = server_endpoint.address;
  info_.endpoint.port = server_endpoint.port;

  // Register service with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_))
    return;

  if (!client->send_message(OPCODE::SRV_REGISTER, info_))
    return;

  rix::msg::mediator::Operation op;
  rix::msg::mediator::Status status;
  if (!client->recv_message(op, status))
    return;
  if (status.error)
    return;

  shutdown_flag_ = false;
  registered_flag_ = true;
}

Service::~Service() {
  if (registered_flag_) {
    auto client = socket_factory_();
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::SRV_DEREGISTER, info_);
    }
  }
}

bool Service::ok() const { return !shutdown_flag_; }

void Service::shutdown() { shutdown_flag_ = true; }

void Service::spin_once() {
  // Check to see if a subscriber has made a connection
  if (!server_->is_readable())
    return;

  // Accept a connection from a subscriber
  auto conn = server_->accept();
  if (!conn) {
    return;
  }

  // Read the request message
  rix::msg::mediator::Operation op;
  if (!conn->recv_message(op, *request_instance_))
    return;

  if (op.opcode != OPCODE::SRV_REQUEST_MESSAGE) {
    return;
  }

  // Invoke the callback
  callback_(*request_instance_, *response_instance_);

  // Send response back
  conn->send_message(OPCODE::SRV_RESPONSE_MESSAGE, *response_instance_);
}

} // namespace rix::core