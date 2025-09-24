#include "rix/core/timer.hpp"

namespace rix::core {

Timer::Timer(const rix::util::Duration &duration, Callback callback) : duration_(duration), callback_(callback) {
  event_.current_real = rix::util::Time::now();
  event_.current_expected = event_.last_expected = event_.last_real = rix::util::Time(0.0);
  event_.last_duration = rix::util::Duration(0.0);
}

Timer::~Timer() {}

bool Timer::ok() const { return !shutdown_flag_; }

void Timer::shutdown() { shutdown_flag_ = true; }

void Timer::spin_once() {
  event_.current_real = rix::util::Time::now();
  if (event_.current_real - event_.last_real > duration_) {
    std::lock_guard<std::mutex> guard(callback_mutex_);
    event_.last_duration = event_.current_real - event_.last_real;
    if (event_.current_expected == 0.0) {
      event_.current_expected = event_.current_real;
    } else {
      event_.current_expected += duration_;
    }
    callback_(event_);
    event_.last_real = event_.current_real;
    event_.last_expected = event_.current_expected;
  }
}

void Timer::set_callback(Callback callback) { callback_ = callback; }

Timer::Callback Timer::get_callback() const { return callback_; }

} // namespace rix::core