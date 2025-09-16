#pragma once

#include "rix/ipc/generic_signal.hpp"
#include <memory>

#ifdef _WIN32

#include "rix/ipc/windows_signal.hpp"

namespace rix::ipc {
using Signal = rix::ipc::WindowsSignal;

#else

#include "rix/ipc/posix_signal.hpp"

namespace rix::ipc {
using Signal = rix::ipc::POSIXSignal;

#endif

static inline std::unique_ptr<GenericSignal> create_signal(int signum) {
  return std::make_unique<Signal>(signum);
}

} // namespace rix::ipc