#pragma once

#include "rix/ipc/generic_socket.hpp"
#include "rix/ipc/poll.hpp"
#include <gmock/gmock.h>

namespace rix {

// TODO: Need to make this more configurable to simulate different poll results (i.e.
// subscribers should only receive when connection is readable, etc.). Should probably
// use a builder pattern like SocketBuilder.
class MockPoller : public GenericPoller {
public:
  MockPoller(int max_poll_count = -1) {
    std::shared_ptr<int> poll_count = std::make_shared<int>(max_poll_count);
    ON_CALL(*this, poll)
        .WillByDefault(
            [poll_count](
                const std::vector<std::shared_ptr<GenericSocket>>& all_sockets,
                const Duration& duration,
                PollFlag flag,
                std::vector<std::shared_ptr<GenericSocket>>& sockets,
                std::vector<std::shared_ptr<GenericSocket>>& exception_sockets) -> bool {
              if (*poll_count < 0) {
                sockets = all_sockets;
                exception_sockets.clear();
                return true;
              }
              if (*poll_count > 0) {
                (*poll_count)--;
                sockets = all_sockets;
                exception_sockets.clear();
                return true;
              }
              return true;
            });
  }

  MOCK_METHOD(bool,
              poll,
              (const std::vector<std::shared_ptr<GenericSocket>>& all_sockets,
               const Duration& duration,
               PollFlag flag,
               std::vector<std::shared_ptr<GenericSocket>>& sockets,
               std::vector<std::shared_ptr<GenericSocket>>& exception_sockets),
              (override));
};

} // namespace rix