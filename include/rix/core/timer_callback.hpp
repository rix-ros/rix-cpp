#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"

namespace rix {

class TimerCallback : public Spinner {
public:
  struct Event {
    Time last_expected{};
    Time last_real{};
    Time current_expected{};
    Time current_real{};
    Duration last_duration{};
  };

  using Callback = std::function<void(const Event& event)>;
  template <typename TObj> using ObjCallback = std::function<void(TObj*, const Event& event)>;

  TimerCallback(const Duration& duration, Callback callback);
  TimerCallback(const TimerCallback&) = delete;
  TimerCallback& operator=(const TimerCallback&) = delete;
  TimerCallback(TimerCallback&&) = delete;
  TimerCallback& operator=(TimerCallback&&) = delete;
  ~TimerCallback();

  /**
   * @brief Set the callback for the timer.
   *
   * @param callback The callback function to be invoked.
   *
   */
  void set_callback(Callback callback);

  /**
   * @brief Returns the callback for this timer.
   *
   * @return Callback
   */
  Callback get_callback() const;

private:
  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;

  Duration duration_;
  Event event_;
  Callback callback_;
  std::mutex callback_mutex_;

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_{};
#endif
};

} // namespace rix