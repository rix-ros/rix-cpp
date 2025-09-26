#pragma once

#include "rix/ipc/generic_socket.hpp"
#include <memory>

#include "rix/ipc/posix_socket.hpp"
#include "rix/ipc/poll.hpp"

namespace rix::ipc {

using Socket = rix::ipc::POSIXSocket;
static inline std::shared_ptr<GenericSocket> create_socket() { return std::make_shared<Socket>(); }

} // namespace rix::ipc