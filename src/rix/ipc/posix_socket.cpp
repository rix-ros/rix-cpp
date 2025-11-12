#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/un.h>

#include "rix/ipc/posix_socket.hpp"

namespace rix {

POSIXSocket::POSIXSocket() : fd_(::socket(AF_INET, SOCK_STREAM, 0)) {
  static bool sigpipe_flag = false;
  if (!sigpipe_flag) {
    // Ignore SIGPIPE signal to prevent process termination on socket write errors
    signal(SIGPIPE, SIG_IGN);
    sigpipe_flag = true;
  }
}

POSIXSocket::POSIXSocket(int fd) : fd_(fd) {}

POSIXSocket::~POSIXSocket() { close(); }

bool POSIXSocket::bind(const Endpoint& endpoint) const {
  struct sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  return ::bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

bool POSIXSocket::listen(int backlog) const { return ::listen(fd_, backlog) == 0; }

std::shared_ptr<GenericSocket> POSIXSocket::accept(Endpoint& remote_endpoint) const {
  struct sockaddr_in addr {};
  socklen_t len = sizeof(addr);
  int sock_fd = ::accept(fd_, (struct sockaddr*)&addr, &len);
  if (sock_fd < 0) {
    return nullptr;
  }
  remote_endpoint.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, remote_endpoint.address.data(), INET_ADDRSTRLEN);
  remote_endpoint.port = ntohs(addr.sin_port);
  return std::shared_ptr<POSIXSocket>(new POSIXSocket(sock_fd));
}

bool POSIXSocket::connect(const Endpoint& endpoint) const {
  struct sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  return ::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

void POSIXSocket::close() const { ::close(fd_); }

ssize_t POSIXSocket::writev(const ConstMessageSegment* segments, size_t segment_count) const {
  struct iovec* iov = new struct iovec[segment_count];
  ssize_t to_send = 0;
  for (int i = 0; i < segment_count; ++i) {
    iov[i].iov_base = const_cast<uint8_t*>(segments[i].ptr());
    iov[i].iov_len = segments[i].len();
    to_send += segments[i].len();
  }
  ssize_t bytes_sent = 0;
  while (bytes_sent < to_send) {
    ssize_t result = ::writev(fd_, iov, segment_count);
    if (result <= 0) {
      break;
    }
    bytes_sent += result;

    // Adjust iov to account for bytes already sent
    ssize_t offset = result;
    for (int i = 0; i < segment_count; ++i) {
      if (offset >= static_cast<ssize_t>(iov[i].iov_len)) {
        offset -= iov[i].iov_len;
        iov[i].iov_base = static_cast<uint8_t*>(iov[i].iov_base) + iov[i].iov_len;
        iov[i].iov_len = 0;
      } else {
        iov[i].iov_base = static_cast<uint8_t*>(iov[i].iov_base) + offset;
        iov[i].iov_len -= offset;
        break;
      }
    }
  }
  delete[] iov;
  return bytes_sent;
}

ssize_t POSIXSocket::readv(MessageSegment* segments, size_t segment_count) const {
  struct iovec* iov = new struct iovec[segment_count];
  ssize_t to_read = 0;
  for (int i = 0; i < segment_count; ++i) {
    iov[i].iov_base = segments[i].ptr();
    iov[i].iov_len = segments[i].len();
    to_read += segments[i].len();
  }
  ssize_t bytes_received = 0;
  while (bytes_received < to_read) {
    ssize_t result = ::readv(fd_, iov, segment_count);
    if (result <= 0) {
      break;
    }
    bytes_received += result;

    // Adjust iov to account for bytes already received
    ssize_t offset = result;
    for (int i = 0; i < segment_count; ++i) {
      if (offset >= static_cast<ssize_t>(iov[i].iov_len)) {
        offset -= iov[i].iov_len;
        iov[i].iov_base = static_cast<uint8_t*>(iov[i].iov_base) + iov[i].iov_len;
        iov[i].iov_len = 0;
      } else {
        iov[i].iov_base = static_cast<uint8_t*>(iov[i].iov_base) + offset;
        iov[i].iov_len -= offset;
        break;
      }
    }
  }
  delete[] iov;
  return bytes_received;
}

ssize_t POSIXSocket::send(const void* buf, size_t len, int flags) const { return ::send(fd_, buf, len, flags); }

ssize_t POSIXSocket::recv(void* buf, size_t len, int flags) const { return ::recv(fd_, buf, len, flags); }

bool POSIXSocket::wait_readable(const Duration& timeout) const {
  // Implement with poll
  struct pollfd pfd {};
  pfd.fd = fd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLIN);
}

bool POSIXSocket::wait_writable(const Duration& timeout) const {
  struct pollfd pfd {};
  pfd.fd = fd_;
  pfd.events = POLLOUT;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLOUT);
}

bool POSIXSocket::wait_exception(const Duration& timeout) const {
  struct pollfd pfd {};
  pfd.fd = fd_;
  pfd.events = 0;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
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
  struct sockaddr_in addr {};
  socklen_t addrlen = sizeof(addr);
  if (getsockname(fd_, (struct sockaddr*)&addr, &addrlen) < 0) {
    return {};
  }
  Endpoint ep;
  ep.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, ep.address.data(), INET_ADDRSTRLEN);
  ep.address.resize(strlen(ep.address.c_str()));
  ep.port = ntohs(addr.sin_port);
  return ep;
}

Endpoint POSIXSocket::remote_endpoint() const {
  struct sockaddr_in addr {};
  socklen_t addrlen = sizeof(addr);
  if (getpeername(fd_, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) < 0) {
    return {};
  }
  Endpoint ep;
  ep.address.resize(INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &addr.sin_addr, ep.address.data(), INET_ADDRSTRLEN);
  ep.address.resize(strlen(ep.address.c_str()));
  ep.port = ntohs(addr.sin_port);
  return ep;
}

int POSIXSocket::get_fd() const { return fd_; }

} // namespace rix