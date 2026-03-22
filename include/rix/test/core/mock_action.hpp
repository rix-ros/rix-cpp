#pragma once

#include "rix/core/action.hpp"

#include <functional>
#include <string>

namespace rix {
namespace test {

/**
 * @brief Lightweight Action (server) mock.
 *
 * The node-under-test registers a callback via set_callback().  Tests call
 * process() (through ActionCapture) to exercise the callback directly,
 * without any network involvement.
 *
 * The optional goal/preempt callbacks (set_goal_callback / set_preempt_callback)
 * are stored and can be invoked by tests to simulate server-side events.
 */
class MockAction final : public Action {
public:
  explicit MockAction(std::string name) : name_(std::move(name)) {}

  // Action interface -----------------------------------------------------

  void set_goal_callback(std::function<void()> cb) override { goal_cb_ = std::move(cb); }
  void set_preempt_callback(std::function<void()> cb) override { preempt_cb_ = std::move(cb); }

  // Accessors ------------------------------------------------------------

  const std::string& name() const { return name_; }

  /**
   * @brief Invoke the action callback synchronously.
   *
   * @param goal     Incoming goal message.
   * @param feedback Feedback message filled in during execution.
   * @param result   Result message filled in on completion.
   * @return Return value of the callback, or false if no callback registered.
   */
  bool process(const Message& goal, Message& feedback, Message& result) {
    if (callback_) {
      return callback_(goal, feedback, result);
    }
    return false;
  }

  /** Trigger the goal callback (simulates a new goal arriving). */
  void trigger_goal() {
    if (goal_cb_) goal_cb_();
  }

  /** Trigger the preempt callback (simulates a preempt request). */
  void trigger_preempt() {
    if (preempt_cb_) preempt_cb_();
  }

protected:
  void on_spin() override {}

private:
  // Action's private virtual: stores the callback for later invocation.
  void set_callback(CallbackUntyped cb,
                    std::shared_ptr<Message> /*goal*/,
                    std::shared_ptr<Message> /*feedback*/,
                    std::shared_ptr<Message> /*result*/) override {
    callback_ = std::move(cb);
  }

  std::string name_;
  CallbackUntyped callback_;
  std::function<void()> goal_cb_;
  std::function<void()> preempt_cb_;
};

} // namespace test
} // namespace rix
