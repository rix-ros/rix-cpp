#pragma once

#include "rix/ipc/signal.hpp"
#include <memory>

namespace rix {

class Spinner {
public:
  Spinner() = default;
  Spinner(const Spinner &other) = default;
  Spinner &operator=(const Spinner &other) = default;
  Spinner(Spinner &&other) = default;
  Spinner &operator=(Spinner &&other) = default;
  virtual ~Spinner() = default;

  void spin() {
    while (ok()) {
      spin_once();
      if (signal_received_) {
        shutdown();
      } else {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_signal_ && shutdown_signal_->is_ready()) {
          signal_received_ = true;
          shutdown();
        }
      }
    }
  }

  /**
   * @brief Returns true if loop should continue, false if loop should stop.
   *
   */
  virtual void spin_once() = 0;

  /**
   * @brief Returns true if shutdown has not been called and the constructor
   * created the object without error.
   *
   */
  bool ok() const { return !shutdown_flag_; }

  /**
   * @brief Shuts down the object. ok() will return false after this call.
   *
   */
  void shutdown() { shutdown_flag_ = true; }

  static void set_shutdown_signal(std::shared_ptr<GenericSignal> signal) { shutdown_signal_ = signal; }

private:
  bool shutdown_flag_{false};
  static inline std::shared_ptr<GenericSignal> shutdown_signal_{create_signal(SIGINT)};
  static inline std::mutex mutex_{};
  static inline bool signal_received_{false};
};

} // namespace rix