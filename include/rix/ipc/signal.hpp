#pragma once

#include "rix/ipc/generic_signal.hpp"
#include <memory>

#ifdef _WIN32

#include "rix/ipc/windows_signal.hpp"

namespace rix {
using Signal = WindowsSignal;

#else

#include "rix/ipc/posix_signal.hpp"

namespace rix {
using Signal = POSIXSignal;

#endif

static inline std::unique_ptr<GenericSignal> create_signal(int signum) {
  return std::make_unique<Signal>(signum);
}

} // namespace rix