#pragma once

#include <cstddef>
#include <string>

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/shm_acceptor.hpp"
#include "rix/ipc/stream.hpp"

namespace rix {

class ShmStream final : public Stream {
  friend class ShmAcceptor;

public:
  // Client-side constructor: connects to a ShmAcceptor at the given endpoint.
  ShmStream(const Endpoint& endpoint, bool blocking);
  ~ShmStream();

  bool set_blocking(bool blocking) const override;
  bool get_blocking() const override;

  Endpoint local_endpoint() const override;
  Endpoint remote_endpoint() const override;

  bool wait_readable(const Duration& timeout) const override;
  bool wait_writable(const Duration& timeout) const override;
  bool wait_exception(const Duration& timeout) const override;

private:
  int fd_;         // Unix domain socket - used for handshake and exception detection
  void* shm_addr_; // mapped shared memory region
  size_t shm_size_;
  std::string shm_name_;
  bool is_server_;
  Endpoint local_ep_;
  Endpoint remote_ep_;
  int data_efd_;   // eventfd: peer signals this when it has written data for us to read
  int space_efd_;  // eventfd: peer signals this when it has consumed data, freeing space

  // Server-side constructor: called by ShmAcceptor::accept().
  ShmStream(int fd, void* shm_addr, size_t shm_size, const std::string& shm_name, bool is_server,
            const Endpoint& local_ep, const Endpoint& remote_ep);

  // writev/readv not implemented - fall back to send/recv in Stream::send_all / recv_all.
  bool writev(const ConstMessageSegment* segments, size_t segment_count, ssize_t& ret) const override {
    return false;
  }
  bool readv(MessageSegment* segments, size_t segment_count, ssize_t& ret) const override {
    return false;
  }

  ssize_t send(const uint8_t* buf, size_t len, int flags) const override;
  ssize_t recv(uint8_t* buf, size_t len, int flags) const override;

  // Return false so Pollable::poll falls through to wait_readable/wait_writable,
  // which check the SHM ring buffer. The Unix socket fd is only used internally
  // for the handshake and exception detection via wait_exception.
  inline bool get_fd(int& fd) const override {
    fd = -1;
    return false;
  }

  // Ring buffer helpers - operate on the correct half of shm based on is_server_.
  size_t get_readable_bytes() const;
  size_t get_writable_bytes() const;
  void increment_read_ptr(size_t bytes) const;
  void increment_write_ptr(size_t bytes) const;
};

} // namespace rix
