#pragma once

#include "rix/core/subscriber.hpp"

#include <deque>
#include <memory>
#include <string>

namespace rix {
namespace test {

/**
 * @brief Lightweight Subscriber mock.
 *
 * Messages queued via enqueue() are delivered to the registered callback on
 * the next spin.  NodeTestHarness wires the SubscriberInjector to this class
 * so that inject() calls translate directly into enqueue() calls.
 */
class MockSubscriber final : public Subscriber {
public:
  explicit MockSubscriber(std::string topic, size_t publisher_count = 1)
      : topic_(std::move(topic)), publisher_count_(publisher_count) {}

  // Subscriber interface -------------------------------------------------

  size_t get_publisher_count() const override { return publisher_count_; }

  // Accessors ------------------------------------------------------------

  const std::string& topic() const { return topic_; }

  /** Queue a message to be delivered on the next spin_once() call. */
  void enqueue(std::shared_ptr<Message> msg) { pending_.push_back(std::move(msg)); }

protected:
  void on_spin() override {
    while (!pending_.empty()) {
      if (callback_) {
        callback_(*pending_.front());
      }
      pending_.pop_front();
    }
  }

private:
  // Subscriber's private virtual: stores the callback for later dispatch.
  void set_callback(CallbackUntyped cb, std::shared_ptr<Message> /*prototype*/) override { callback_ = std::move(cb); }

  std::string topic_;
  size_t publisher_count_;
  CallbackUntyped callback_;
  std::deque<std::shared_ptr<Message>> pending_;
};

} // namespace test
} // namespace rix
