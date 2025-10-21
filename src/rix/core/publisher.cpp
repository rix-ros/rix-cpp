#include "rix/core/publisher.hpp"

namespace rix {

Publisher::Publisher(const msg::mediator::PubInfo& info, SocketFactory factory, Endpoint rixhub_endpoint)
    : info_(info), socket_factory_(factory), rixhub_endpoint_(rixhub_endpoint), registered_flag_(false) {

  server_ = socket_factory_();
  if (!server_) {
    shutdown();
    return;
  }

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

  // Register publisher with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::PUB_REGISTER, info_)) {
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

  Log::debug << "Publisher created on topic \"" << info_.topic_info.name << "\"." << std::endl;

#ifdef RIX_MULTITHREADED
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

Publisher::~Publisher() {
  // Deregister publisher with rixhub
  if (registered_flag_) {
    auto client = socket_factory_();
    if (!client) {
      return;
    }
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::PUB_DEREGISTER, info_);
    }
  }
  Log::debug << "Publisher on topic \"" << info_.topic_info.name << "\" destroyed." << std::endl;

#ifdef RIX_MULTITHREADED
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif
}

void Publisher::publish(const msg::Message& msg) {
  if (!ok()) {
    return;
  }
  // Ensure that the message hash matches the one that the publisher
  // was created with
  if (msg.hash() != info_.topic_info.message_hash) {
    Log::warn << "Message type mismatch in publish." << std::endl;
    return;
  }

  std::lock_guard<std::mutex> lock(connections_mutex_);
  if (connections_.empty()) {
    return;
  }

  std::vector<std::shared_ptr<GenericSocket>> sockets(connections_.begin(), connections_.end());
  std::vector<std::shared_ptr<GenericSocket>> writable;

  if (GenericSocket::get_poller()) {
    std::vector<std::shared_ptr<GenericSocket>> exceptional;
    GenericSocket::poll(sockets, Duration(0.0), PollFlag::WRITE, writable, exceptional);

    // Remove any clients that have exceptions
    for (const auto& conn : exceptional) {
      connections_.erase(conn);
      Log::debug << "Removed exceptional subscriber from topic \"" << info_.topic_info.name << "\"." << std::endl;
    }
  } else {
    // Fallback if poller is not available
    for (const auto& sock : sockets) {
      if (sock->is_writable()) {
        writable.push_back(sock);
      } else {
        connections_.erase(sock);
        Log::debug << "Removed exceptional subscriber from topic \"" << info_.topic_info.name << "\"." << std::endl;
      }
    }
  }

  // Send the message to each current connection
  auto it = writable.begin();
  while (it != writable.end()) {
    auto conn = *it;

    // Send the message to the subscriber
    if (!conn->send_message(OPCODE::PUB_MESSAGE, msg)) {
      connections_.erase(conn);
      it++;
      Log::debug << "Removed exceptional subscriber from topic \"" << info_.topic_info.name << "\"." << std::endl;
      continue;
    }
    it++;
  }
  Log::debugv << "Published message on topic \"" << info_.topic_info.name << "\"." << std::endl;
}

size_t Publisher::get_subscriber_count() const {
  std::lock_guard<std::mutex> guard(connections_mutex_);
  return connections_.size();
}

void Publisher::on_spin() {
  // Check to see if a subscriber has made a connection
  if (!server_->wait_readable(Duration(1.0))) {
    return;
  }

  // Accept a connection from a subscriber
  Endpoint remote_endpoint;
  auto conn = server_->accept(remote_endpoint);
  if (!conn) {
    return;
  }

  // Store the connection
  std::lock_guard<std::mutex> guard(connections_mutex_);
  Log::debug << "Accepted new subscriber at \"" << remote_endpoint.address << ":" << remote_endpoint.port
             << "\" on topic \"" << info_.topic_info.name << "\"." << std::endl;
  connections_.insert(conn);
}

} // namespace rix