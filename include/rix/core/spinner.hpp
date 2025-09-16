#pragma once

#include "rix/ipc/generic_signal.hpp"
#include <memory>

namespace rix::core {

class Spinner {
public:
  Spinner() = default;
  Spinner(const Spinner &other) = default;
  Spinner &operator=(const Spinner &other) = default;
  virtual ~Spinner() = default;

  void spin(std::unique_ptr<rix::ipc::GenericSignal> signal) {
    while (ok()) {
      if (signal->is_ready()) {
        shutdown();
        break;
      }
      spin_once();
    }
  }

  void spin() {
    while (ok())
      spin_once();
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
  virtual bool ok() const = 0;

  /**
   * @brief Shuts down the object. ok() will return false after this call.
   *
   */
  virtual void shutdown() = 0;
};

} // namespace rix::core