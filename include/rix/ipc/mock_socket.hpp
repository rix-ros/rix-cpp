#pragma once

#include "rix/ipc/client.hpp"
#include "rix/ipc/generic_socket.hpp"
#include "rix/ipc/server.hpp"
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
  mutable std::vector<std::vector<uint8_t>> send_buffer;
  mutable std::vector<std::vector<uint8_t>> recv_buffer;
  mutable std::vector<GenericSocket *> accepted_sockets;

  void push_recv_message(const rix::msg::Message &msg) {
    std::vector<uint8_t> data(msg.size());
    size_t offset = 0;
    msg.serialize(data.data(), offset);
    recv_buffer.push_back(std::move(data));
  }

  bool pop_send_message(rix::msg::Message &msg) {
    if (send_buffer.empty()) {
      return false;
    }
    auto &data = send_buffer.front();
    size_t offset = 0;
    msg.deserialize(data.data(), offset);
    send_buffer.erase(send_buffer.begin());
    return true;
  }

  MockSocket() {
    ON_CALL(*this, bind)
        .WillByDefault([this](const Endpoint &endpoint) -> bool {
          this->local_endpoint_ = endpoint;
          return true;
        });
    ON_CALL(*this, listen).WillByDefault([this](int backlog) -> bool {
      this->backlog = backlog;
      return true;
    });
    ON_CALL(*this, accept)
        .WillByDefault([this](Endpoint &remote_endpoint)
                           -> std::unique_ptr<GenericSocket> {
          auto socket = std::make_unique<MockSocket>();
          socket->is_connected = true;
          this->accepted_sockets.push_back(socket.get());
          return socket;
        });
    ON_CALL(*this, connect)
        .WillByDefault([this](const Endpoint &endpoint) -> bool {
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

    ON_CALL(*this, send)
        .WillByDefault(
            [this](const void *buf, size_t len, int flags) -> ssize_t {
              std::vector<uint8_t> data(len);
              std::memcpy(data.data(), buf, len);
              this->send_buffer.push_back(std::move(data));
              return len;
            });
    ON_CALL(*this, recv)
        .WillByDefault([this](void *buf, size_t len, int flags) -> ssize_t {
          if (this->recv_buffer.empty()) {
            return 0;
          }
          auto &data = this->recv_buffer.front();
          size_t to_copy = std::min(len, data.size());
          std::memcpy(buf, data.data(), to_copy);
          if (to_copy < data.size()) {
            data.erase(data.begin(), data.begin() + to_copy);
          } else {
            this->recv_buffer.erase(this->recv_buffer.begin());
          }
          return to_copy;
        });
    ON_CALL(*this, send_to)
        .WillByDefault([this](const void *buf, size_t len,
                              const Endpoint &endpoint, int flags) -> ssize_t {
          return this->send(buf, len, flags);
        });
    ON_CALL(*this, recv_from)
        .WillByDefault([this](void *buf, size_t len, Endpoint &endpoint,
                              int flags) -> ssize_t {
          return this->recv(buf, len, flags);
        });

    ON_CALL(*this, wait_readable)
        .WillByDefault([this](const rix::util::Duration &timeout) -> bool {
          return !this->recv_buffer.empty();
        });
    ON_CALL(*this, wait_writable)
        .WillByDefault([this](const rix::util::Duration &timeout) -> bool {
          return this->is_connected;
        });
    ON_CALL(*this, wait_exception)
        .WillByDefault([this](const rix::util::Duration &timeout) -> bool {
          return false;
        });

    ON_CALL(*this, set_blocking).WillByDefault([this](bool blocking) -> bool {
      this->blocking = blocking;
      return true;
    });
    ON_CALL(*this, get_blocking).WillByDefault([this]() -> bool {
      return this->blocking;
    });
    ON_CALL(*this, set_reuse_address).WillByDefault([this](bool reuse) -> bool {
      this->reuse_address = reuse;
      return true;
    });
    ON_CALL(*this, get_reuse_address).WillByDefault([this]() -> bool {
      return this->reuse_address;
    });

    ON_CALL(*this, local_endpoint).WillByDefault([this]() -> Endpoint {
      return this->local_endpoint_;
    });
    ON_CALL(*this, remote_endpoint).WillByDefault([this]() -> Endpoint {
      return this->remote_endpoint_;
    });
  }

  MOCK_METHOD(bool, bind, (const Endpoint &endpoint), (const, override));
  MOCK_METHOD(bool, listen, (int backlog), (const, override));
  MOCK_METHOD(std::unique_ptr<GenericSocket>, accept,
              (Endpoint & remote_endpoint), (const, override));
  MOCK_METHOD(bool, connect, (const Endpoint &endpoint), (const, override));
  MOCK_METHOD(void, close, (), (const, override));

  MOCK_METHOD(ssize_t, send, (const void *buf, size_t len, int flags),
              (const, override));
  MOCK_METHOD(ssize_t, recv, (void *buf, size_t len, int flags),
              (const, override));
  MOCK_METHOD(ssize_t, send_to,
              (const void *buf, size_t len, const Endpoint &endpoint,
               int flags),
              (const, override));
  MOCK_METHOD(ssize_t, recv_from,
              (void *buf, size_t len, Endpoint &endpoint, int flags),
              (const, override));

  MOCK_METHOD(bool, wait_readable, (const rix::util::Duration &timeout),
              (const, override));
  MOCK_METHOD(bool, wait_writable, (const rix::util::Duration &timeout),
              (const, override));
  MOCK_METHOD(bool, wait_exception, (const rix::util::Duration &timeout),
              (const, override));

  MOCK_METHOD(bool, set_blocking, (bool blocking), (const, override));
  MOCK_METHOD(bool, get_blocking, (), (const, override));
  MOCK_METHOD(bool, set_reuse_address, (bool reuse), (const, override));
  MOCK_METHOD(bool, get_reuse_address, (), (const, override));

  MOCK_METHOD(Endpoint, local_endpoint, (), (const, override));
  MOCK_METHOD(Endpoint, remote_endpoint, (), (const, override));
};

// Factory methods for convenience

static inline std::shared_ptr<Server>
create_mock_server(const rix::ipc::Endpoint &endpoint) {
  return std::make_shared<rix::ipc::Server>(
      std::move(std::make_unique<MockSocket>()), endpoint);
}

static inline std::shared_ptr<Client> create_mock_client() {
  return std::make_shared<rix::ipc::Client>(
      std::move(std::make_unique<MockSocket>()));
}

} // namespace rix::ipc