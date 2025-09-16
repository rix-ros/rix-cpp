#include "rix/core/publisher.hpp"

namespace rix::core {

Publisher::Publisher(const rix::msg::mediator::PubInfo &info,
                     std::shared_ptr<rix::ipc::Server> server,
                     ClientFactory factory, rix::ipc::Endpoint rixhub_endpoint)
    : info_(info), server_(server), factory_(factory),
      rixhub_endpoint_(rixhub_endpoint), shutdown_flag_(false) {
  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    rix::util::Log::error << "Server invalid!" << std::endl;
    shutdown();
    return;
  }

  // Register publisher with rixhub
  if (!send_message_with_opcode(factory_(), info_, OPCODE::PUB_REGISTER,
                                rixhub_endpoint_)) {
    shutdown();
  }
}

Publisher::~Publisher() {
  shutdown();

  // Deregister publisher with rixhub
  send_message_with_opcode_no_response(
      factory_(), info_, OPCODE::PUB_DEREGISTER, rixhub_endpoint_);
}

bool Publisher::ok() const { return !shutdown_flag_; }

void Publisher::shutdown() { shutdown_flag_ = true; }

void Publisher::publish(const rix::msg::Message &msg) {
  // Ensure that the message hash matches the one that the publisher
  // was created with
  if (msg.hash() != info_.topic_info.message_hash) {
    rix::util::Log::warn << "Message type mismatch in publish." << std::endl;
    return;
  }

  // Serialize the message with size prefix
  rix::msg::standard::UInt32 size;
  size.data = msg.size();
  std::vector<uint8_t> buffer(size.size() + size.data);
  size_t offset = 0;
  size.serialize(buffer.data(), offset);
  msg.serialize(buffer.data(), offset);

  // Send the message to each current connection
  std::lock_guard<std::mutex> lock(connections_mutex_);
  auto it = connections_.begin();
  while (it != connections_.end()) {
    auto conn = *it;

    // If the connection is not writable, erase from the list
    if (!conn->is_writable()) {
      it = connections_.erase(it);
      continue;
    }

    // Send the message to the subscriber
    size_t bytes_written = 0;
    while (bytes_written < buffer.size()) {
      ssize_t bytes = conn->write(buffer.data() + bytes_written,
                                  buffer.size() - bytes_written);
      if (bytes <= 0) {
        break;
      }
      bytes_written += bytes;
    }
    it++;
  }
}

size_t Publisher::get_subscriber_count() const {
  std::lock_guard<std::mutex> guard(connections_mutex_);
  return connections_.size();
}

void Publisher::spin_once() {
  // Check to see if a subscriber has made a connection
  if (!server_->wait_acceptable(rix::util::Duration(0.0))) {
    return;
  }

  // Accept a connection from a subscriber
  std::shared_ptr<rix::ipc::Connection> conn = server_->accept();
  if (!conn) {
    return;
  }

  // Store the connection
  std::lock_guard<std::mutex> guard(connections_mutex_);
  connections_.insert(conn);
}

} // namespace rix::core