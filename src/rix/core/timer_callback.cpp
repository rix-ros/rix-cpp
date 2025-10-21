#include "rix/core/timer_callback.hpp"

namespace rix {

TimerCallback::TimerCallback(const Duration& duration, Callback callback) : duration_(duration), callback_(callback) {
  event_.current_real = Time::now();
  event_.current_expected = event_.last_expected = event_.last_real = Time(0.0);
  event_.last_duration = Duration(0.0);

#ifdef RIX_MULTITHREADED
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

TimerCallback::~TimerCallback() {
#ifdef RIX_MULTITHREADED
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif
}

void TimerCallback::on_spin() {
  event_.current_real = Time::now();
  if (event_.current_real - event_.last_real >= duration_) {
    event_.last_duration = event_.current_real - event_.last_real;
    if (event_.current_expected == 0.0) {
      event_.current_expected = event_.current_real;
    } else {
      event_.current_expected += duration_;
    }
    std::lock_guard<std::mutex> guard(callback_mutex_);
    callback_(event_);
    event_.last_real = event_.current_real;
    event_.last_expected = event_.current_expected;
  }
}

void TimerCallback::set_callback(Callback callback) { callback_ = callback; }

TimerCallback::Callback TimerCallback::get_callback() const { return callback_; }

} // namespace rix