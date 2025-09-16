#pragma once

#include "rix/ipc/connection.hpp"
#include "rix/ipc/generic_socket.hpp"
#include <memory>

namespace rix::ipc {
class Server {
public:
  explicit Server(std::unique_ptr<GenericSocket> socket,
                  const Endpoint &endpoint)
      : socket_(std::move(socket)) {
    if (!socket_->set_reuse_address(true)) {
      socket_->close();
      return;
    }
    if (!socket_->bind(endpoint)) {
      socket_->close();
      return;
    }
    if (!socket_->listen(16)) {
      socket_->close();
      return;
    }
  }
  ~Server() { socket_->close(); }

  bool is_exception() const {
    return socket_->wait_exception(rix::util::Duration(0.0));
  }

  bool wait_exception(const rix::util::Duration &timeout) const {
    return socket_->wait_exception(timeout);
  }

  bool is_acceptable() const {
    return socket_->wait_readable(rix::util::Duration(0.0));
  }

  bool wait_acceptable(const rix::util::Duration &timeout) const {
    return socket_->wait_readable(timeout);
  }

  std::shared_ptr<Connection> accept() const {
    auto client_socket = socket_->accept();
    if (client_socket) {
      return std::make_shared<Connection>(std::move(client_socket));
    }
    return nullptr;
  }

  Endpoint local_endpoint() const { return socket_->local_endpoint(); }

  bool set_blocking(bool blocking) const {
    return socket_->set_blocking(blocking);
  }

  bool get_blocking() const { return socket_->get_blocking(); }

private:
  std::unique_ptr<GenericSocket> socket_;
};

} // namespace rix::ipc