#pragma once

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/poll.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/message.hpp"
#include "rix/util/time.hpp"

#include <memory>

namespace rix {

const int MAX_CONN = 128;

class GenericSocket {
public:
  // Constructor and Destructor
  GenericSocket() = default;
  virtual ~GenericSocket() = default;

  // Disable copy and move semantics (force use of shared/unique pointers)
  GenericSocket(const GenericSocket&) = delete;
  GenericSocket& operator=(const GenericSocket&) = delete;
  GenericSocket(GenericSocket&&) = delete;
  GenericSocket& operator=(GenericSocket&&) = delete;

  // Socket state operations
  virtual bool bind(const Endpoint& endpoint) const = 0;
  virtual bool listen(int backlog) const = 0;
  virtual std::shared_ptr<GenericSocket> accept(Endpoint& remote_endpoint) const = 0;
  std::shared_ptr<GenericSocket> accept() const {
    Endpoint ep;
    return accept(ep);
  }
  virtual bool connect(const Endpoint& endpoint) const = 0;
  virtual void close() const = 0;

  // I/O multiplexing operations
  virtual bool wait_readable(const Duration& timeout) const = 0;
  virtual bool wait_writable(const Duration& timeout) const = 0;
  virtual bool wait_exception(const Duration& timeout) const = 0;

  // Socket control operations
  virtual bool set_blocking(bool blocking) const = 0;
  virtual bool get_blocking() const = 0;
  virtual bool set_reuse_address(bool reuse) const = 0;
  virtual bool get_reuse_address() const = 0;

  // Endpoint retrieval
  virtual Endpoint local_endpoint() const = 0;
  virtual Endpoint remote_endpoint() const = 0;

  virtual int get_fd() const { return -1; }

  bool is_writable() const { return wait_writable(Duration(0.0)); }
  bool is_readable() const { return wait_readable(Duration(0.0)); }
  bool is_exception() const { return wait_exception(Duration(0.0)); }

  // Write operation and message
  virtual bool send_message(uint8_t opcode, const msg::Message& msg) const {
    // Serialize the message
    msg::mediator::Operation op;
    op.len = msg.size();
    op.opcode = opcode;
    std::vector<uint8_t> buffer(op.size() + msg.size());
    size_t offset = 0;
    op.serialize(buffer.data(), offset);
    msg.serialize(buffer.data(), offset);

    size_t bytes = 0;
    while (bytes < buffer.size()) {
      ssize_t result = send(buffer.data() + bytes, buffer.size() - bytes, 0);
      if (result <= 0) {
        return false;
      }
      bytes += result;
    }
    return bytes == buffer.size();
  }

  // Read message only
  virtual bool recv_message(msg::Message& msg, size_t len) const {
    // Read the message body only
    std::vector<uint8_t> buffer(len);
    size_t bytes = 0;
    while (bytes < buffer.size()) {
      ssize_t result = recv(buffer.data() + bytes, buffer.size() - bytes, 0);
      if (result <= 0) {
        return false;
      }
      bytes += result;
    }
    size_t offset = 0;
    if (!msg.deserialize(buffer.data(), buffer.size(), offset)) {
      return false;
    }
    return true;
  }

  // Read both operation and message (useful if message type is known)
  bool recv_message(msg::mediator::Operation& op, msg::Message& msg) const {
    // Read the operation header first
    if (!recv_message(op, op.size())) {
      return false;
    }
    // Then read the message body
    if (!recv_message(msg, op.len)) {
      return false;
    }
    return true;
  }

  static std::shared_ptr<GenericPoller> get_poller() { return poller_; }
  static void set_poller(std::shared_ptr<GenericPoller> poller) { poller_ = poller; }
  static bool poll(const std::vector<std::shared_ptr<GenericSocket>>& all_sockets,
                   const Duration& duration,
                   PollFlag flag,
                   std::vector<std::shared_ptr<GenericSocket>>& sockets,
                   std::vector<std::shared_ptr<GenericSocket>>& exception_sockets) {
    if (!poller_) {
      return false;
    }
    return poller_->poll(all_sockets, duration, flag, sockets, exception_sockets);
  }

private:
  // Low-level I/O operations to be implemented by derived classes
  virtual ssize_t send(const void* buf, size_t len, int flags) const = 0;
  virtual ssize_t recv(void* buf, size_t len, int flags) const = 0;
  static inline std::shared_ptr<GenericPoller> poller_{std::make_shared<Poller>()};
};

} // namespace rix