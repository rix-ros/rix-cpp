#pragma once

#include "rix/core/common.hpp"
#include "rix/ipc/mock_socket.hpp"
#include <memory>
#include <queue>

namespace rix {

// Automatically manages socket creation and lifecycle
class SocketManager {
public:
  SocketManager()
      : sockets_(std::make_shared<std::queue<std::shared_ptr<MockSocket>>>()) {
    socket_factory_ = [sockets = sockets_]() -> std::shared_ptr<GenericSocket> {
      if (sockets->empty()) {
        throw std::runtime_error(
            "No more sockets available. Did you forget to configure a socket?");
      }
      auto socket = sockets->front();
      sockets->pop();
      return socket;
    };
  }

  SocketManager(const SocketManager& other)
      : sockets_(other.sockets_), socket_factory_(other.socket_factory_) {}
  SocketManager& operator=(const SocketManager& other) {
    if (this != &other) {
      sockets_ = other.sockets_;
      socket_factory_ = other.socket_factory_;
    }
    return *this;
  }
  SocketManager(SocketManager&& other)
      : sockets_(std::move(other.sockets_)),
        socket_factory_(std::move(other.socket_factory_)) {}
  SocketManager& operator=(SocketManager&& other) {
    if (this != &other) {
      sockets_ = std::move(other.sockets_);
      socket_factory_ = std::move(other.socket_factory_);
    }
    return *this;
  }

  // Get the socket factory for Node/Mediator construction
  SocketFactory get_factory() { return socket_factory_; }

  // Add a preconfigured socket to the queue
  void add_socket(std::shared_ptr<MockSocket> socket) { sockets_->push(socket); }

  // Create a new mock socket and add it to the queue
  std::shared_ptr<MockSocket> create_socket() {
    auto socket = std::make_shared<MockSocket>();
    add_socket(socket);
    return socket;
  }

  void reset() {
    while (!sockets_->empty()) {
      sockets_->pop();
    }
  }

private:
  std::shared_ptr<std::queue<std::shared_ptr<MockSocket>>> sockets_;
  SocketFactory socket_factory_;
};

} // namespace rix