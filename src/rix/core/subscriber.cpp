#include "rix/core/subscriber.hpp"
#include "rix/sys_msgs/Status.hpp"
#include "rix/sys_msgs/SubNotify.hpp"

namespace rix {

Subscriber::Subscriber(const sys_msgs::SubInfo& info, TransportFactory socket_factory, const Endpoint& rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), callback_(nullptr), rixhub_endpoint_(rixhub_endpoint),
      registered_flag_(false) {

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

  // Register subscriber with rixhub
  auto client = socket_factory_.create_stream(rixhub_endpoint_, true);
  if (!client) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::SUB_REGISTER, info_)) {
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

  Log::debug << "Subscriber created on topic \"" << info_.topic_info.name << "\"." << std::endl;

  if (MULTITHREADED) {
    sub_notify_acceptor_.spin_thread = std::thread([this]() { this->sub_notify_acceptor_.spin(); });
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

Subscriber::~Subscriber() {
  if (registered_flag_) {
    auto client = socket_factory_.create_stream(rixhub_endpoint_, true);
    if (!client) {
      return;
    }
    client->send_message(OPCODE::SUB_DEREGISTER, info_);
  }
  Log::debug << "Subscriber on topic \"" << info_.topic_info.name << "\" destroyed." << std::endl;

  if (MULTITHREADED) {
    sub_notify_acceptor_.shutdown();
    if (sub_notify_acceptor_.spin_thread.joinable()) {
      sub_notify_acceptor_.spin_thread.join();
    }
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

size_t Subscriber::get_publisher_count() const {
  std::lock_guard<std::mutex> guard(callback_mutex_);
  return clients_.size();
}

/**< TODO: Implement the spin_once method */
void Subscriber::on_spin() {

  if (!MULTITHREADED) {
    // In single-threaded mode, we need to also spin the acceptor
    sub_notify_acceptor_.spin_once();
  }

  std::lock_guard<std::mutex> guard(callback_mutex_);
  if (clients_.empty() || !callback_) {
    return;
  }
  std::vector<std::shared_ptr<Stream>> readable;
  std::vector<std::shared_ptr<Stream>> exceptional;
  if (Pollable::get_poller()) {
    Duration timeout(MULTITHREADED ? 1.0 : 0.0);
    std::vector<std::shared_ptr<Stream>> clients_vector(clients_.begin(), clients_.end());
    Pollable::poll(clients_vector, timeout, PollFlag::READ, readable, exceptional);
  } else {
    // Fallback if poller is not available
    for (const auto& client : clients_) {
      if (client->is_readable()) {
        readable.push_back(client);
      } else if (client->is_exception()) {
        exceptional.push_back(client);
      }
    }
  }

  // Remove any clients that have exceptions
  for (const auto& client : exceptional) {
    clients_.erase(client);
    Log::debug << "Removed exceptional publisher from topic \"" << info_.topic_info.name << "\"." << std::endl;
  }

  auto it = readable.begin();
  while (it != readable.end()) {
    auto client = *it;

    // Read a message from the publisher
    sys_msgs::Operation operation;
    if (!client->recv_message(operation, *msg_instance_)) {
      clients_.erase(client);
      it++;
      Log::debug << "Removed exceptional publisher from topic \"" << info_.topic_info.name << "\"." << std::endl;
      continue;
    }

    if (operation.opcode != OPCODE::PUB_MESSAGE) {
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
  Duration timeout(MULTITHREADED ? 1.0 : 0.0);

  // Check to see if rixhub has made a connection
  if (!parent.server_->wait_readable(timeout)) {
    return;
  }

  // Accept a connection from rixhub
  auto conn = parent.server_->accept();
  if (!conn) {
    return;
  }

  sys_msgs::Operation operation;
  sys_msgs::SubNotify sub_notify;
  if (!conn->recv_message(operation, sub_notify)) {
    return;
  }
  if (operation.opcode != OPCODE::SUB_NOTIFY) {
    Log::warn << "Received invalid opcode from rixhub." << std::endl;
    return;
  }

  std::lock_guard<std::mutex> guard(parent.callback_mutex_);
  // Connect to the specified publishers (non-blocking)
  for (const auto& pub : sub_notify.publishers) {
    auto client = parent.socket_factory_.create_stream(Endpoint(pub.endpoint.address, pub.endpoint.port), false);
    if (!client) {
      continue;
    }
    client->set_blocking(true);
    parent.clients_.insert(client);
    Log::debug << "Connected to publisher at \"" << pub.endpoint.address << ":" << pub.endpoint.port << "\" on topic \""
               << pub.topic_info.name << "\"." << std::endl;
  }
}

} // namespace rix