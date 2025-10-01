#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"

namespace rix::core {

class Timer : public Spinner {
public:
  struct Event {
    rix::util::Time last_expected{};
    rix::util::Time last_real{};
    rix::util::Time current_expected{};
    rix::util::Time current_real{};
    rix::util::Duration last_duration{};
  };

  using Callback = std::function<void(const Event &event)>;

  Timer(const rix::util::Duration &duration, Callback callback);
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;
  Timer(Timer &&) = delete;
  Timer &operator=(Timer &&) = delete;
  ~Timer();

  /**
   * @brief A single iteration of the timer loop. The callback will only
   * be called if the timer duration has passed since its last calling.
   *
   */
  void spin_once() override;

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
  rix::util::Duration duration_;
  Event event_;
  Callback callback_;
  std::mutex callback_mutex_;

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_;
#endif
};

} // namespace rix::core