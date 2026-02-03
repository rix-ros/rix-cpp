#pragma once

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/poll.hpp"
#include "rix/msg/message.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include "rix/util/time.hpp"

#include <memory>

namespace rix {

const int MAX_CONN = 128;

class Stream : public Pollable {
public:
  // Constructor and Destructor
  Stream() = default;
  virtual ~Stream() = default;

  // Disable copy and move semantics (force use of shared/unique pointers)
  Stream(const Stream&) = delete;
  Stream& operator=(const Stream&) = delete;
  Stream(Stream&&) = delete;
  Stream& operator=(Stream&&) = delete;

  // Socket control operations
  virtual bool set_blocking(bool blocking) const = 0;
  virtual bool get_blocking() const = 0;

  // Endpoint retrieval
  virtual Endpoint local_endpoint() const = 0;
  virtual Endpoint remote_endpoint() const = 0;

  virtual bool send_message(uint8_t opcode, const Message& msg) const;
  virtual bool recv_message(Message& msg, size_t prefix_len) const;
  bool recv_message(sys_msgs::Operation& operation, Message& msg) const;
  void ignore_message(size_t len) const;

private:
  // Low-level I/O operations to be implemented by derived classes
  virtual bool writev(const ConstMessageSegment* segments, size_t segment_count, ssize_t& ret) const { return false; }
  virtual bool readv(MessageSegment* segments, size_t segment_count, ssize_t& ret) const { return false; }
  virtual ssize_t send(const void* buf, size_t len, int flags) const = 0;
  virtual ssize_t recv(void* buf, size_t len, int flags) const = 0;

  bool send_all(const ConstMessageSegment* segments, size_t segment_count) const;
  bool recv_all(MessageSegment* segments, size_t segment_count) const;
};

} // namespace rix