#pragma once

#include "rix/core/timer_callback.hpp"

#include <functional>
#include <string>
#include <vector>

namespace rix {
namespace test {

// ============================================================================
// MockTimerCallback
// ============================================================================

/**
 * @brief Timer mock driven by the MockClock.
 *
 * Fires the callback whenever the elapsed time since the last firing equals or
 * exceeds the configured duration.  Because the clock only advances when the
 * test calls advance_time(), no real wall-clock time elapses.
 */
class MockTimerCallback final : public TimerCallback {
public:
  MockTimerCallback(const Duration& duration, TimerCallback::Callback callback)
      : duration_(duration), callback_(std::move(callback)) {}

  void set_callback(TimerCallback::Callback cb) override { callback_ = std::move(cb); }
  TimerCallback::Callback get_callback() const override { return callback_; }

protected:
  void on_spin() override {
    const Time now = Time::now();
    if (callback_ && (now - last_fired_) >= duration_) {
      Event event;
      event.last_real = last_fired_;
      event.current_real = now;
      event.last_duration = now - last_fired_;
      callback_(event);
      last_fired_ = now;
    }
  }

private:
  Duration duration_;
  TimerCallback::Callback callback_;
  Time last_fired_; // default-constructed to epoch (t = 0)
};

} // namespace test
} // namespace rix