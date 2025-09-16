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

bool POSIXSocket::listen(int backlog) const {
  return ::listen(fd_, backlog) == 0;
}

std::unique_ptr<GenericSocket>
POSIXSocket::accept(Endpoint &remote_endpoint) const {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int sock_fd = ::accept(fd_, (struct sockaddr *)&addr, &len);
  if (sock_fd < 0) {
    return nullptr;
  }
  remote_endpoint.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, remote_endpoint.address.data(),
            INET_ADDRSTRLEN);
  remote_endpoint.port = ntohs(addr.sin_port);
  return std::unique_ptr<POSIXSocket>(new POSIXSocket(sock_fd));
}

bool POSIXSocket::connect(const Endpoint &endpoint) const {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  return ::connect(fd_, (struct sockaddr *)&addr, sizeof(addr)) == 0;
}

void POSIXSocket::close() const { ::close(fd_); }

ssize_t POSIXSocket::send(const void *buf, size_t len, int flags) const {
  return ::send(fd_, buf, len, flags);
}

ssize_t POSIXSocket::recv(void *buf, size_t len, int flags) const {
  return ::recv(fd_, buf, len, flags);
}

ssize_t POSIXSocket::send_to(const void *buf, size_t len,
                             const Endpoint &endpoint, int flags) const {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  return ::sendto(fd_, buf, len, flags, (struct sockaddr *)&addr, sizeof(addr));
}

ssize_t POSIXSocket::recv_from(void *buf, size_t len, Endpoint &endpoint,
                               int flags) const {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  ssize_t n;
  n = ::recvfrom(fd_, buf, len, flags, (struct sockaddr *)&addr, &addrlen);
  if (n < 0) {
    return n;
  }
  endpoint.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, endpoint.address.data(), INET_ADDRSTRLEN);
  endpoint.port = ntohs(addr.sin_port);
  return n;
}

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
  return ret > 0 && (pfd.revents & POLLHUP || pfd.revents & POLLERR ||
                     pfd.revents & POLLNVAL);
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

bool POSIXSocket::set_reuse_port(bool reuse) const {
  int optval = reuse ? 1 : 0;
  int status;
  status = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
  return status == 0;
}

bool POSIXSocket::get_reuse_port() const {
  int optval;
  socklen_t optlen = sizeof(optval);
  if (getsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, &optlen) < 0) {
    return false;
  }
  return optval != 0;
}

bool POSIXSocket::set_recv_buffer_size(int size) const {
  return setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == 0;
}

int POSIXSocket::get_recv_buffer_size() const {
  int size;
  socklen_t optlen = sizeof(size);
  if (getsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, &optlen) < 0) {
    return -1;
  }
  return size;
}

bool POSIXSocket::set_send_buffer_size(int size) const {
  return setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == 0;
}

int POSIXSocket::get_send_buffer_size() const {
  int size;
  socklen_t optlen = sizeof(size);
  if (getsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, &optlen) < 0) {
    return -1;
  }
  return size;
}

bool POSIXSocket::join_multicast_group(
    const std::string &multicast_address) const {
  ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_address.c_str());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  int status;
  status = setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  return status == 0;
}

bool POSIXSocket::leave_multicast_group(
    const std::string &multicast_address) const {
  ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_address.c_str());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  int status;
  status = setsockopt(fd_, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
  return status == 0;
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