#include "rix/ipc/poll.hpp"
#include "rix/ipc/generic_socket.hpp"

namespace rix {

bool SelectPoller::poll(const std::vector<std::shared_ptr<GenericSocket>> &all_sockets,
                        const Duration &duration, PollFlag flag,
                        std::vector<std::shared_ptr<GenericSocket>> &sockets,
                        std::vector<std::shared_ptr<GenericSocket>> &exception_sockets) {
  fd_set fds;
  fd_set exception_fds;
  FD_ZERO(&fds);
  FD_ZERO(&exception_fds);
  int max_fd = -1;
  for (const auto &socket : all_sockets) {
    int fd = socket->get_fd();
    FD_SET(fd, &fds);
    FD_SET(fd, &exception_fds);
    if (fd > max_fd) {
      max_fd = fd;
    }
  }
  if (max_fd < 0) {
    return false;
  }

  struct timeval tv;
  int64_t ns = duration.to_nanoseconds();
  tv.tv_sec = static_cast<long>(ns / 1'000'000'000);
  tv.tv_usec = static_cast<long>(ns % 1'000'000'000 / 1'000);

  int ret = ::select(max_fd + 1, (flag == PollFlag::READ) ? &fds : nullptr, (flag == PollFlag::WRITE) ? &fds : nullptr,
                     &exception_fds, (duration.to_nanoseconds() < 0) ? nullptr : &tv);
  if (ret < 0) {
    return false;
  }

  sockets.clear();
  sockets.reserve(all_sockets.size());
  exception_sockets.clear();
  exception_sockets.reserve(all_sockets.size());
  for (const auto &socket : all_sockets) {
    int fd = socket->get_fd();
    if (FD_ISSET(fd, &fds)) {
      sockets.push_back(socket);
    }
    if (FD_ISSET(fd, &exception_fds)) {
      exception_sockets.push_back(socket);
    }
  }
  sockets.shrink_to_fit();
  exception_sockets.shrink_to_fit();
  return true;
}

bool PollPoller::poll(const std::vector<std::shared_ptr<GenericSocket>> &all_sockets,
                      const Duration &duration, PollFlag flag,
                      std::vector<std::shared_ptr<GenericSocket>> &sockets,
                      std::vector<std::shared_ptr<GenericSocket>> &exception_sockets) {
  std::vector<struct pollfd> pfds;
  pfds.reserve(all_sockets.size());
  for (const auto &socket : all_sockets) {
    int fd = socket->get_fd();
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = 0;
    // Unnecessary to set for EXCEPT
    if (flag == PollFlag::READ) {
      pfd.events |= POLLIN;
    } else if (flag == PollFlag::WRITE) {
      pfd.events |= POLLOUT;
    }
    pfds.push_back(pfd);
  }

  int timeout_ms = duration.to_milliseconds();
  int ret = ::poll(pfds.data(), pfds.size(), timeout_ms);
  if (ret < 0) {
    return false;
  }
  sockets.clear();
  sockets.reserve(pfds.size());
  exception_sockets.clear();
  exception_sockets.reserve(pfds.size());

  for (size_t i = 0; i < pfds.size(); ++i) {
    if (pfds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
      exception_sockets.push_back(all_sockets[i]);
      continue;
    }
    if (flag == PollFlag::READ && (pfds[i].revents & POLLIN)) {
      sockets.push_back(all_sockets[i]);
    } else if (flag == PollFlag::WRITE && (pfds[i].revents & POLLOUT)) {
      sockets.push_back(all_sockets[i]);
    }
  }

  return true;
}

} // namespace rix