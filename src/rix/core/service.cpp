#include "rix/core/service.hpp"

namespace rix::core {

Service::~Service() {
  shutdown();
  send_message_with_opcode_no_response(
      factory_(), info_, OPCODE::SRV_DEREGISTER, rixhub_endpoint_);
}

bool Service::ok() const { return !shutdown_flag_; }

void Service::shutdown() { shutdown_flag_ = true; }

Service::SerializedCallback Service::get_callback() const { return callback_; }

Service::Service(const rix::msg::mediator::SrvInfo &info,
                 std::shared_ptr<rix::ipc::Server> server,
                 ClientFactory factory,
                 const rix::ipc::Endpoint &rixhub_endpoint)
    : info_(info), server_(server), factory_(factory),
      rixhub_endpoint_(rixhub_endpoint), shutdown_flag_(false) {
  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    rix::util::Log::error << "Server invalid!" << std::endl;
    shutdown();
    return;
  }

  if (!send_message_with_opcode(factory_(), info_, OPCODE::SRV_REGISTER,
                                rixhub_endpoint_)) {
    shutdown();
  }
}

void Service::spin_once() {
  // Check to see if a subscriber has made a connection
  if (!server_->wait_acceptable(rix::util::Duration(0.0))) {
    return;
  }

  // Accept a connection from a subscriber
  std::shared_ptr<rix::ipc::Connection> conn = server_->accept();
  if (!conn) {
    return;
  }

  rix::msg::standard::UInt32 size;
  std::vector<uint8_t> request_buffer(size.size());

  // Read the size of the request
  ssize_t bytes = conn->read(request_buffer.data(), request_buffer.size());
  size_t offset = 0;

  // If the deserialize operation fails, erase the conn
  if (!size.deserialize(request_buffer.data(), bytes, offset)) {
    rix::util::Log::warn << "Failed to read from publisher." << std::endl;
    return;
  }

  // Read the request
  request_buffer.resize(size.data);
  bytes = conn->read(request_buffer.data(), request_buffer.size());
  request_buffer.resize(bytes);

  // Invoke the callback
  std::vector<uint8_t> response_buffer;
  callback_(request_buffer, response_buffer);

  // Send response back
  conn->write(response_buffer.data(), response_buffer.size());
}

} // namespace rix::core