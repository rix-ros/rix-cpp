#pragma once

#include <condition_variable>
#include <mutex>

#include "rix/ipc/acceptor.hpp"
#include <gmock/gmock.h>

namespace rix {

class MockAcceptor : public Acceptor {
public:
  MockAcceptor() {
    ON_CALL(*this, accept).WillByDefault([](Endpoint&) { return nullptr; });
    ON_CALL(*this, set_blocking).WillByDefault([](bool) { return false; });
    ON_CALL(*this, get_blocking).WillByDefault([]() { return false; });
    ON_CALL(*this, local_endpoint).WillByDefault([]() { return Endpoint{}; });
    ON_CALL(*this, remote_endpoint).WillByDefault([]() { return Endpoint{}; });
    ON_CALL(*this, wait_readable).WillByDefault([](const Duration&) { return false; });
    ON_CALL(*this, wait_writable).WillByDefault([](const Duration&) { return false; });
    ON_CALL(*this, wait_exception).WillByDefault([](const Duration&) { return false; });
  }

  ~MockAcceptor() override = default;

  MockAcceptor(const MockAcceptor&) = delete;
  MockAcceptor& operator=(const MockAcceptor&) = delete;
  MockAcceptor(MockAcceptor&&) = delete;
  MockAcceptor& operator=(MockAcceptor&&) = delete;

  MOCK_METHOD(std::shared_ptr<Stream>, accept, (Endpoint & endpoint), (const, override));
  MOCK_METHOD(bool, set_blocking, (bool blocking), (const, override));
  MOCK_METHOD(bool, get_blocking, (), (const, override));
  MOCK_METHOD(Endpoint, local_endpoint, (), (const, override));
  MOCK_METHOD(Endpoint, remote_endpoint, (), (const, override));
  MOCK_METHOD(bool, wait_readable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_writable, (const Duration& timeout), (const, override));
  MOCK_METHOD(bool, wait_exception, (const Duration& timeout), (const, override));

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
  mutable std::mutex sync_mutex_;
  std::condition_variable sync_cv_;
  size_t operation_count_ = 0;
};

} // namespace rix
