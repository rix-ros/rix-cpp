#include "rix/core/service.hpp"

namespace rix {

Service::Service(const msg::mediator::SrvInfo& info,
                 SocketFactory socket_factory,
                 const Endpoint& rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), rixhub_endpoint_(rixhub_endpoint),
      registered_flag_(false), request_instance_(nullptr), response_instance_(nullptr) {

  server_ = socket_factory_();
  server_->set_reuse_address(true);
  server_->bind(Endpoint(info_.endpoint.address, info_.endpoint.port));
  server_->listen(MAX_CONN);

  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    shutdown();
    return;
  }

  auto server_endpoint = server_->local_endpoint();
  // Update the endpoint in case the port was set to 0 (ephemeral)
  info_.endpoint.address = server_endpoint.address;
  info_.endpoint.port = server_endpoint.port;

  // Register service with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    shutdown();
    return;
  }

  if (!client->send_message(OPCODE::SRV_REGISTER, info_)) {
    shutdown();
    return;
  }

  msg::mediator::Operation op;
  msg::mediator::Status status;
  if (!client->recv_message(op, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;

  Log::debug << "Service created for \"" << info_.name << "\"." << std::endl;

#ifdef RIX_MULTITHREADED
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

Service::~Service() {
  if (registered_flag_) {
    auto client = socket_factory_();
    if (!client) {
      return;
    }
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::SRV_DEREGISTER, info_);
    }
  }

#ifdef RIX_MULTITHREADED
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif

  Log::debug << "Service for \"" << info_.name << "\" destroyed." << std::endl;
}

void Service::on_spin() {
  if (!callback_) {
    return;
  }
  std::lock_guard lock(callback_mutex_);

  // Check to see if a subscriber has made a connection
  if (!server_->is_readable())
    return;

  // Accept a connection from a subscriber
  auto conn = server_->accept();
  if (!conn) {
    return;
  }

  // Read the request message
  msg::mediator::Operation op;
  if (!conn->recv_message(op, *request_instance_))
    return;

  if (op.opcode != OPCODE::SRV_REQUEST_MESSAGE) {
    return;
  }

  // Invoke the callback
  callback_(*request_instance_, *response_instance_);

  // Send response back
  conn->send_message(OPCODE::SRV_RESPONSE_MESSAGE, *response_instance_);

  Log::debug << "Processed service request for \"" << info_.name << "\"." << std::endl;
}

} // namespace rix