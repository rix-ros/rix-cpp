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
  virtual ~Spinner() = default;

  // Disable copy and move semantics
  Spinner(const Spinner& other) = delete;
  Spinner& operator=(const Spinner& other) = delete;
  Spinner(Spinner&& other) = delete;
  Spinner& operator=(Spinner&& other) = delete;

  /**
   * @brief Spins the object until shutdown is called or a shutdown signal is received.
   *
   */
  void spin() {
    while (ok()) {
      spin_once();
    }
  }

  /**
   * @brief Spins the object once. Checks for shutdown signal after spinning.
   *
   */
  void spin_once() {
    on_spin();
    if (signal_received_) {
      shutdown();
      return;
    }
    if (mutex_.try_lock()) {
      // TODO: Move signal checking to a separate thread.
      if (shutdown_signal_ && shutdown_signal_->is_ready()) {
        signal_received_ = true;
        shutdown();
      }
      mutex_.unlock();
    }
  }

  /**
   * @brief Returns true if the object is not shut down.
   *
   */
  bool ok() const noexcept { return !shutdown_flag_; }

  /**
   * @brief Shuts down the object. ok() will return false after this call.
   *
   */
  void shutdown() noexcept { shutdown_flag_ = true; }

  /**
   * @brief Sets the global shutdown signal used by all Spinners.
   * @param signal The shutdown signal.
   */
  static void set_shutdown_signal(std::shared_ptr<GenericSignal> signal) { shutdown_signal_ = std::move(signal); }

  /**
   * @brief Gets the global shutdown signal used by all Spinners.
   * @return The shutdown signal.
   */
  static std::shared_ptr<GenericSignal> get_shutdown_signal() { return shutdown_signal_; }

  static bool signal_received() { return signal_received_; }
  static void reset_signal_received() { signal_received_ = false; }

protected:
  /**
   * @brief Internal spin implementation for the Spinner.
   */
  virtual void on_spin() = 0;

private:
  std::atomic<bool> shutdown_flag_{false}; ///< Flag indicating if the spinner is shut down.
  static inline std::shared_ptr<GenericSignal> shutdown_signal_{create_signal(SIGINT)}; ///< The global shutdown signal.
  static inline std::mutex mutex_{};          ///< Mutex for protecting the shutdown signal.
  static inline bool signal_received_{false}; ///< Flag indicating if a shutdown signal has been received.
};

} // namespace rix