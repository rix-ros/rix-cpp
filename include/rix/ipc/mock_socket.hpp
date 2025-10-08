#pragma once

#include <queue>
#include <vector>

#include "rix/ipc/generic_socket.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include <gmock/gmock.h>

namespace rix {

class MockSocket : public GenericSocket {
public:
  MockSocket() {
    ON_CALL(*this, bind).WillByDefault([](const Endpoint& endpoint) -> bool {
      return false;
    });
    ON_CALL(*this, listen).WillByDefault([](int backlog) -> bool { return false; });
    ON_CALL(*this, accept).WillByDefault([](Endpoint& endpoint) { return nullptr; });
    ON_CALL(*this, connect).WillByDefault([](const Endpoint& endpoint) -> bool {
      return false;
    });
    ON_CALL(*this, close).WillByDefault([]() -> void {});

    ON_CALL(*this, send)
        .WillByDefault(
            [](const void* buf, size_t len, int flags) -> ssize_t { return -1; });
    ON_CALL(*this, recv).WillByDefault([](void* buf, size_t len, int flags) -> ssize_t {
      return -1;
    });

    ON_CALL(*this, send_message)
        .WillByDefault(
            [](uint8_t opcode, const msg::Message& msg) -> bool { return false; });
    ON_CALL(*this, recv_message).WillByDefault([](msg::Message& msg, size_t len) -> bool {
      return false;
    });

    ON_CALL(*this, wait_readable).WillByDefault([](const Duration& timeout) -> bool {
      return false;
    });
    ON_CALL(*this, wait_writable).WillByDefault([](const Duration& timeout) -> bool {
      return false;
    });
    ON_CALL(*this, wait_exception).WillByDefault([](const Duration& timeout) -> bool {
      return false;
    });

    ON_CALL(*this, set_blocking).WillByDefault([](bool blocking) -> bool {
      return false;
    });
    ON_CALL(*this, get_blocking).WillByDefault([]() -> bool { return false; });
    ON_CALL(*this, set_reuse_address).WillByDefault([](bool reuse) -> bool {
      return false;
    });
    ON_CALL(*this, get_reuse_address).WillByDefault([]() -> bool { return false; });
    ON_CALL(*this, local_endpoint).WillByDefault([]() -> Endpoint { return {}; });
    ON_CALL(*this, remote_endpoint).WillByDefault([]() -> Endpoint { return {}; });
  }

  MockSocket(const MockSocket&) = delete;
  MockSocket& operator=(const MockSocket&) = delete;
  MockSocket(MockSocket&&) = delete;
  MockSocket& operator=(MockSocket&&) = delete;

  ~MockSocket() { close(); }

  MOCK_METHOD(bool, bind, (const Endpoint& endpoint), (const, override));
  MOCK_METHOD(bool, listen, (int backlog), (const, override));
  MOCK_METHOD(std::shared_ptr<GenericSocket>,
              accept,
              (Endpoint & endpoint),
              (const, override));
  MOCK_METHOD(bool, connect, (const Endpoint& endpoint), (const, override));
  MOCK_METHOD(void, close, (), (const, override));

  MOCK_METHOD(ssize_t, send, (const void* buf, size_t len, int flags), (const, override));
  MOCK_METHOD(ssize_t, recv, (void* buf, size_t len, int flags), (const, override));

  MOCK_METHOD(bool,
              send_message,
              (uint8_t opcode, const msg::Message& msg),
              (const, override));
  MOCK_METHOD(bool, recv_message, (msg::Message & msg, size_t len), (const, override));

  MOCK_METHOD(bool, wait_readable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_writable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_exception, (const Duration& timeout), (const, override));

  MOCK_METHOD(bool, set_blocking, (bool blocking), (const, override));
  MOCK_METHOD(bool, get_blocking, (), (const, override));
  MOCK_METHOD(bool, set_reuse_address, (bool reuse), (const, override));
  MOCK_METHOD(bool, get_reuse_address, (), (const, override));

  MOCK_METHOD(Endpoint, local_endpoint, (), (const, override));
  MOCK_METHOD(Endpoint, remote_endpoint, (), (const, override));
};

} // namespace rix