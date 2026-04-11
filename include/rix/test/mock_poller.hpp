#pragma once

#include <gmock/gmock.h>

#include "rix/ipc/poll.hpp"

namespace rix {

class MockPoller final : public GenericPoller {
public:
  explicit MockPoller(int max_poll_count = -1) {
    auto poll_count = std::make_shared<int>(max_poll_count);
    ON_CALL(*this, poll)
        .WillByDefault([poll_count](const std::vector<std::shared_ptr<Pollable>>& all_pollables,
                                    const Duration& duration,
                                    PollFlag flag,
                                    std::vector<std::shared_ptr<Pollable>>& ready,
                                    std::vector<std::shared_ptr<Pollable>>& exception) -> bool {
          if (*poll_count < 0) {
            ready = all_pollables;
            exception.clear();
            return true;
          }
          if (*poll_count > 0) {
            (*poll_count)--;
            ready = all_pollables;
            exception.clear();
            return true;
          }
          return true;
        });
  }

  MOCK_METHOD(bool,
              poll,
              (const std::vector<std::shared_ptr<Pollable>>& all_pollables,
               const Duration& duration,
               PollFlag flag,
               std::vector<std::shared_ptr<Pollable>>& ready,
               std::vector<std::shared_ptr<Pollable>>& exception),
              (override));
};

} // namespace rix
