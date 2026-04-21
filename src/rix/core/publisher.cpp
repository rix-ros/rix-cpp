#include "rix/core/publisher.hpp"

#include "rix/sys_msgs/Status.hpp"

namespace rix {
namespace detail {

PublisherImpl::PublisherImpl(const sys_msgs::PubInfo& info, Endpoint rixhub_endpoint)
    : info_(info), factory_(get_transport_factory(static_cast<Protocol>(info.protocol))),
      tcp_factory_(get_transport_factory(Protocol::TCP)), rixhub_endpoint_(rixhub_endpoint), registered_flag_(false) {
  server_ = factory_.create_acceptor(Endpoint(info_.endpoint.address, info_.endpoint.port));
  if (!server_) {
    shutdown();
    return;
  }

  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    shutdown();
    return;
  }

  auto server_endpoint = server_->local_endpoint();
  // Update the endpoint in case the port was set to 0 (ephemeral)
  info_.endpoint.address = server_endpoint.address;
  info_.endpoint.port = server_endpoint.port;

  // Register publisher with rixhub (always TCP - rixhub only speaks TCP)
  auto client = tcp_factory_.create_stream(rixhub_endpoint_, true);
  if (!client) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::PUB_REGISTER, info_)) {
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

  Log::debug << "PublisherImpl created on topic \"" << info_.topic_info.name << "\"." << std::endl;

  if (MULTITHREADED) {
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

PublisherImpl::~PublisherImpl() {
  // Deregister publisher with rixhub
  if (registered_flag_) {
    auto client = get_transport_factory(Protocol::TCP).create_stream(rixhub_endpoint_, true);
    if (!client) {
      return;
    }
    client->send_message(OPCODE::PUB_DEREGISTER, info_);
  }
  Log::debug << "PublisherImpl on topic \"" << info_.topic_info.name << "\" destroyed." << std::endl;

  if (MULTITHREADED) {
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

void PublisherImpl::publish(const Message& msg) {
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

  std::vector<std::shared_ptr<Stream>> writable;
  std::vector<std::shared_ptr<Stream>> exceptional;
  if (Pollable::get_poller()) {
    std::vector<std::shared_ptr<Stream>> connections_vector(connections_.begin(), connections_.end());
    Pollable::poll(connections_vector, Duration(0.0), PollFlag::WRITE, writable, exceptional);
  } else {
    // Fallback if poller is not available
    for (const auto& conn : connections_) {
      if (conn->is_writable()) {
        writable.push_back(conn);
      } else {
        exceptional.push_back(conn);
      }
    }
  }
  // Remove any clients that have exceptions
  for (const auto& conn : exceptional) {
    connections_.erase(conn);
    Log::debug << "Removed exceptional subscriber from topic \"" << info_.topic_info.name << "\"." << std::endl;
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

size_t PublisherImpl::get_subscriber_count() const {
  std::lock_guard<std::mutex> guard(connections_mutex_);
  return connections_.size();
}

void PublisherImpl::on_spin() {
  Duration timeout(MULTITHREADED ? 1.0 : 0.0);

  // Check to see if a subscriber has made a connection
  if (!server_->wait_readable(timeout)) {
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

} // namespace detail
} // namespace rix