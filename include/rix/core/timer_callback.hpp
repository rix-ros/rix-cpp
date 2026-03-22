#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"

namespace rix {

class TimerCallback : public Spinner {
public:
  /**
   * @brief Event structure passed to the timer callback.
   */
  struct Event {
    Time last_expected{};     ///< The last expected time point.
    Time last_real{};         ///< The last real time point.
    Time current_expected{};  ///< The current expected time point.
    Time current_real{};      ///< The current real time point.
    Duration last_duration{}; ///< The duration of the last interval.
  };

  /**
   * @brief Callback type definition for timer events.
   */
  using Callback = std::function<void(const Event& event)>;

  virtual void set_callback(Callback callback) = 0;
  virtual Callback get_callback() const = 0;
};

namespace detail {

class TimerCallbackImpl final : public TimerCallback {
public:
  /**
   * @brief Constructs a TimerCallbackImpl with the given duration and callback function.
   */
  TimerCallbackImpl(const Duration& duration, Callback callback);

  // Disable copy and move semantics
  TimerCallbackImpl(const TimerCallbackImpl&) = delete;
  TimerCallbackImpl& operator=(const TimerCallbackImpl&) = delete;
  TimerCallbackImpl(TimerCallbackImpl&&) = delete;
  TimerCallbackImpl& operator=(TimerCallbackImpl&&) = delete;

  /**
   * @brief Destructor. Stops the timer.
   */
  ~TimerCallbackImpl() override;

  /**
   * @brief Sets the callback for this timer.
   * @param callback The callback function.
   */
  void set_callback(Callback callback) override;

  /**
   * @brief Returns the callback for this timer.
   * @return The callback function.
   */
  Callback get_callback() const override;

private:
  Duration duration_;         ///< The duration between timer events.
  Event event_;               ///< The current timer event data.
  Callback callback_;         ///< The timer callback function.
  std::mutex callback_mutex_; ///< Mutex for protecting the callback.
  std::thread spin_thread_{}; ///< The thread running the spin loop.

  // Disable public spin methods (only Node can spin the TimerCallbackImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the TimerCallbackImpl.
   * @details Invokes the callback at the specified duration intervals.
   */
  void on_spin() override;
};

} // namespace detail
} // namespace rix