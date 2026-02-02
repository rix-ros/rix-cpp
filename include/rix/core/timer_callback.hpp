#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"

namespace rix {

class Node; // Forward declaration

class TimerCallback final : public Spinner {
  friend class Node;

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

  // Disable copy and move semantics
  TimerCallback(const TimerCallback&) = delete;
  TimerCallback& operator=(const TimerCallback&) = delete;
  TimerCallback(TimerCallback&&) = delete;
  TimerCallback& operator=(TimerCallback&&) = delete;

  /**
   * @brief Destructor. Stops the timer.
   */
  ~TimerCallback() override;

  /**
   * @brief Sets the callback for this timer.
   * @param callback The callback function.
   */
  void set_callback(Callback callback);

  /**
   * @brief Returns the callback for this timer.
   * @return The callback function.
   */
  Callback get_callback() const;

private:
  Duration duration_;         ///< The duration between timer events.
  Event event_;               ///< The current timer event data.
  Callback callback_;         ///< The timer callback function.
  std::mutex callback_mutex_; ///< Mutex for protecting the callback.
  std::thread spin_thread_{}; ///< The thread running the spin loop.

  /**
   * @brief Constructs a TimerCallback with the given duration and callback function.
   */
  TimerCallback(const Duration& duration, Callback callback);

  // Disable public spin methods (only Node can spin the TimerCallback)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the TimerCallback.
   * @details Invokes the callback at the specified duration intervals.
   */
  void on_spin() override;
};

} // namespace rix