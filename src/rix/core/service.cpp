#include "rix/core/service.hpp"
#include "rix/sys_msgs/Status.hpp"

namespace rix {

Service::Service(const sys_msgs::SrvInfo& info, TransportFactory socket_factory, const Endpoint& rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), rixhub_endpoint_(rixhub_endpoint), registered_flag_(false),
      request_instance_(nullptr), response_instance_(nullptr) {

  server_ = socket_factory_.create_acceptor(Endpoint(info_.endpoint.address, info_.endpoint.port));

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
  auto client = socket_factory_.create_stream(rixhub_endpoint_, true);

  if (!client->send_message(OPCODE::SRV_REGISTER, info_)) {
    shutdown();
    return;
  }

  sys_msgs::Operation operation;
  sys_msgs::Status status;
  if (!client->recv_message(operation, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;

  Log::debug << "Service created for \"" << info_.name << "\"." << std::endl;

  if (MULTITHREADED) {
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

Service::~Service() {
  if (registered_flag_) {
    auto client = socket_factory_.create_stream(rixhub_endpoint_, true);
    if (!client) {
      return;
    }
    client->send_message(OPCODE::SRV_DEREGISTER, info_);
  }

  if (MULTITHREADED) {
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }

  Log::debug << "Service for \"" << info_.name << "\" destroyed." << std::endl;
}

void Service::on_spin() {
  if (!callback_) {
    return;
  }
  std::lock_guard<std::mutex> lock(callback_mutex_);

  // Check to see if a subscriber has made a connection
  if (!server_->is_readable())
    return;

  // Accept a connection from a subscriber
  auto conn = server_->accept();
  if (!conn) {
    return;
  }

  // Read the request message
  sys_msgs::Operation operation;
  if (!conn->recv_message(operation, *request_instance_))
    return;

  if (operation.opcode != OPCODE::SRV_REQUEST_MESSAGE) {
    return;
  }

  // Invoke the callback
  callback_(*request_instance_, *response_instance_);

  // Send response back
  conn->send_message(OPCODE::SRV_RESPONSE_MESSAGE, *response_instance_);

  Log::debug << "Processed service request for \"" << info_.name << "\"." << std::endl;
}

} // namespace rix