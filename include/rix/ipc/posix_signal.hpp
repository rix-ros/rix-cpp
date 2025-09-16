#pragma once

#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include <functional>

#include "rix/ipc/generic_signal.hpp"

namespace rix::ipc {

class POSIXSignal : public GenericSignal {
public:
  POSIXSignal(int signum);
  virtual ~POSIXSignal();

  virtual bool ignore() const override;
  virtual bool raise() const override;
  virtual bool wait(const rix::util::Duration &d) const override;

private:
  struct Notifier {
    Notifier() : is_init(false) {};
    std::array<int, 2> pipe; /**< 0: read end, 1: write end */
    bool is_init;            /**< false if Notifier has not been initialized */
  };
  static std::array<Notifier, 32> notifier;
  static void handler(int signum);

  int signum_;
};

} // namespace rix::ipc