#pragma once

#include "rix/ipc/generic_socket.hpp"
#include "rix/ipc/posix_socket.hpp"
#include <memory>

namespace rix {

using Socket = POSIXSocket;
static inline std::shared_ptr<GenericSocket> create_socket() { return std::make_shared<Socket>(); }

} // namespace rix