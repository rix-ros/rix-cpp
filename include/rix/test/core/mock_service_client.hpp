#pragma once

#include "rix/core/service_client.hpp"

#include <functional>
#include <string>

namespace rix {
namespace test {

/**
 * @brief Lightweight ServiceClient mock.
 *
 * Every call() invocation is forwarded to the handler installed by
 * NodeTestHarness.  The handler captures the typed request and returns a
 * pre-queued response — no network or IPC involved.
 */
class MockServiceClient final : public ServiceClient {
public:
  using CallHandler = std::function<bool(const Message& request, Message& response)>;

  explicit MockServiceClient(std::string name) : name_(std::move(name)) {}

  // ServiceClient interface ----------------------------------------------

  bool call(const Message& request, Message& response) override {
    if (handler_) {
      return handler_(request, response);
    }
    return false;
  }

  // Accessors ------------------------------------------------------------

  const std::string& name() const { return name_; }

  /** Called by NodeTestHarness after the node is constructed. */
  void set_call_handler(CallHandler handler) { handler_ = std::move(handler); }

protected:
  void on_spin() override {}

private:
  std::string name_;
  CallHandler handler_;
};

} // namespace test
} // namespace rix
