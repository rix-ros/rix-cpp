#pragma once

#include "rix/ipc/generic_socket.hpp"
#include <memory>

#ifdef _WIN32

#include "rix/ipc/windows_socket.hpp"

namespace rix::ipc {
using Socket = rix::ipc::WindowsSocket;

#else

#include "rix/ipc/posix_socket.hpp"

namespace rix::ipc {
using Socket = rix::ipc::POSIXSocket;

#endif

static inline std::shared_ptr<GenericSocket> create_socket() { return std::make_shared<Socket>(); }

} // namespace rix::ipc