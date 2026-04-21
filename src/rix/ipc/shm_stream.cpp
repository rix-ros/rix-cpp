#include "rix/ipc/shm_stream.hpp"

#include <atomic>
#include <cstring>
#include <iostream>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace rix {

// ---------------------------------------------------------------------------
// Shared memory layout
//
// The region is split into two equal halves, each a one-directional ring buffer:
//
//   [0 .. half)         server->client channel  (server writes, client reads)
//   [half .. shm_size)  client->server channel  (client writes, server reads)
//
// Each half begins with a ShmHalfHeader followed by the ring data.
//
// Two eventfds are passed from server to client via SCM_RIGHTS:
//   s2c_efd: server writes data  -> server increments -> client polls to wake up
//   c2s_efd: client writes data  -> client increments -> server polls to wake up
//
// So:
//   Server send: writes into [0..half), then increments s2c_efd
//   Server recv: polls s2c_efd ... wait no.
//
// Corrected assignment:
//   data_efd_  = the eventfd the PEER signals when it has written data for US
//   space_efd_ = the eventfd the PEER signals when it has consumed our data
//
//   Server:
//     data_efd_  = c2s_efd  (client signals server when client wrote data)
//     space_efd_ = s2c_efd  (client signals server when client consumed server's data)
//   Client:
//     data_efd_  = s2c_efd  (server signals client when server wrote data)
//     space_efd_ = c2s_efd  (server signals client when server consumed client's data)
// ---------------------------------------------------------------------------

struct ShmHalfHeader {
  std::atomic<uint64_t> write_ptr;
  std::atomic<uint64_t> read_ptr;
};

static constexpr size_t HEADER_SIZE = sizeof(ShmHalfHeader);

// Server writes to first half, reads from second.
// Client writes to second half, reads from first.
static inline ShmHalfHeader* write_half(void* base, size_t shm_size, bool is_server) {
  return is_server ? reinterpret_cast<ShmHalfHeader*>(base)
                   : reinterpret_cast<ShmHalfHeader*>(static_cast<uint8_t*>(base) + shm_size / 2);
}

static inline ShmHalfHeader* read_half(void* base, size_t shm_size, bool is_server) {
  return is_server ? reinterpret_cast<ShmHalfHeader*>(static_cast<uint8_t*>(base) + shm_size / 2)
                   : reinterpret_cast<ShmHalfHeader*>(base);
}

static inline uint8_t* data_ptr(ShmHalfHeader* hdr) { return reinterpret_cast<uint8_t*>(hdr) + HEADER_SIZE; }

// ---------------------------------------------------------------------------
// SCM_RIGHTS helpers - pass/receive file descriptors over a Unix socket
// ---------------------------------------------------------------------------

static bool send_fds(int sock, const int* fds, int count) {
  char buf[1] = {0};
  struct iovec iov{buf, 1};
  size_t cmsg_size = CMSG_SPACE(count * sizeof(int));
  std::vector<char> cmsg_buf(cmsg_size, 0);

  struct msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsg_buf.data();
  msg.msg_controllen = cmsg_size;

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(count * sizeof(int));
  memcpy(CMSG_DATA(cmsg), fds, count * sizeof(int));

  return sendmsg(sock, &msg, 0) >= 0;
}

static bool recv_fds(int sock, int* fds, int count) {
  char buf[1];
  struct iovec iov{buf, 1};
  size_t cmsg_size = CMSG_SPACE(count * sizeof(int));
  std::vector<char> cmsg_buf(cmsg_size, 0);

  struct msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsg_buf.data();
  msg.msg_controllen = cmsg_size;

  if (recvmsg(sock, &msg, 0) < 0)
    return false;

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS)
    return false;
  memcpy(fds, CMSG_DATA(cmsg), count * sizeof(int));
  return true;
}

// ---------------------------------------------------------------------------
// Server-side constructor (called by ShmAcceptor::accept)
// ---------------------------------------------------------------------------

ShmStream::ShmStream(int fd,
                     void* shm_addr,
                     size_t shm_size,
                     const std::string& shm_name,
                     bool is_server,
                     const Endpoint& local_ep,
                     const Endpoint& remote_ep)
    : fd_(fd), shm_addr_(shm_addr), shm_size_(shm_size), shm_name_(shm_name), is_server_(is_server),
      local_ep_(local_ep), remote_ep_(remote_ep), data_rfd_(-1), signal_wfd_(-1) {

  memset(shm_addr_, 0, shm_size_);

  // Two pipes: s2c (server writes, client reads) and c2s (client writes, server reads).
  // Server keeps: data_rfd_ = c2s[0], signal_wfd_ = s2c[1]
  // Client keeps: data_rfd_ = s2c[0], signal_wfd_ = c2s[1]
  int s2c[2], c2s[2];
  if (::pipe(s2c) < 0 || ::pipe(c2s) < 0) {
    std::cout << "ShmStream: failed to create pipes: " << strerror(errno) << std::endl;
    return;
  }
  // Set non-blocking on the ends this side uses
  fcntl(c2s[0], F_SETFL, O_NONBLOCK); // data_rfd_
  fcntl(s2c[1], F_SETFL, O_NONBLOCK); // signal_wfd_

  data_rfd_ = c2s[0];
  signal_wfd_ = s2c[1];

  // Send all 4 pipe ends to client; client closes the two it doesn't need
  int fds[4] = {s2c[0], s2c[1], c2s[0], c2s[1]};
  if (!send_fds(fd_, fds, 4)) {
    std::cout << "ShmStream: failed to send pipe fds: " << strerror(errno) << std::endl;
  }
  // Close the ends the server doesn't use
  ::close(s2c[0]);
  ::close(c2s[1]);
}

