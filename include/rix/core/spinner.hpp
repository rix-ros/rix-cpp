#pragma once

#include <atomic>
#include <csignal>
#include <memory>
#include <utility>

#include "rix/ipc/signal.hpp"

namespace rix {

class Spinner {
public:
  Spinner() = default;
  Spinner(const Spinner& other) = delete;
  Spinner& operator=(const Spinner& other) = delete;
  Spinner(Spinner&& other) = delete;
  Spinner& operator=(Spinner&& other) = delete;
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
    on_spin();
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

  static void set_shutdown_signal(std::shared_ptr<GenericSignal> signal) { shutdown_signal_ = std::move(signal); }
  static std::shared_ptr<GenericSignal> get_shutdown_signal() { return shutdown_signal_; }

private:
  std::atomic<bool> shutdown_flag_{false};
  static inline std::shared_ptr<GenericSignal> shutdown_signal_{create_signal(SIGINT)};
  static inline std::mutex mutex_{};
  static inline bool signal_received_{false};

  virtual void on_spin() = 0;
};

} // namespace rix