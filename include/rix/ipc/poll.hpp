#pragma once

#include <memory>
#include <sys/poll.h>
#include <sys/select.h>
#include <vector>

#include "rix/util/time.hpp"

namespace rix {

enum class PollFlag { READ = 1, WRITE = 2 };

class GenericSocket; // Forward declaration

class GenericPoller {
public:
  virtual ~GenericPoller() = default;
  virtual bool poll(const std::vector<std::shared_ptr<GenericSocket>> &all_sockets, const Duration &duration,
                    PollFlag flag, std::vector<std::shared_ptr<GenericSocket>> &sockets,
                    std::vector<std::shared_ptr<GenericSocket>> &exception_sockets) = 0;
};

class SelectPoller : public GenericPoller {
public:
  bool poll(const std::vector<std::shared_ptr<GenericSocket>> &all_sockets, const Duration &duration,
            PollFlag flag, std::vector<std::shared_ptr<GenericSocket>> &sockets,
            std::vector<std::shared_ptr<GenericSocket>> &exception_sockets) override;
};

class PollPoller : public GenericPoller {
public:
  bool poll(const std::vector<std::shared_ptr<GenericSocket>> &all_sockets, const Duration &duration,
            PollFlag flag, std::vector<std::shared_ptr<GenericSocket>> &sockets,
            std::vector<std::shared_ptr<GenericSocket>> &exception_sockets) override;
};

// Define the default poller here
using Poller = PollPoller;

} // namespace rix