// ---------------------------------------------------------------------------
// Client-side constructor
// ---------------------------------------------------------------------------

ShmStream::ShmStream(const Endpoint& endpoint, bool blocking)
    : fd_(::socket(AF_UNIX, SOCK_STREAM, 0)), shm_addr_(nullptr), shm_size_(0), is_server_(false), data_rfd_(-1),
      signal_wfd_(-1) {

  if (fd_ < 0) {
    std::cout << "ShmStream: failed to create socket: " << strerror(errno) << std::endl;
    return;
  }

  std::string path = "/tmp/rix_shm_" + std::to_string(endpoint.port);
  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::cout << "ShmStream: failed to connect: " << strerror(errno) << std::endl;
    ::close(fd_);
    fd_ = -1;
    return;
  }

  uint32_t name_len = 0;
  if (::recv(fd_, &name_len, sizeof(name_len), MSG_WAITALL) != sizeof(name_len)) {
    std::cout << "ShmStream: failed to receive shm name length" << std::endl;
    ::close(fd_);
    fd_ = -1;
    return;
  }

  shm_name_.resize(name_len);
  if (::recv(fd_, &shm_name_[0], name_len, MSG_WAITALL) != static_cast<ssize_t>(name_len)) {
    std::cout << "ShmStream: failed to receive shm name" << std::endl;
    ::close(fd_);
    fd_ = -1;
    return;
  }

  int shm_fd = shm_open(shm_name_.c_str(), O_RDWR, 0666);
  if (shm_fd < 0) {
    std::cout << "ShmStream: failed to open shm " << shm_name_ << ": " << strerror(errno) << std::endl;
    ::close(fd_);
    fd_ = -1;
    return;
  }

  shm_size_ = lseek(shm_fd, 0, SEEK_END);
  shm_addr_ = mmap(nullptr, shm_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  ::close(shm_fd);

  if (shm_addr_ == MAP_FAILED) {
    std::cout << "ShmStream: failed to map shm: " << strerror(errno) << std::endl;
    ::close(fd_);
    fd_ = -1;
    shm_addr_ = nullptr;
    return;
  }

  // Receive [s2c[0], s2c[1], c2s[0], c2s[1]] from server
  // Client keeps: data_rfd_ = s2c[0], signal_wfd_ = c2s[1]
  int fds[4];
  if (!recv_fds(fd_, fds, 4)) {
    std::cout << "ShmStream: failed to receive pipe fds" << std::endl;
    munmap(shm_addr_, shm_size_);
    ::close(fd_);
    fd_ = -1;
    shm_addr_ = nullptr;
    return;
  }
  data_rfd_ = fds[0];   // s2c[0]: read end, poll for data from server
  signal_wfd_ = fds[3]; // c2s[1]: write end, signal server
  // Close the ends the client doesn't use
  ::close(fds[1]); // s2c[1]: server's write end
  ::close(fds[2]); // c2s[0]: server's read end

  fcntl(data_rfd_, F_SETFL, O_NONBLOCK);
  fcntl(signal_wfd_, F_SETFL, O_NONBLOCK);

  local_ep_ = endpoint;
  remote_ep_ = endpoint;

  if (!blocking) {
    set_blocking(false);
  }
}

ShmStream::~ShmStream() {
  if (data_rfd_ >= 0)
    ::close(data_rfd_);
  if (signal_wfd_ >= 0)
    ::close(signal_wfd_);
  if (shm_addr_ && shm_addr_ != MAP_FAILED) {
    munmap(shm_addr_, shm_size_);
  }
  if (fd_ >= 0)
    ::close(fd_);
  if (is_server_)
    shm_unlink(shm_name_.c_str());
}

// ---------------------------------------------------------------------------
// Socket control
// ---------------------------------------------------------------------------

bool ShmStream::set_blocking(bool blocking) const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0)
    return false;
  flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
  return fcntl(fd_, F_SETFL, flags) == 0;
}

bool ShmStream::get_blocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0)
    return false;
  return (flags & O_NONBLOCK) == 0;
}

Endpoint ShmStream::local_endpoint() const { return local_ep_; }
Endpoint ShmStream::remote_endpoint() const { return remote_ep_; }

// ---------------------------------------------------------------------------
// Poll
// ---------------------------------------------------------------------------

