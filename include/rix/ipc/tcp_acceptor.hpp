#pragma once

#include "rix/ipc/acceptor.hpp"
#include "rix/ipc/stream.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

namespace rix {

class TCPAcceptor final : public Acceptor {
public:
  // Constructor and Destructor
  TCPAcceptor(const Endpoint& endpoint, int backlog = 64);
  ~TCPAcceptor();

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
  int fd_;
};

} // namespace rix