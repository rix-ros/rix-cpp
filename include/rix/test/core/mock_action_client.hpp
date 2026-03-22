#pragma once

#include "rix/core/action_client.hpp"

#include <functional>
#include <memory>
#include <string>

namespace rix {
namespace test {

/**
 * @brief Lightweight ActionClient mock.
 *
 * dispatch(), cancel(), and wait_for_result() are forwarded to optional
 * handler functions installed by NodeTestHarness.
 *
 * Use inject_feedback() / inject_result() to simulate server responses and
 * exercise the student's feedback/result callbacks without any networking.
 */
class MockActionClient final : public ActionClient {
public:
  using DispatchHandler = std::function<bool(const Message& goal)>;
  using CancelHandler = std::function<bool()>;
  using WaitHandler = std::function<bool(const Duration& timeout)>;

  explicit MockActionClient(std::string name) : name_(std::move(name)) {}

  // ActionClient interface -----------------------------------------------

  bool dispatch(const Message& goal) override {
    if (dispatch_handler_) return dispatch_handler_(goal);
    return true;
  }

  bool cancel() override {
    if (cancel_handler_) return cancel_handler_();
    return true;
  }

  bool wait_for_result(const Duration& timeout) override {
    if (wait_handler_) return wait_handler_(timeout);
    return true;
  }

  // Accessors ------------------------------------------------------------

  const std::string& name() const { return name_; }

  void set_dispatch_handler(DispatchHandler h) { dispatch_handler_ = std::move(h); }
  void set_cancel_handler(CancelHandler h) { cancel_handler_ = std::move(h); }
  void set_wait_handler(WaitHandler h) { wait_handler_ = std::move(h); }

  /** Deliver a feedback message to the registered feedback callback. */
  void inject_feedback(std::shared_ptr<Message> feedback) {
    if (feedback_cb_ && feedback) feedback_cb_(*feedback);
  }

  /** Deliver a result message to the registered result callback. */
  void inject_result(std::shared_ptr<Message> result) {
    if (result_cb_ && result) result_cb_(*result);
  }

protected:
  void on_spin() override {}

private:
  // ActionClient private virtuals: store callbacks for later invocation.
  void set_feedback_callback(CallbackUntyped cb,
                             std::shared_ptr<Message> /*prototype*/) override {
    feedback_cb_ = std::move(cb);
  }

  void set_result_callback(CallbackUntyped cb,
                           std::shared_ptr<Message> /*prototype*/) override {
    result_cb_ = std::move(cb);
  }

  std::string name_;
  DispatchHandler dispatch_handler_;
  CancelHandler cancel_handler_;
  WaitHandler wait_handler_;
  CallbackUntyped feedback_cb_;
  CallbackUntyped result_cb_;
};

} // namespace test
} // namespace rix
