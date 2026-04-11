#pragma once

#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

#include "rix/ipc/acceptor.hpp"
#include "rix/ipc/stream.hpp"

namespace rix {

class ShmAcceptor final : public Acceptor {
public:
  // Constructor and Destructor
  ShmAcceptor(const Endpoint& endpoint, int backlog = 64);
  ~ShmAcceptor();
  // Socket state operations
  std::shared_ptr<Stream> accept(Endpoint& remote_endpoint) const override;

  // Socket control operations
  bool set_blocking(bool blocking) const override;
  bool get_blocking() const override;

  // Endpoint retrieval
  Endpoint local_endpoint() const override;
  Endpoint remote_endpoint() const override;

  bool wait_readable(const Duration& timeout) const override;
  bool wait_writable(const Duration& timeout) const override;
  bool wait_exception(const Duration& timeout) const override;

private:
  std::string shm_path_;
  int posix_fd_;
  size_t shm_buffer_size_;
  // Need mutex to support concurrency in shm
  std::mutex accept_mutex_;
};

} // namespace rix