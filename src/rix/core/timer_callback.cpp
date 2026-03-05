#include "rix/core/timer_callback.hpp"

namespace rix {

TimerCallback::TimerCallback(const Duration& duration, Callback callback) : duration_(duration), callback_(callback) {
  event_.current_real = Time::now();
  event_.current_expected = event_.last_expected = event_.last_real = Time(0.0);
  event_.last_duration = Duration(0.0);

  if (MULTITHREADED) {
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

TimerCallback::~TimerCallback() {
  if (MULTITHREADED) {
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

void TimerCallback::on_spin() {
  static const Duration ms = Duration(1e-3);
  static const Duration margin = Duration(1e-4); // 0.1 ms
  static const Duration d_zero = Duration(0.0);
  static const Time t_zero = Time(0.0);

  event_.current_real = Time::now();
  Duration delta = event_.current_real - event_.last_real;
  Duration remaining = duration_ - delta;

  if (remaining >= ms) {
    if (MULTITHREADED) {
      Time::sleep_for(remaining - margin);
    }
    return;
  } else if (remaining > d_zero) {
    return;
  }

  event_.last_duration = delta;
  if (event_.current_expected == t_zero) {
    event_.current_expected = event_.current_real;
  } else {
    event_.current_expected += duration_;
  }

  std::lock_guard<std::mutex> guard(callback_mutex_);
  callback_(event_);
  event_.last_real = event_.current_real;
  event_.last_expected = event_.current_expected;
}

void TimerCallback::set_callback(Callback callback) { callback_ = callback; }

TimerCallback::Callback TimerCallback::get_callback() const { return callback_; }

} // namespace rix