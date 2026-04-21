#include "rix/ipc/tcp_stream.hpp"

#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

namespace rix {

// Constructor and Destructor
TCPStream::TCPStream(const Endpoint& endpoint, bool blocking) : fd_(::socket(AF_INET, SOCK_STREAM, 0)) {
  static bool sigpipe_flag = false;
  if (!sigpipe_flag) {
    // Ignore SIGPIPE signal to prevent process termination on socket write errors
    signal(SIGPIPE, SIG_IGN);
    sigpipe_flag = true;
  }

  if (blocking) {
    set_blocking(true);
  }

  struct sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(endpoint.port);
  inet_pton(AF_INET, endpoint.address.c_str(), &addr.sin_addr);
  ::connect(fd_, (struct sockaddr*)&addr, sizeof(addr));
}

TCPStream::~TCPStream() { ::close(fd_); }

// Socket control operations
bool TCPStream::set_blocking(bool blocking) const {
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

bool TCPStream::get_blocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }
  return (flags & O_NONBLOCK) == 0;
}

// Endpoint retrieval
Endpoint TCPStream::local_endpoint() const {
  struct sockaddr_in addr {};
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

Endpoint TCPStream::remote_endpoint() const {
  struct sockaddr_in addr {};
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

bool TCPStream::wait_readable(const Duration& timeout) const {
  // Implement with poll
  struct pollfd pfd {};
  pfd.fd = fd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLIN);
}

bool TCPStream::wait_writable(const Duration& timeout) const {
  struct pollfd pfd {};
  pfd.fd = fd_;
  pfd.events = POLLOUT;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLOUT);
}

bool TCPStream::wait_exception(const Duration& timeout) const {
  struct pollfd pfd {};
  pfd.fd = fd_;
  pfd.events = 0;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLHUP || pfd.revents & POLLERR || pfd.revents & POLLNVAL);
}

TCPStream::TCPStream(int fd) : fd_(fd) {}

// Low-level I/O operations to be implemented by derived classes
bool TCPStream::writev(const ConstMessageSegment* segments, size_t segment_count, ssize_t& ret) const {
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
      bytes_sent = result;
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
  ret = bytes_sent;
  return true;
}

bool TCPStream::readv(MessageSegment* segments, size_t segment_count, ssize_t& ret) const {
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
      bytes_received = result;
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
  ret = bytes_received;
  return true;
}

ssize_t TCPStream::send(const uint8_t* buf, size_t len, int flags) const {
  ssize_t bytes_sent = 0;
  ssize_t to_send = len;
  while (bytes_sent < to_send) {
    ssize_t ret = ::send(fd_, buf + bytes_sent, len - bytes_sent, flags);
    if (ret < 0)
      return -1;
    bytes_sent += ret;
  }
  return bytes_sent;
}

ssize_t TCPStream::recv(uint8_t* buf, size_t len, int flags) const {
  ssize_t bytes_read = 0;
  ssize_t to_read = len;
  while (bytes_read < to_read) {
    ssize_t ret = ::recv(fd_, buf + bytes_read, len - bytes_read, flags);
    if (ret < 0)
      return -1;
    bytes_read += ret;
  }
  return bytes_read;
}

} // namespace rix