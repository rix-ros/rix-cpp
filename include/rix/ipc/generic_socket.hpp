#pragma once

#include "rix/ipc/endpoint.hpp"
#include "rix/util/time.hpp"

#include <memory>

namespace rix::ipc {

class GenericSocket {
public:
  // Constructor and Destructor
  GenericSocket() = default;
  virtual ~GenericSocket() = default;

  // Disable copy and move semantics (force use of shared/unique pointers)
  GenericSocket(const GenericSocket &) = delete;
  GenericSocket &operator=(const GenericSocket &) = delete;
  GenericSocket(GenericSocket &&) = delete;
  GenericSocket &operator=(GenericSocket &&) = delete;

  // Socket state operations
  virtual bool bind(const Endpoint &endpoint) const = 0;
  virtual bool listen(int backlog) const = 0;
  virtual std::unique_ptr<GenericSocket>
  accept(Endpoint &remote_endpoint) const = 0;
  std::unique_ptr<GenericSocket> accept() const {
    Endpoint ep;
    return accept(ep);
  }
  virtual bool connect(const Endpoint &endpoint) const = 0;
  virtual void close() const = 0;

  // I/O operations
  virtual ssize_t send(const void *buf, size_t len, int flags) const = 0;
  virtual ssize_t recv(void *buf, size_t len, int flags) const = 0;
  virtual ssize_t send_to(const void *buf, size_t len, const Endpoint &endpoint,
                          int flags) const = 0;
  virtual ssize_t recv_from(void *buf, size_t len, Endpoint &endpoint,
                            int flags) const = 0;

  // I/O multiplexing operations
  virtual bool wait_readable(const rix::util::Duration &timeout) const = 0;
  virtual bool wait_writable(const rix::util::Duration &timeout) const = 0;
  virtual bool wait_exception(const rix::util::Duration &timeout) const = 0;

  // Socket control operations
  virtual bool set_blocking(bool blocking) const = 0;
  virtual bool get_blocking() const = 0;
  virtual bool set_reuse_address(bool reuse) const = 0;
  virtual bool get_reuse_address() const = 0;

  // Optional implementations (return false / -1 if not supported)
  virtual bool set_reuse_port(bool reuse) const { return false; }
  virtual bool get_reuse_port() const { return false; }
  virtual bool set_recv_buffer_size(int size) const { return false; }
  virtual int get_recv_buffer_size() const { return -1; }
  virtual bool set_send_buffer_size(int size) const { return false; }
  virtual int get_send_buffer_size() const { return -1; }
  virtual bool
  join_multicast_group(const std::string &multicast_address) const {
    return false;
  }
  virtual bool
  leave_multicast_group(const std::string &multicast_address) const {
    return false;
  }

  // Endpoint retrieval
  virtual Endpoint local_endpoint() const = 0;
  virtual Endpoint remote_endpoint() const = 0;
};

} // namespace rix::ipc