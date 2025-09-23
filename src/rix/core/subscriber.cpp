#include "rix/core/subscriber.hpp"

namespace rix::core {

Subscriber::Subscriber(const rix::msg::mediator::SubInfo &info, SocketFactory socket_factory,
                       const rix::ipc::Endpoint &rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), callback_(nullptr), rixhub_endpoint_(rixhub_endpoint),
      shutdown_flag_(true), registered_flag_(false) {

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

  // Register subscriber with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_))
    return;
  if (!client->send_message(OPCODE::SUB_REGISTER, info_))
    return;

  rix::msg::mediator::Operation op;
  rix::msg::mediator::Status status;
  if (!client->recv_message(op, status))
    return;
  if (status.error)
    return;

  shutdown_flag_ = false;
  registered_flag_ = true;

  rix::util::Log::debug << "Subscriber created on topic \"" << info_.topic_info.name << "\"." << std::endl;
}

Subscriber::~Subscriber() {
  if (registered_flag_) {
    auto client = socket_factory_();
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::SUB_DEREGISTER, info_);
    }
  }
}

bool Subscriber::ok() const { return !shutdown_flag_; }

void Subscriber::shutdown() { shutdown_flag_ = true; }

size_t Subscriber::get_publisher_count() const {
  std::lock_guard<std::mutex> guard(callback_mutex_);
  return clients_.size();
}

/**< TODO: Implement the spin_once method */
void Subscriber::spin_once() {
  std::lock_guard<std::mutex> guard(callback_mutex_);

  // Check to see if rixhub has made a connection
  if (server_->wait_readable(rix::util::Duration(0.0))) {
    // Accept a connection from rixhub
    auto conn = server_->accept();
    if (!conn) {
      return;
    }

    rix::msg::mediator::SubNotify sub_notify;
    rix::msg::mediator::Operation op;
    if (!conn->recv_message(op, sub_notify)) {
      return;
    }
    if (op.opcode != OPCODE::SUB_NOTIFY) {
      rix::util::Log::warn << "Received invalid opcode from rixhub." << std::endl;
      return;
    }

    // Connect to the specified publishers (non-blocking)
    for (const auto &pub : sub_notify.publishers) {
      auto client = socket_factory_();
      client->set_blocking(false);
      client->connect(rix::ipc::Endpoint(pub.endpoint.address, pub.endpoint.port));
      clients_.insert(client);
      rix::util::Log::debug << "Connected to publisher at \"" << pub.endpoint.address << ":" << pub.endpoint.port
                            << "\" on topic \"" << pub.topic_info.name << "\"." << std::endl;
    }
  }

  std::vector<std::shared_ptr<rix::ipc::GenericSocket>> readable_clients;
  std::vector<std::shared_ptr<rix::ipc::GenericSocket>> exception_clients;
  rix::ipc::poll(readable_clients, exception_clients, clients_.begin(), clients_.end(), rix::util::Duration(0.0),
                 rix::ipc::SelectFlag::READ);

  auto it = readable_clients.begin();
  while (it != readable_clients.end()) {
    auto client = *it;

    // Read a message from the publisher
    rix::msg::mediator::Operation op;
    if (!client->recv_message(op, *msg_instance_)) {
      clients_.erase(*it);
      it++;
      continue;
    }

    if (op.opcode != OPCODE::PUB_MESSAGE) {
      clients_.erase(*it);
      it++;
      continue;
    }

    // Invoke the callback
    callback_(*msg_instance_);
    it++;
  }
  readable_clients.clear();
}

} // namespace rix::core