#pragma once

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/stream.hpp"
#include "rix/ipc/shm_acceptor.hpp"

namespace rix {

class ShmStream final : public Stream {
  friend class ShmAcceptor;

public:
  // Constructor and Destructor
  ShmStream(const Endpoint& endpoint, bool blocking);
  ~ShmStream();

  // Socket control operations
  bool set_blocking(bool blocking) const override;
  bool get_blocking() const override;

  // Endpoint retrieval
  Endpoint local_endpoint() const override;
  Endpoint remote_endpoint() const override;

  bool wait_readable(const Duration& timeout) const override;
  bool wait_writable(const Duration& timeout) const override;
  bool wait_exception(const Duration& timeout) const override;

private:
  int fd_;
  ShmStream(int fd);

  // Low-level I/O operations to be implemented by derived classes
  bool writev(const ConstMessageSegment* segments, size_t segment_count, ssize_t& ret) const override;
  bool readv(MessageSegment* segments, size_t segment_count, ssize_t& ret) const override;
  ssize_t send(const void* buf, size_t len, int flags) const override;
  ssize_t recv(void* buf, size_t len, int flags) const override;

  inline bool get_fd(int& fd) const override { return fd_; }

  // Ring buffer management for shared memory
  size_t get_readable_bytes() const;
  size_t get_writable_bytes() const;
  void increment_read_ptr(size_t bytes) const;
  void increment_write_ptr(size_t bytes) const;
};

} // namespace rix