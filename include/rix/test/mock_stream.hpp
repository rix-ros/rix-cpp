#pragma once

#include <condition_variable>
#include <mutex>

#include "rix/ipc/stream.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include <gmock/gmock.h>

namespace rix {

class MockStream : public Stream {
public:
  MockStream() {
    ON_CALL(*this, set_blocking).WillByDefault([](bool) { return false; });
    ON_CALL(*this, get_blocking).WillByDefault([]() { return false; });
    ON_CALL(*this, local_endpoint).WillByDefault([]() { return Endpoint{}; });
    ON_CALL(*this, remote_endpoint).WillByDefault([]() { return Endpoint{}; });
    ON_CALL(*this, send_message).WillByDefault([](uint8_t, const Message&) { return false; });
    ON_CALL(*this, recv_message).WillByDefault([](Message&, size_t) { return false; });
    ON_CALL(*this, wait_readable).WillByDefault([](const Duration&) { return false; });
    ON_CALL(*this, wait_writable).WillByDefault([](const Duration&) { return false; });
    ON_CALL(*this, wait_exception).WillByDefault([](const Duration&) { return false; });
  }

  ~MockStream() override = default;

  MockStream(const MockStream&) = delete;
  MockStream& operator=(const MockStream&) = delete;
  MockStream(MockStream&&) = delete;
  MockStream& operator=(MockStream&&) = delete;

  // High-level message operations (override Stream's virtual methods)
  MOCK_METHOD(bool, send_message, (uint8_t opcode, const Message& msg), (const, override));
  MOCK_METHOD(bool, recv_message, (Message & msg, size_t prefix_len), (const, override));

  // Pollable interface
  MOCK_METHOD(bool, wait_readable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_writable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_exception, (const Duration& timeout), (const, override));

  // Stream control
  MOCK_METHOD(bool, set_blocking, (bool blocking), (const, override));
  MOCK_METHOD(bool, get_blocking, (), (const, override));
  MOCK_METHOD(Endpoint, local_endpoint, (), (const, override));
  MOCK_METHOD(Endpoint, remote_endpoint, (), (const, override));

  // Synchronization support for multithreaded tests
  void notify_operation_complete() {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    operation_count_++;
    sync_cv_.notify_all();
  }

  bool wait_for_operations(size_t expected_count, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
    std::unique_lock<std::mutex> lock(sync_mutex_);
    return sync_cv_.wait_for(lock, timeout, [this, expected_count]() { return operation_count_ >= expected_count; });
  }

  void reset_operation_count() {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    operation_count_ = 0;
  }

  size_t get_operation_count() const {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    return operation_count_;
  }

private:
  // Low-level I/O — unused because send_message/recv_message are mocked,
  // but required because Stream declares them pure virtual.
  ssize_t send(const uint8_t*, size_t, int) const override { return -1; }
  ssize_t recv(uint8_t*, size_t, int) const override { return -1; }

  mutable std::mutex sync_mutex_;
  std::condition_variable sync_cv_;
  size_t operation_count_ = 0;
};

} // namespace rix
