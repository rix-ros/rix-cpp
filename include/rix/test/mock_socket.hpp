#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

#include "rix/ipc/generic_socket.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include <gmock/gmock.h>

namespace rix {

class MockSocket : public GenericSocket {
public:
  MockSocket() {
    ON_CALL(*this, bind).WillByDefault([](const Endpoint& endpoint) -> bool { return false; });
    ON_CALL(*this, listen).WillByDefault([](int backlog) -> bool { return false; });
    ON_CALL(*this, accept).WillByDefault([](Endpoint& endpoint) { return nullptr; });
    ON_CALL(*this, connect).WillByDefault([](const Endpoint& endpoint) -> bool { return false; });
    ON_CALL(*this, close).WillByDefault([]() -> void {});

    ON_CALL(*this, send).WillByDefault([](const void* buf, size_t len, int flags) -> ssize_t { return -1; });
    ON_CALL(*this, recv).WillByDefault([](void* buf, size_t len, int flags) -> ssize_t { return -1; });

    ON_CALL(*this, send_message).WillByDefault([](uint8_t opcode, const Message& msg) -> bool { return false; });
    ON_CALL(*this, recv_message).WillByDefault([](Message& msg, size_t len) -> bool { return false; });

    ON_CALL(*this, wait_readable).WillByDefault([](const Duration& timeout) -> bool { return false; });
    ON_CALL(*this, wait_writable).WillByDefault([](const Duration& timeout) -> bool { return false; });
    ON_CALL(*this, wait_exception).WillByDefault([](const Duration& timeout) -> bool { return false; });

    ON_CALL(*this, set_blocking).WillByDefault([](bool blocking) -> bool { return false; });
    ON_CALL(*this, get_blocking).WillByDefault([]() -> bool { return false; });
    ON_CALL(*this, set_reuse_address).WillByDefault([](bool reuse) -> bool { return false; });
    ON_CALL(*this, get_reuse_address).WillByDefault([]() -> bool { return false; });
    ON_CALL(*this, local_endpoint).WillByDefault([]() -> Endpoint { return {}; });
    ON_CALL(*this, remote_endpoint).WillByDefault([]() -> Endpoint { return {}; });
  }

  ~MockSocket() override { MockSocket::close(); }

  MockSocket(const MockSocket&) = delete;
  MockSocket& operator=(const MockSocket&) = delete;
  MockSocket(MockSocket&&) = delete;
  MockSocket& operator=(MockSocket&&) = delete;

  MOCK_METHOD(bool, bind, (const Endpoint& endpoint), (const, override));
  MOCK_METHOD(bool, listen, (int backlog), (const, override));
  MOCK_METHOD(std::shared_ptr<GenericSocket>, accept, (Endpoint & endpoint), (const, override));
  MOCK_METHOD(bool, connect, (const Endpoint& endpoint), (const, override));
  MOCK_METHOD(void, close, (), (const, override));

  MOCK_METHOD(ssize_t, send, (const void* buf, size_t len, int flags), (const, override));
  MOCK_METHOD(ssize_t, recv, (void* buf, size_t len, int flags), (const, override));

  MOCK_METHOD(bool, send_message, (uint8_t opcode, const Message& msg), (const, override));
  MOCK_METHOD(bool, recv_message, (Message & msg, size_t len), (const, override));

  MOCK_METHOD(bool, wait_readable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_writable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_exception, (const Duration& timeout), (const, override));

  MOCK_METHOD(bool, set_blocking, (bool blocking), (const, override));
  MOCK_METHOD(bool, get_blocking, (), (const, override));
  MOCK_METHOD(bool, set_reuse_address, (bool reuse), (const, override));
  MOCK_METHOD(bool, get_reuse_address, (), (const, override));

  MOCK_METHOD(Endpoint, local_endpoint, (), (const, override));
  MOCK_METHOD(Endpoint, remote_endpoint, (), (const, override));

  // Synchronization support for multithreaded tests
  // Allows tests to wait for specific operations to complete instead of sleeping

  // Notify that an operation has completed
  void notify_operation_complete() {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    operation_count_++;
    sync_cv_.notify_all();
  }

  // Wait for a specific number of operations to complete
  // Returns true if the count was reached, false if timeout occurred
  bool wait_for_operations(size_t expected_count, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
    std::unique_lock<std::mutex> lock(sync_mutex_);
    return sync_cv_.wait_for(lock, timeout, [this, expected_count]() { return operation_count_ >= expected_count; });
  }

  // Reset the operation counter
  void reset_operation_count() {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    operation_count_ = 0;
  }

  // Get current operation count
  size_t get_operation_count() const {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    return operation_count_;
  }

private:
  mutable std::mutex sync_mutex_;
  std::condition_variable sync_cv_;
  size_t operation_count_ = 0;
};

} // namespace rix