#include "rix/core/publisher.hpp"

namespace rix::core {

Publisher::Publisher(const rix::msg::mediator::PubInfo &info, SocketFactory factory, rix::ipc::Endpoint rixhub_endpoint)
    : info_(info), socket_factory_(factory), rixhub_endpoint_(rixhub_endpoint), shutdown_flag_(true),
      registered_flag_(false) {

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

  // Register publisher with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_))
    return;
  if (!client->send_message(OPCODE::PUB_REGISTER, info_))
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

Publisher::~Publisher() {
  shutdown();

  // Deregister publisher with rixhub
  if (registered_flag_) {
    auto client = socket_factory_();
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::PUB_DEREGISTER, info_);
    }
  }
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
    if (!conn->send_message(OPCODE::PUB_MESSAGE, msg)) {
      it = connections_.erase(it);
      continue;
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
  if (!server_->wait_readable(rix::util::Duration(0.0))) {
    return;
  }

  // Accept a connection from a subscriber
  auto conn = server_->accept();
  if (!conn) {
    return;
  }

  // Store the connection
  std::lock_guard<std::mutex> guard(connections_mutex_);
  connections_.insert(conn);
}

} // namespace rix::core