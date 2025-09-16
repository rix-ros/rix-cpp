#pragma once

#include "rix/ipc/generic_socket.hpp"
#include <memory>

namespace rix::ipc {

class Connection {
public:
  explicit Connection(std::unique_ptr<GenericSocket> socket)
      : socket_(std::move(socket)) {}
      
  ~Connection() { socket_->close(); }

  bool is_writable() const { return wait_writable(rix::util::Duration(0.0)); }

  bool is_readable() const { return wait_readable(rix::util::Duration(0.0)); }

  bool is_exception() const { return wait_exception(rix::util::Duration(0.0)); }

  bool wait_readable(const rix::util::Duration &timeout) const {
    return socket_->wait_readable(timeout);
  }

  bool wait_writable(const rix::util::Duration &timeout) const {
    return socket_->wait_writable(timeout);
  }

  bool wait_exception(const rix::util::Duration &timeout) const {
    return socket_->wait_exception(timeout);
  }

  ssize_t write(const void *buf, size_t len) const {
    return socket_->send(buf, len, 0);
  }

  ssize_t read(void *buf, size_t len) const {
    return socket_->recv(buf, len, 0);
  }

  Endpoint local_endpoint() const { return socket_->local_endpoint(); }

  Endpoint remote_endpoint() const { return socket_->remote_endpoint(); }

  bool set_blocking(bool blocking) const {
    return socket_->set_blocking(blocking);
  }

  bool get_blocking() const { return socket_->get_blocking(); }

protected:
  std::unique_ptr<GenericSocket> socket_;
};

} // namespace rix::ipc