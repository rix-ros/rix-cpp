#include "rix/core/subscriber.hpp"

namespace rix {

Subscriber::Subscriber(const msg::mediator::SubInfo& info,
                       SocketFactory socket_factory,
                       const Endpoint& rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), callback_(nullptr), rixhub_endpoint_(rixhub_endpoint),
      registered_flag_(false) {

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

  // Register subscriber with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::SUB_REGISTER, info_)) {
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

  Log::debug << "Subscriber created on topic \"" << info_.topic_info.name << "\"." << std::endl;

#ifdef RIX_MULTITHREADED
  sub_notify_acceptor_.spin_thread = std::thread([this]() { this->sub_notify_acceptor_.spin(); });
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

Subscriber::~Subscriber() {
  if (registered_flag_) {
    auto client = socket_factory_();
    if (!client) {
      return;
    }
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::SUB_DEREGISTER, info_);
    }
  }
  Log::debug << "Subscriber on topic \"" << info_.topic_info.name << "\" destroyed." << std::endl;

#ifdef RIX_MULTITHREADED
  sub_notify_acceptor_.shutdown();
  if (sub_notify_acceptor_.spin_thread.joinable()) {
    sub_notify_acceptor_.spin_thread.join();
  }
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif
}

size_t Subscriber::get_publisher_count() const {
  std::lock_guard<std::mutex> guard(callback_mutex_);
  return clients_.size();
}

/**< TODO: Implement the spin_once method */
void Subscriber::on_spin() {

#ifndef RIX_MULTITHREADED
  // In single-threaded mode, we need to also spin the acceptor
  sub_notify_acceptor_.spin_once();
#endif

  std::lock_guard<std::mutex> guard(callback_mutex_);
  if (clients_.empty() || !callback_) {
    return;
  }
  std::vector<std::shared_ptr<GenericSocket>> sockets(clients_.begin(), clients_.end());
  std::vector<std::shared_ptr<GenericSocket>> readable;

  if (GenericSocket::get_poller()) {

    std::vector<std::shared_ptr<GenericSocket>> exceptional;

#ifdef RIX_MULTITHREADED
    Duration timeout(1.0);
#else
    Duration timeout(0.0);
#endif

    GenericSocket::poll(sockets, timeout, PollFlag::READ, readable, exceptional);

    // Remove any clients that have exceptions
    for (const auto& conn : exceptional) {
      clients_.erase(conn);
      Log::debug << "Removed exceptional publisher from topic \"" << info_.topic_info.name << "\"." << std::endl;
    }
    exceptional.clear();
  } else {
    // Fallback if poller is not available
    for (const auto& sock : sockets) {
      if (sock->is_readable()) {
        readable.push_back(sock);
      }
    }
  }

  auto it = readable.begin();
  while (it != readable.end()) {
    auto client = *it;

    // Read a message from the publisher
    msg::mediator::Operation op;
    if (!client->recv_message(op, *msg_instance_)) {
      clients_.erase(client);
      it++;
      Log::debug << "Removed exceptional publisher from topic \"" << info_.topic_info.name << "\"." << std::endl;
      continue;
    }

    if (op.opcode != OPCODE::PUB_MESSAGE) {
      clients_.erase(client);
      it++;
      Log::debug << "Removed exceptional publisher from topic \"" << info_.topic_info.name << "\"." << std::endl;
      continue;
    }

    Log::debugv << "Received message on topic \"" << info_.topic_info.name << "\"." << std::endl;
    // Invoke the callback
    callback_(*msg_instance_);
    it++;
  }
  readable.clear();
}

Subscriber::SubNotifyAcceptor::SubNotifyAcceptor(Subscriber& parent) : parent(parent) {}

void Subscriber::SubNotifyAcceptor::on_spin() {
  // Check to see if rixhub has made a connection
  if (!parent.server_->wait_readable(Duration(1.0))) {
    return;
  }

  // Accept a connection from rixhub
  auto conn = parent.server_->accept();
  if (!conn) {
    return;
  }

  msg::mediator::Operation op;
  msg::mediator::SubNotify sub_notify;
  if (!conn->recv_message(op, sub_notify)) {
    return;
  }
  if (op.opcode != OPCODE::SUB_NOTIFY) {
    Log::warn << "Received invalid opcode from rixhub." << std::endl;
    return;
  }

  std::lock_guard<std::mutex> guard(parent.callback_mutex_);
  // Connect to the specified publishers (non-blocking)
  for (const auto& pub : sub_notify.publishers) {
    auto client = parent.socket_factory_();
    if (!client) {
      continue;
    }
    client->set_blocking(false);
    client->connect(Endpoint(pub.endpoint.address, pub.endpoint.port));
    parent.clients_.insert(client);
    Log::debug << "Connected to publisher at \"" << pub.endpoint.address << ":" << pub.endpoint.port << "\" on topic \""
               << pub.topic_info.name << "\"." << std::endl;
  }
}

} // namespace rix