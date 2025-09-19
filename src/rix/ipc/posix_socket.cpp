#include "rix/ipc/posix_socket.hpp"

namespace rix::ipc {

POSIXSocket::POSIXSocket() : fd_(::socket(AF_INET, SOCK_STREAM, 0)) {}

POSIXSocket::POSIXSocket(int fd) : fd_(fd) {}

POSIXSocket::~POSIXSocket() { close(); }

bool POSIXSocket::bind(const Endpoint &endpoint) const {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  return ::bind(fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

bool POSIXSocket::listen(int backlog) const { return ::listen(fd_, backlog) == 0; }

std::shared_ptr<GenericSocket> POSIXSocket::accept(Endpoint &remote_endpoint) const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int sock_fd = ::accept(fd_, (struct sockaddr *)&addr, &len);
  if (sock_fd < 0) {
    return nullptr;
  }
  remote_endpoint.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, remote_endpoint.address.data(), INET_ADDRSTRLEN);
  remote_endpoint.port = ntohs(addr.sin_port);
  return std::shared_ptr<POSIXSocket>(new POSIXSocket(sock_fd));
}

bool POSIXSocket::connect(const Endpoint &endpoint) const {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  return ::connect(fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

void POSIXSocket::close() const { ::close(fd_); }

ssize_t POSIXSocket::send(const void *buf, size_t len, int flags) const { return ::send(fd_, buf, len, flags); }

ssize_t POSIXSocket::recv(void *buf, size_t len, int flags) const { return ::recv(fd_, buf, len, flags); }

bool POSIXSocket::wait_readable(const rix::util::Duration &timeout) const {
  // Implement with poll
  struct pollfd pfd;
  pfd.fd = fd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLIN);
}

bool POSIXSocket::wait_writable(const rix::util::Duration &timeout) const {
  struct pollfd pfd;
  pfd.fd = fd_;
  pfd.events = POLLOUT;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLOUT);
}

bool POSIXSocket::wait_exception(const rix::util::Duration &timeout) const {
  struct pollfd pfd;
  pfd.fd = fd_;
  pfd.events = 0;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLHUP || pfd.revents & POLLERR || pfd.revents & POLLNVAL);
}

bool POSIXSocket::set_blocking(bool blocking) const {
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

bool POSIXSocket::get_blocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }
  return (flags & O_NONBLOCK) == 0;
}

bool POSIXSocket::set_reuse_address(bool reuse) const {
  int optval = reuse ? 1 : 0;
  int status;
  status = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  return status == 0;
}

bool POSIXSocket::get_reuse_address() const {
  int optval;
  socklen_t optlen = sizeof(optval);
  if (getsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen) < 0) {
    return false;
  }
  return optval != 0;
}

Endpoint POSIXSocket::local_endpoint() const {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  if (getsockname(fd_, (struct sockaddr *)&addr, &addrlen) < 0) {
    return Endpoint();
  }
  Endpoint ep;
  ep.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, ep.address.data(), INET_ADDRSTRLEN);
  ep.port = ntohs(addr.sin_port);
  return ep;
}

Endpoint POSIXSocket::remote_endpoint() const {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  if (getpeername(fd_, (struct sockaddr *)&addr, &addrlen) < 0) {
    return Endpoint();
  }
  Endpoint ep;
  ep.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, ep.address.data(), INET_ADDRSTRLEN);
  ep.port = ntohs(addr.sin_port);
  return ep;
}

} // namespace rix::ipc