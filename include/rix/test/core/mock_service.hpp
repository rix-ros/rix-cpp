#pragma once

#include <string>

#include "rix/core/service.hpp"

namespace rix {
namespace test {

/**
 * @brief Lightweight Service mock.
 *
 * The node-under-test registers a callback via set_callback().  Tests call
 * process() (through ServiceCapture) to exercise the callback directly,
 * without any network involvement.
 */
class MockService final : public Service {
public:
  explicit MockService(std::string name) : name_(std::move(name)) {}

  // Accessors ------------------------------------------------------------

  const std::string& name() const { return name_; }

  /**
   * @brief Invoke the service callback synchronously.
   *
   * @param request  Incoming request message.
   * @param response Response message filled in by the callback.
   * @return true if a callback was registered and executed, false otherwise.
   */
  bool process(const Message& request, Message& response) {
    if (callback_) {
      callback_(request, response);
      return true;
    }
    return false;
  }

protected:
  void on_spin() override {}

private:
  // Service's private virtual: stores the callback for later invocation.
  void set_callback(CallbackUntyped cb,
                    std::shared_ptr<Message> /*req_prototype*/,
                    std::shared_ptr<Message> /*res_prototype*/) override {
    callback_ = std::move(cb);
  }

  std::string name_;
  CallbackUntyped callback_;
};

} // namespace test
} // namespace rix
