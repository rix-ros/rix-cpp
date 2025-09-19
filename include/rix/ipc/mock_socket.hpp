#pragma once

#include <queue>
#include <vector>

#include "rix/ipc/generic_socket.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include <gmock/gmock.h>

namespace rix::ipc {

class MockSocket : public GenericSocket {
public:
  mutable Endpoint local_endpoint_;
  mutable Endpoint remote_endpoint_;
  mutable bool is_listening = false;
  mutable bool is_connected = false;
  mutable bool blocking = true;
  mutable bool reuse_address = false;
  mutable int backlog = 0;
  mutable std::vector<std::shared_ptr<rix::msg::Message>> recv_buffer;
  mutable std::vector<std::shared_ptr<GenericSocket>> accepted_sockets;

  MockSocket() {
    ON_CALL(*this, bind).WillByDefault([this](const Endpoint &endpoint) -> bool {
      this->local_endpoint_ = endpoint;
      return true;
    });
    ON_CALL(*this, listen).WillByDefault([this](int backlog) -> bool {
      this->backlog = backlog;
      return true;
    });
    ON_CALL(*this, accept).WillByDefault([this](Endpoint &endpoint) -> std::shared_ptr<GenericSocket> {
      auto socket = std::make_shared<MockSocket>();
      socket->is_connected = true;
      this->accepted_sockets.push_back(socket);
      return socket;
    });
    ON_CALL(*this, connect).WillByDefault([this](const Endpoint &endpoint) -> bool {
      if (this->is_connected) {
        return false;
      }
      if (this->is_listening) {
        return false;
      }
      this->is_connected = true;
      return true;
    });
    ON_CALL(*this, close).WillByDefault([this]() -> void {});

    ON_CALL(*this, send).WillByDefault([this](const void *buf, size_t len, int flags) -> ssize_t { return -1; });
    ON_CALL(*this, recv).WillByDefault([this](void *buf, size_t len, int flags) -> ssize_t { return -1; });

    ON_CALL(*this, send_message).WillByDefault([this](uint8_t opcode, const rix::msg::Message &msg) -> bool {
      return true;
    });
    ON_CALL(*this, recv_message).WillByDefault([this](rix::msg::Message &msg, size_t len) -> bool { return true; });

    ON_CALL(*this, wait_readable).WillByDefault([this](const rix::util::Duration &timeout) -> bool {
      return !this->recv_buffer.empty();
    });
    ON_CALL(*this, wait_writable).WillByDefault([this](const rix::util::Duration &timeout) -> bool {
      return this->is_connected;
    });
    ON_CALL(*this, wait_exception).WillByDefault([this](const rix::util::Duration &timeout) -> bool { return false; });

    ON_CALL(*this, set_blocking).WillByDefault([this](bool blocking) -> bool {
      this->blocking = blocking;
      return true;
    });
    ON_CALL(*this, get_blocking).WillByDefault([this]() -> bool { return this->blocking; });
    ON_CALL(*this, set_reuse_address).WillByDefault([this](bool reuse) -> bool {
      this->reuse_address = reuse;
      return true;
    });
    ON_CALL(*this, get_reuse_address).WillByDefault([this]() -> bool { return this->reuse_address; });

    ON_CALL(*this, local_endpoint).WillByDefault([this]() -> Endpoint { return this->local_endpoint_; });
    ON_CALL(*this, remote_endpoint).WillByDefault([this]() -> Endpoint { return this->remote_endpoint_; });
  }

  ~MockSocket() { close(); }

  MOCK_METHOD(bool, bind, (const Endpoint &endpoint), (const, override));
  MOCK_METHOD(bool, listen, (int backlog), (const, override));
  MOCK_METHOD(std::shared_ptr<GenericSocket>, accept, (Endpoint & endpoint), (const, override));
  MOCK_METHOD(bool, connect, (const Endpoint &endpoint), (const, override));
  MOCK_METHOD(void, close, (), (const, override));

  MOCK_METHOD(ssize_t, send, (const void *buf, size_t len, int flags), (const, override));
  MOCK_METHOD(ssize_t, recv, (void *buf, size_t len, int flags), (const, override));

  MOCK_METHOD(bool, send_message, (uint8_t opcode, const rix::msg::Message &msg), (const, override));
  MOCK_METHOD(bool, recv_message, (rix::msg::Message & msg, size_t len), (const, override));

  MOCK_METHOD(bool, wait_readable, (const rix::util::Duration &timeout), (const, override));
  MOCK_METHOD(bool, wait_writable, (const rix::util::Duration &timeout), (const, override));
  MOCK_METHOD(bool, wait_exception, (const rix::util::Duration &timeout), (const, override));

  MOCK_METHOD(bool, set_blocking, (bool blocking), (const, override));
  MOCK_METHOD(bool, get_blocking, (), (const, override));
  MOCK_METHOD(bool, set_reuse_address, (bool reuse), (const, override));
  MOCK_METHOD(bool, get_reuse_address, (), (const, override));

  MOCK_METHOD(Endpoint, local_endpoint, (), (const, override));
  MOCK_METHOD(Endpoint, remote_endpoint, (), (const, override));
};

} // namespace rix::ipc