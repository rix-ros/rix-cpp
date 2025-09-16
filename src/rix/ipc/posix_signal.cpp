#include "rix/ipc/posix_signal.hpp"

#include <iostream>

namespace rix {
namespace ipc {

std::array<POSIXSignal::Notifier, 32> POSIXSignal::notifier = {};

POSIXSignal::POSIXSignal(int signum) : signum_(signum) {
  if (signum < 1 || signum > 32) {
    signum_ = -1;
    return;
  }
  if (notifier[signum_ - 1].is_init) {
    signum_ = -1;
    return;
  }
  if (!notifier[signum_ - 1].is_init) {
    notifier[signum_ - 1].is_init = true;
    ::pipe(notifier[signum_ - 1].pipe.data());
    ::signal(signum_, handler);
  }
}

POSIXSignal::~POSIXSignal() {
  if (signum_ < 1 || signum_ > 32)
    return;
  if (notifier[signum_ - 1].is_init) {
    notifier[signum_ - 1].is_init = false;
    notifier[signum_ - 1].pipe = {};
    ::signal(signum_, SIG_DFL);
  }
}

bool POSIXSignal::ignore() const {
  if (signum_ < 1 || signum_ > 32)
    return false;
  return ::signal(signum_, SIG_IGN) != SIG_ERR;
}

bool POSIXSignal::raise() const {
  if (signum_ < 1 || signum_ > 32)
    return false;
  return ::raise(signum_) == 0;
}

bool POSIXSignal::wait(const rix::util::Duration &d) const {
  if (signum_ < 1 || signum_ > 32)
    return false;

  bool is_readable = false;
  // Use select to wait for the read end of the pipe to be readable
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(notifier[signum_ - 1].pipe[0], &read_fds);
  struct timeval timeout;
  timeout.tv_sec = d.to_nanoseconds() / 1'000'000'000;
  timeout.tv_usec = (d.to_nanoseconds() % 1'000'000'000) / 1'000;
  is_readable = select(notifier[signum_ - 1].pipe[0] + 1, &read_fds, nullptr,
                       nullptr, &timeout) > 0;

  if (is_readable) {
    int signum_read = -1;
    ssize_t bytes_read = read(notifier[signum_ - 1].pipe[0],
                              (uint8_t *)&signum_read, sizeof(int));
    if (bytes_read != sizeof(int))
      return false;
    return signum_read == signum_;
  }
  return false;
}

void POSIXSignal::handler(int signum) {
  if (notifier[signum - 1].is_init) {
    write(notifier[signum - 1].pipe[1], (uint8_t *)&signum, sizeof(int));
  }
}

} // namespace ipc
} // namespace rix