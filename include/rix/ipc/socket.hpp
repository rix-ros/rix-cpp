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

static inline std::unique_ptr<GenericSocket> create_socket() {
  return std::make_unique<Socket>();
}

} // namespace rix::ipc