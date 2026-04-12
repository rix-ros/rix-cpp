#include "rix/ipc/poll.hpp"
#include "rix/util/log.hpp"

namespace rix {

bool SelectPoller::poll(const std::vector<std::shared_ptr<Pollable>>& pollables,
                        const Duration& duration,
                        PollFlag flag,
                        std::vector<std::shared_ptr<Pollable>>& success,
                        std::vector<std::shared_ptr<Pollable>>& exception) {
  success.clear();
  exception.clear();

  // Separate fd-based and non-fd-based pollables
  std::vector<std::shared_ptr<Pollable>> fd_pollables;
  std::vector<std::shared_ptr<Pollable>> non_fd_pollables;
  fd_set fds;
  fd_set exception_fds;
  FD_ZERO(&fds);
  FD_ZERO(&exception_fds);
  int max_fd = -1;

  for (const auto& p : pollables) {
    int fd = -1;
    if (p->get_fd(fd)) {
      fd_pollables.push_back(p);
      FD_SET(fd, &fds);
      FD_SET(fd, &exception_fds);
      if (fd > max_fd) {
        max_fd = fd;
      }
    } else {
      non_fd_pollables.push_back(p);
    }
  }

  // For non-fd pollables, check them with zero timeout
  for (const auto& pollable : non_fd_pollables) {
    bool ready =
        (flag == PollFlag::READ) ? pollable->wait_readable(Duration(0.0)) : pollable->wait_writable(Duration(0.0));
    if (ready) {
      success.push_back(pollable);
    }
  }

  // Poll fd-based pollables if any exist
  if (max_fd >= 0) {
    struct timeval tv{};
    const int64_t ns = duration.to_nanoseconds();
    tv.tv_sec = static_cast<int>(ns / 1'000'000'000);
    tv.tv_usec = static_cast<int>(ns % 1'000'000'000 / 1'000);

    int ret = ::select(max_fd + 1,
                       (flag == PollFlag::READ) ? &fds : nullptr,
                       (flag == PollFlag::WRITE) ? &fds : nullptr,
                       &exception_fds,
                       (duration.to_nanoseconds() < 0) ? nullptr : &tv);
    if (ret < 0) {
      return false;
    }

    for (const auto& p : fd_pollables) {
      int fd = -1;
      if (p->get_fd(fd)) {
        if (FD_ISSET(fd, &fds)) {
          success.push_back(p);
        }
        if (FD_ISSET(fd, &exception_fds)) {
          exception.push_back(p);
        }
      }
    }
  }

  return true;
}

bool PollPoller::poll(const std::vector<std::shared_ptr<Pollable>>& pollables,
                      const Duration& duration,
                      PollFlag flag,
                      std::vector<std::shared_ptr<Pollable>>& success,
                      std::vector<std::shared_ptr<Pollable>>& exception) {
  success.clear();
  exception.clear();

  // Separate fd-based and non-fd-based pollables
  std::vector<std::shared_ptr<Pollable>> fd_pollables;
  std::vector<std::shared_ptr<Pollable>> non_fd_pollables;
  std::vector<struct pollfd> poll_fds;

  for (const auto& pollable : pollables) {
    int fd = -1;
    if (pollable->get_fd(fd)) {
      fd_pollables.push_back(pollable);
      struct pollfd pfd;
      pfd.fd = fd;
      pfd.events = (flag == PollFlag::READ) ? POLLIN : POLLOUT;
      pfd.revents = 0;
      poll_fds.push_back(pfd);
    } else {
      non_fd_pollables.push_back(pollable);
    }
  }

  // Log::info << "Fd: " << fd_pollables.size() << ", Non-fd: " << non_fd_pollables.size() << std::endl;

  // For non-fd pollables, check them with zero timeout
  for (const auto& pollable : non_fd_pollables) {
    bool ready =
        (flag == PollFlag::READ) ? pollable->wait_readable(Duration(0.0)) : pollable->wait_writable(Duration(0.0));
    if (ready) {
      success.push_back(pollable);
    }
  }

  // Poll fd-based pollables if any exist
  if (!poll_fds.empty()) {
    int timeout_ms = static_cast<int>(duration.to_milliseconds());
    int ret = ::poll(poll_fds.data(), poll_fds.size(), timeout_ms);
    // Log::info << "ret: " << ret << std::endl;
    if (ret > 0) {
      for (size_t i = 0; i < poll_fds.size(); ++i) {
        if (poll_fds[i].revents & (POLLIN | POLLOUT)) {
          success.push_back(fd_pollables[i]);
        }
        if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
          exception.push_back(fd_pollables[i]);
        }
      }
    }
  }

  return true;
}

} // namespace rix