#include "rix/core/subscriber.hpp"

namespace rix::core {

Subscriber::Subscriber(const rix::msg::mediator::SubInfo &info,
                       std::shared_ptr<rix::ipc::Server> server,
                       ClientFactory factory,
                       const rix::ipc::Endpoint &rixhub_endpoint)
    : info_(info), server_(server), factory_(factory), callback_(nullptr),
      rixhub_endpoint_(rixhub_endpoint), shutdown_flag_(false) {
  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    shutdown();
    return;
  }

  if (!send_message_with_opcode(factory_(), info_, OPCODE::SUB_REGISTER,
                                rixhub_endpoint_)) {
    shutdown();
  }
}

Subscriber::~Subscriber() {
  shutdown();
  send_message_with_opcode_no_response(
      factory_(), info_, OPCODE::SUB_DEREGISTER, rixhub_endpoint_);
}

bool Subscriber::ok() const { return !shutdown_flag_; }

void Subscriber::shutdown() { shutdown_flag_ = true; }

Subscriber::SerializedCallback Subscriber::get_callback() const {
  return callback_;
}

size_t Subscriber::get_publisher_count() const {
  std::lock_guard<std::mutex> guard(callback_mutex_);
  return clients_.size();
}

/**< TODO: Implement the spin_once method */
void Subscriber::spin_once() {
  std::lock_guard<std::mutex> guard(callback_mutex_);

  // Check to see if rixhub has made a connection
  if (server_->wait_acceptable(rix::util::Duration(0.0))) {
    // Accept a connection from rixhub
    std::shared_ptr<rix::ipc::Connection> conn = server_->accept();
    if (!conn) {
      return;
    }

    rix::msg::mediator::Operation op;
    std::vector<uint8_t> buffer(op.size());
    ssize_t bytes_read = conn->read(buffer.data(), buffer.size());
    size_t offset = 0;

    // If the deserialize operation fails, return
    if (!op.deserialize(buffer.data(), bytes_read, offset)) {
      return;
    }

    if (op.opcode != OPCODE::SUB_NOTIFY) {
      rix::util::Log::warn << "Received invalid opcode from rixhub."
                           << std::endl;
      return;
    }

    // Resize the buffer for the SubNotify message
    rix::msg::mediator::SubNotify sub_notify;
    buffer.resize(op.len);
    bytes_read = conn->read(buffer.data(), buffer.size());
    offset = 0;

    // If the deserialize operation fails, return
    if (!sub_notify.deserialize(buffer.data(), bytes_read, offset)) {
      return;
    }

    // Connect to the specified publishers (non-blocking)
    for (const auto &pub : sub_notify.publishers) {
      auto client = factory_();
      client->set_blocking(false);
      client->connect(
          rix::ipc::Endpoint(pub.endpoint.address, pub.endpoint.port));
      clients_.insert({pub.id, client});
    }
  }

  auto it = clients_.begin();
  while (it != clients_.end()) {
    auto client = it->second;
    // If client is not ok (hang up), erase it
    if (client->is_exception()) {
      it = clients_.erase(it);
      continue;
    }

    // If the client is not connected or not readable, go to next
    if (!client->is_connected() || !client->is_readable()) {
      it++;
      continue;
    }

    rix::msg::standard::UInt32 size;
    std::vector<uint8_t> buffer(size.size());
    ssize_t bytes = client->read(buffer.data(), buffer.size());
    size_t offset = 0;

    // If the deserialize operation fails, erase the client
    if (!size.deserialize(buffer.data(), bytes, offset)) {
      rix::util::Log::warn << "Failed to read from publisher." << std::endl;
      it = clients_.erase(it);
      continue;
    }

    // Read the message
    buffer.resize(size.data);

    size_t bytes_read = 0;
    while (bytes_read < size.data) {
      bytes =
          client->read(buffer.data() + bytes_read, buffer.size() - bytes_read);
      if (bytes <= 0) {
        break;
      }
      bytes_read += bytes;
    }

    // Invoke the callback
    callback_(buffer.data(), bytes_read);
    it++;
  }
}

} // namespace rix::core