#pragma once

#include <memory>

#include "rix/ipc/generic_signal.hpp"
#include "rix/ipc/posix_signal.hpp"

namespace rix {
using Signal = POSIXSignal;

static inline std::unique_ptr<GenericSignal> create_signal(int signum) { return std::make_unique<Signal>(signum); }

} // namespace rix