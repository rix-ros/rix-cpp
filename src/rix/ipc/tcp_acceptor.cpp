#include "rix/ipc/tcp_acceptor.hpp"

#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

#include "rix/ipc/tcp_stream.hpp"

namespace rix {

TCPAcceptor::TCPAcceptor(const Endpoint& endpoint, int backlog) : fd_(::socket(AF_INET, SOCK_STREAM, 0)) {
  int optval = 1;
  int status = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (status < 0) {
    return;
  }

  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  status = ::bind(fd_, (struct sockaddr*)&addr, sizeof(addr));
  if (status < 0) {
    return;
  }

  ::listen(fd_, backlog);
}

TCPAcceptor::~TCPAcceptor() { ::close(fd_); }

// Socket state operations
std::shared_ptr<Stream> TCPAcceptor::accept(Endpoint& remote_endpoint) const {
  struct sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  int sock_fd = ::accept(fd_, (struct sockaddr*)&addr, &len);
  if (sock_fd < 0) {
    return nullptr;
  }
  remote_endpoint.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, &remote_endpoint.address[0], INET_ADDRSTRLEN);
  remote_endpoint.address.resize(strlen(remote_endpoint.address.c_str()));
  remote_endpoint.port = ntohs(addr.sin_port);
  return std::shared_ptr<TCPStream>(new TCPStream(sock_fd));
}

// Socket control operations
bool TCPAcceptor::set_blocking(bool blocking) const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }
  if (blocking) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }
  return fcntl(fd_, F_SETFL, flags) == 0;
}

bool TCPAcceptor::get_blocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }
  return (flags & O_NONBLOCK) == 0;
}

// Endpoint retrieval
Endpoint TCPAcceptor::local_endpoint() const {
  struct sockaddr_in addr{};
  socklen_t addrlen = sizeof(addr);
  if (getsockname(fd_, (struct sockaddr*)&addr, &addrlen) < 0) {
    return {};
  }
  Endpoint ep;
  ep.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, &ep.address[0], INET_ADDRSTRLEN);
  ep.address.resize(strlen(ep.address.c_str()));
  ep.port = ntohs(addr.sin_port);
  return ep;
}

Endpoint TCPAcceptor::remote_endpoint() const {
  struct sockaddr_in addr{};
  socklen_t addrlen = sizeof(addr);
  if (getpeername(fd_, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) < 0) {
    return {};
  }
  Endpoint ep;
  ep.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, &ep.address[0], INET_ADDRSTRLEN);
  ep.address.resize(strlen(ep.address.c_str()));
  ep.port = ntohs(addr.sin_port);
  return ep;
}

bool TCPAcceptor::wait_readable(const Duration& timeout) const {
  // Implement with poll
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLIN);
}

bool TCPAcceptor::wait_writable(const Duration& timeout) const {
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = POLLOUT;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLOUT);
}

bool TCPAcceptor::wait_exception(const Duration& timeout) const {
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = 0;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLHUP || pfd.revents & POLLERR || pfd.revents & POLLNVAL);
}

} // namespace rix