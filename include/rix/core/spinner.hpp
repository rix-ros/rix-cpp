#pragma once

#include "rix/ipc/signal.hpp"
#include <memory>

namespace rix {

class Spinner {
public:
  Spinner() = default;
  Spinner(const Spinner& other) = default;
  Spinner& operator=(const Spinner& other) = default;
  Spinner(Spinner&& other) = default;
  Spinner& operator=(Spinner&& other) = default;
  virtual ~Spinner() = default;

  void spin() {
    while (ok()) {
      spin_once();
    }
  }

  /**
   * @brief Returns true if loop should continue, false if loop should stop.
   *
   */
  void spin_once() {
    spin_function();
    if (signal_received_) {
      shutdown();
      return;
    }
    if (mutex_.try_lock()) {
      if (shutdown_signal_ && shutdown_signal_->is_ready()) {
        signal_received_ = true;
        shutdown();
      }
      mutex_.unlock();
      return;
    }
  }

  /**
   * @brief Returns true if shutdown has not been called and the constructor
   * created the object without error.
   *
   */
  bool ok() const noexcept { return !shutdown_flag_; }

  /**
   * @brief Shuts down the object. ok() will return false after this call.
   *
   */
  void shutdown() noexcept { shutdown_flag_ = true; }

  static void set_shutdown_signal(std::shared_ptr<GenericSignal> signal) { shutdown_signal_ = signal; }
  static std::shared_ptr<GenericSignal> get_shutdown_signal() { return shutdown_signal_; }

private:
  bool shutdown_flag_{false};
  static inline std::shared_ptr<GenericSignal> shutdown_signal_{create_signal(SIGINT)};
  static inline std::mutex mutex_{};
  static inline bool signal_received_{false};

  virtual void spin_function() = 0;
};

} // namespace rix