bool ShmStream::wait_readable(const Duration& timeout) const {
  if (get_readable_bytes() > 0)
    return true;
  // For zero-timeout polls only check the ring buffer above.
  // The eventfd counter may be non-zero even when the ring buffer is empty
  // (if the eventfd was never drained after a previous read), so polling it
  // with 0ms would give a false positive and cause recv() to block forever.
  if (timeout.to_milliseconds() == 0)
    return false;
  struct pollfd pfd{};
  pfd.fd = data_rfd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  if (ret > 0 && (pfd.revents & POLLIN)) {
    // Drain the pipe so the next call does not return a stale notification.
    char buf[64];
    while (::read(data_rfd_, buf, sizeof(buf)) > 0) {}
    return true;
  }
  return false;
}

bool ShmStream::wait_writable(const Duration& timeout) const {
  if (get_writable_bytes() > 0)
    return true;
  // No backpressure signaling yet - just check the ring buffer
  struct pollfd pfd{};
  pfd.fd = data_rfd_;
  pfd.events = POLLIN;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & POLLIN);
}

bool ShmStream::wait_exception(const Duration& timeout) const {
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = 0;
  int timeout_ms = static_cast<int>(timeout.to_milliseconds());
  int ret = ::poll(&pfd, 1, timeout_ms);
  return ret > 0 && (pfd.revents & (POLLHUP | POLLERR | POLLNVAL));
}

// ---------------------------------------------------------------------------
// Ring buffer helpers
// ---------------------------------------------------------------------------

size_t ShmStream::get_readable_bytes() const {
  auto* hdr = read_half(shm_addr_, shm_size_, is_server_);
  return hdr->write_ptr.load(std::memory_order_acquire) - hdr->read_ptr.load(std::memory_order_relaxed);
}

size_t ShmStream::get_writable_bytes() const {
  auto* hdr = write_half(shm_addr_, shm_size_, is_server_);
  size_t capacity = shm_size_ / 2 - HEADER_SIZE;
  return capacity - (hdr->write_ptr.load(std::memory_order_relaxed) - hdr->read_ptr.load(std::memory_order_acquire));
}

void ShmStream::increment_read_ptr(size_t bytes) const {
  auto* hdr = read_half(shm_addr_, shm_size_, is_server_);
  hdr->read_ptr.fetch_add(bytes, std::memory_order_release);
}

void ShmStream::increment_write_ptr(size_t bytes) const {
  auto* hdr = write_half(shm_addr_, shm_size_, is_server_);
  hdr->write_ptr.fetch_add(bytes, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// I/O
// ---------------------------------------------------------------------------

ssize_t ShmStream::send(const uint8_t* buf, size_t len, int flags) const {
  auto* hdr = write_half(shm_addr_, shm_size_, is_server_);
  uint8_t* data = data_ptr(hdr);
  size_t capacity = shm_size_ / 2 - HEADER_SIZE;

  size_t written = 0;
  while (written < len) {
    uint64_t wp = hdr->write_ptr.load(std::memory_order_relaxed);
    uint64_t rp = hdr->read_ptr.load(std::memory_order_acquire);
    size_t writable = capacity - (wp - rp);
    if (writable == 0) {
      sched_yield();
      continue;
    }

    size_t to_write = std::min(len - written, writable);
    size_t pos = wp % capacity;
    size_t contiguous = capacity - pos;

    if (to_write <= contiguous) {
      memcpy(data + pos, buf + written, to_write);
    } else {
      memcpy(data + pos, buf + written, contiguous);
      memcpy(data, buf + written + contiguous, to_write - contiguous);
    }

    hdr->write_ptr.store(wp + to_write, std::memory_order_release);
    written += to_write;

    // Signal the peer after each chunk so it can drain the buffer while
    // we continue writing (needed when message > ring capacity).
    char sig = 1;
    (void)::write(signal_wfd_, &sig, 1);
  }

  return static_cast<ssize_t>(written);
}

ssize_t ShmStream::recv(uint8_t* buf, size_t len, int flags) const {
  auto* hdr = read_half(shm_addr_, shm_size_, is_server_);
  uint8_t* data = data_ptr(hdr);
  size_t capacity = shm_size_ / 2 - HEADER_SIZE;

  size_t consumed = 0;
  while (consumed < len) {
    uint64_t rp = hdr->read_ptr.load(std::memory_order_relaxed);
    uint64_t wp = hdr->write_ptr.load(std::memory_order_acquire);
    size_t readable = wp - rp;
    if (readable == 0) {
      // Wait for peer to signal data is available
      struct pollfd pfd{};
      pfd.fd = data_rfd_;
      pfd.events = POLLIN;
      if (::poll(&pfd, 1, -1) <= 0)
        return -1;
      char buf[64];
      while (::read(data_rfd_, buf, sizeof(buf)) > 0) {} // drain
      continue;
    }

    size_t to_read = std::min(len - consumed, readable);
    size_t pos = rp % capacity;
    size_t contiguous = capacity - pos;

    if (to_read <= contiguous) {
      memcpy(buf + consumed, data + pos, to_read);
    } else {
      memcpy(buf + consumed, data + pos, contiguous);
      memcpy(buf + consumed + contiguous, data, to_read - contiguous);
    }

    hdr->read_ptr.store(rp + to_read, std::memory_order_release);
    consumed += to_read;
  }

  return static_cast<ssize_t>(consumed);
}

} // namespace rix
