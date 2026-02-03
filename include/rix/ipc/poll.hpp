#pragma once

#include <memory>
#include <sys/poll.h>
#include <sys/select.h>
#include <vector>

#include "rix/util/time.hpp"

namespace rix {

enum class PollFlag { READ = 1, WRITE = 2 };

class Pollable; // Forward declaration

class GenericPoller {
public:
  virtual ~GenericPoller() = default;
  virtual bool poll(const std::vector<std::shared_ptr<Pollable>>& pollables,
                    const Duration& duration,
                    PollFlag flag,
                    std::vector<std::shared_ptr<Pollable>>& success,
                    std::vector<std::shared_ptr<Pollable>>& exception) = 0;
};

class SelectPoller final : public GenericPoller {
public:
  bool poll(const std::vector<std::shared_ptr<Pollable>>& pollables,
            const Duration& duration,
            PollFlag flag,
            std::vector<std::shared_ptr<Pollable>>& success,
            std::vector<std::shared_ptr<Pollable>>& exception) override;
};

class PollPoller final : public GenericPoller {
public:
  bool poll(const std::vector<std::shared_ptr<Pollable>>& pollables,
            const Duration& duration,
            PollFlag flag,
            std::vector<std::shared_ptr<Pollable>>& success,
            std::vector<std::shared_ptr<Pollable>>& exception) override;
};

class Pollable {
  friend class PollPoller;
  friend class SelectPoller;

public:
  bool is_readable() const { return wait_readable(Duration(0.0)); }
  bool is_writable() const { return wait_writable(Duration(0.0)); }
  bool is_exception() const { return wait_exception(Duration(0.0)); }

  virtual bool wait_readable(const Duration& timeout) const = 0;
  virtual bool wait_writable(const Duration& timeout) const = 0;
  virtual bool wait_exception(const Duration& timeout) const = 0;

  static std::shared_ptr<GenericPoller> get_poller() { return poller_; }
  static void set_poller(const std::shared_ptr<GenericPoller>& poller) { poller_ = poller; }
  static bool poll(const std::vector<std::shared_ptr<Pollable>>& pollables,
                   const Duration& duration,
                   const PollFlag flag,
                   std::vector<std::shared_ptr<Pollable>>& success,
                   std::vector<std::shared_ptr<Pollable>>& exception) {
    if (!poller_) {
      return false;
    }
    return poller_->poll(pollables, duration, flag, success, exception);
  }

private:
  static inline std::shared_ptr<GenericPoller> poller_{std::make_shared<Poller>()};
  virtual bool get_fd(int& fd) const { return false; }
};

// Define the default poller here
using Poller = PollPoller;

} // namespace rix