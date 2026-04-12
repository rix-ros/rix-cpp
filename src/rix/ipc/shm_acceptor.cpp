#include "rix/ipc/shm_acceptor.hpp"
#include "rix/ipc/shm_stream.hpp"

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

ShmAcceptor::ShmAcceptor(const Endpoint& endpoint, int backlog = 64)
    : shm_path_("/rix_shm_" + std::to_string(endpoint.port)), shm_buffer_size_(1024 * 1024),
      posix_fd_(::socket(AF_UNIX, SOCK_STREAM, 0)) {
  int optval = 1;

  if (posix_fd_ < 0) {
    cout << "Failed to create socket: " << strerror(errno) << endl;
    return;
  }

  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, shm_path_.c_str(), sizeof(addr.sun_path) - 1);
  int status = ::bind(posix_fd_, (struct sockaddr*)&addr, sizeof(addr));
  if (status < 0) {
    cout << "Failed to bind socket: " << strerror(errno) << endl;
    ::close(posix_fd_);
    return;
  }

  status = ::listen(posix_fd_, backlog);
  if (status < 0) {
    cout << "Failed to listen on socket: " << strerror(errno) << endl;
    ::close(posix_fd_);
    return;
  }
  local_endpoint_ = endpoint;
}

ShmAcceptor::~ShmAcceptor() { ::close(fd_); }

// Socket state operations
std::shared_ptr<Stream> ShmAcceptor::accept(Endpoint& remote_endpoint) const {

  std::lock_guard<std::mutex> lock(accept_mutex_);

  // Accept the incoming connection
  struct sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  int sock_fd = ::accept(posix_fd_, (struct sockaddr*)&addr, &len);
  if (sock_fd < 0) {
    return nullptr;
  }

  // Create a unique shared memory name based on the socket file descriptor
  std::string shm_name = "/rix_shm_" + std::to_string(sock_fd);
  int shm_fd = shm_open(shm_name.c_str(), O_RDWR | O_CREAT, 0666);
  if (shm_fd < 0) {
    cout << "Failed to create shared memory: " << strerror(errno) << endl;
    ::close(sock_fd);
    return nullptr;
  }

  // Set the size of the shared memory
  if (ftruncate(shm_fd, shm_buffer_size_) < 0) {
    cout << "Failed to set shared memory size: " << strerror(errno) << endl;
    ::close(sock_fd);
    ::close(shm_fd);
    // On failure, unlink the shared memory to clean up
    shm_unlink(shm_name.c_str());
    return nullptr;
  }

  // Map shared memory
  void* shm_addr = mmap(nullptr, shm_buffer_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_addr == MAP_FAILED) {
    cout << "Failed to map shared memory: " << strerror(errno) << endl;
    ::close(sock_fd);
    ::close(shm_fd);
    shm_unlink(shm_name.c_str());
    return nullptr;
  }

  return std::shared_ptr<TCPStream>(new TCPStream(sock_fd));
}

// Socket control operations
bool ShmAcceptor::set_blocking(bool blocking) const {
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

bool ShmAcceptor::get_blocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }
  return (flags & O_NONBLOCK) == 0;
}

// Endpoint retrieval
Endpoint ShmAcceptor::local_endpoint() const {
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

Endpoint ShmAcceptor::remote_endpoint() const {
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

bool ShmAcceptor::wait_readable(const Duration& timeout) const {
  // Implement with poll
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLIN);
}

bool ShmAcceptor::wait_writable(const Duration& timeout) const {
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = POLLOUT;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLOUT);
}

bool ShmAcceptor::wait_exception(const Duration& timeout) const {
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = 0;

  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLHUP || pfd.revents & POLLERR || pfd.revents & POLLNVAL);
}

} // namespace rix