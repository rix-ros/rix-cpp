#pragma once

#include "rix/ipc/generic_socket.hpp"
#include "rix/ipc/poll.hpp"
#include <gmock/gmock.h>

namespace rix {

class MockPoller : public GenericPoller {
public:
  MockPoller() {
    ON_CALL(*this, poll)
        .WillByDefault([](const std::vector<std::shared_ptr<GenericSocket>> &all_sockets,
                          const Duration &duration, PollFlag flag,
                          std::vector<std::shared_ptr<GenericSocket>> &sockets,
                          std::vector<std::shared_ptr<GenericSocket>> &exception_sockets) -> bool {
          sockets = all_sockets;
          exception_sockets.clear();
          return true;
        });
  }

  MOCK_METHOD(bool, poll,
              (const std::vector<std::shared_ptr<GenericSocket>> &all_sockets, const Duration &duration,
               PollFlag flag, std::vector<std::shared_ptr<GenericSocket>> &sockets,
               std::vector<std::shared_ptr<GenericSocket>> &exception_sockets),
              (override));
};

} // namespace rix