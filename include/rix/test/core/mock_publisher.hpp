#pragma once

#include <functional>
#include <string>
#include <vector>

#include "rix/core/publisher.hpp"

namespace rix {
namespace test {

/**
 * @brief Lightweight Publisher mock.
 *
 * Every publish() invocation is forwarded to all registered observers.
 * NodeTestHarness registers an observer that appends the typed message to its
 * PublisherCapture.  No network or IPC is involved.
 */
class MockPublisher final : public Publisher {
public:
  MockPublisher(std::string topic, size_t subscriber_count)
      : topic_(std::move(topic)), subscriber_count_(subscriber_count) {}

  // Publisher interface --------------------------------------------------

  void publish(const Message& msg) override {
    for (auto& cb : observers_) {
      cb(msg);
    }
  }

  size_t get_subscriber_count() const override { return subscriber_count_; }

  // Accessors ------------------------------------------------------------

  const std::string& topic() const { return topic_; }

  /** Called by NodeTestHarness after the node is constructed. */
  void add_observer(std::function<void(const Message&)> cb) { observers_.push_back(std::move(cb)); }

  void set_subscriber_count(size_t n) { subscriber_count_ = n; }

protected:
  void on_spin() override {}

private:
  std::string topic_;
  size_t subscriber_count_;
  std::vector<std::function<void(const Message&)>> observers_;
};

} // namespace test
} // namespace rix
