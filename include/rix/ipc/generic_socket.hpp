#pragma once

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/poll.hpp"
#include "rix/msg/message.hpp"
#include "rix/sys_msgs/Operation.hpp"
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
  virtual bool send_message(uint8_t opcode, const Message& msg) const {
    // Get the message prefix
    const size_t prefix_len = msg.get_prefix_len();
    uint8_t* prefix_buffer = new uint8_t[prefix_len];
    size_t offset = 0;
    msg.get_prefix(prefix_buffer, offset);

    // Serialize the message
    sys_msgs::Operation operation;
    operation.len = msg.get_prefix_len();
    operation.opcode = opcode;
    const int segment_count = operation.get_segment_count() + msg.get_segment_count() + 1;
    std::vector<ConstMessageSegment> segments(segment_count);
    offset = 0;
    operation.get_segments(segments.data(), segments.size(), offset);
    segments[offset++] = ConstMessageSegment(prefix_buffer, prefix_len);
    msg.get_segments(segments.data(), segments.size(), offset);

    // Send the serialized message
    ssize_t bytes_sent = writev(segments.data(), static_cast<int>(segments.size()));
    delete[] prefix_buffer;
    return bytes_sent > 0;
  }

  // Read message only
  virtual bool recv_message(Message& msg, size_t prefix_len) const {
    ssize_t bytes = 0;
    if (prefix_len > 0) {
      // Read the prefix first
      uint8_t* prefix_buffer = new uint8_t[prefix_len];
      bytes = recv(prefix_buffer, prefix_len, 0);

      // Resize the message
      size_t offset = 0;
      if (!msg.resize(prefix_buffer, bytes, offset)) {
        delete[] prefix_buffer;
        return false;
      }
      delete[] prefix_buffer;
    }

    // Read the segments
    std::vector<MessageSegment> segments(msg.get_segments());
    bytes = readv(segments.data(), segments.size());
    return bytes > 0;
  }

  // Read both operation and message (useful if message type is known)
  bool recv_message(sys_msgs::Operation& operation, Message& msg) const {
    // Read the operation header first
    if (!recv_message(operation, operation.get_prefix_len())) {
      return false;
    }
    // Then read the message body
    if (!recv_message(msg, operation.len)) {
      return false;
    }
    return true;
  }
  
  void ignore_message(size_t len) const {
    // Read and discard 'len' bytes
    std::vector<uint8_t> buffer(len);
    size_t bytes = 0;
    while (bytes < buffer.size()) {
      ssize_t result = recv(buffer.data() + bytes, buffer.size() - bytes, 0);
      if (result <= 0) {
        return;
      }
      bytes += result;
    }
  }

  static std::shared_ptr<GenericPoller> get_poller() { return poller_; }
  static void set_poller(const std::shared_ptr<GenericPoller>& poller) { poller_ = poller; }
  static bool poll(const std::vector<std::shared_ptr<GenericSocket>>& all_sockets,
                   const Duration& duration,
                   const PollFlag flag,
                   std::vector<std::shared_ptr<GenericSocket>>& sockets,
                   std::vector<std::shared_ptr<GenericSocket>>& exception_sockets) {
    if (!poller_) {
      return false;
    }
    return poller_->poll(all_sockets, duration, flag, sockets, exception_sockets);
  }

private:
  // Low-level I/O operations to be implemented by derived classes
  virtual ssize_t writev(const ConstMessageSegment* segments, size_t segment_count) const = 0;
  virtual ssize_t readv(MessageSegment* segments, size_t segment_count) const = 0;
  virtual ssize_t send(const void* buf, size_t len, int flags) const = 0;
  virtual ssize_t recv(void* buf, size_t len, int flags) const = 0;
  static inline std::shared_ptr<GenericPoller> poller_{std::make_shared<Poller>()};
};

} // namespace rix