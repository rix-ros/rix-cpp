#pragma once

#include <gmock/gmock.h>

#include "rix/core/common.hpp"
#include "rix/test/mock_acceptor.hpp"
#include "rix/test/transport_manager.hpp"

namespace rix {

/**
 * @brief Fluent builder for programming MockAcceptor expectations.
 *
 * Configures a MockAcceptor to simulate server behavior: binding, listening,
 * accepting connections (returning MockStreams), and reporting endpoints.
 */
class AcceptorBuilder {
public:
  explicit AcceptorBuilder(const std::shared_ptr<MockAcceptor>& acceptor) : acceptor_(acceptor) {}

  /**
   * @brief Configure the acceptor to report a specific local endpoint and pass
   *        exception / readability checks. Simulates a successfully bound+listening server.
   *
   * @param bound_endpoint The endpoint the acceptor reports as its local address.
   * @param accept_count   Number of connections to accept before wait_readable returns false.
   * @param create_stream  Factory to create the MockStream returned by each accept().
   *                        Typically comes from TransportManager.
   * @param iters_between_accept  Number of wait_readable calls that return false before
   *                               allowing the next accept (simulates delay).
   */
  AcceptorBuilder& as_server(const Endpoint& bound_endpoint,
                             int accept_count,
                             std::function<std::shared_ptr<Stream>()> create_stream,
                             int iters_between_accept = 0) {
    EXPECT_CALL(*acceptor_, wait_exception(::testing::_)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*acceptor_, local_endpoint())
        .Times(::testing::AtLeast(1))
        .WillRepeatedly(::testing::Return(bound_endpoint));

    auto accepted = std::make_shared<int>(0);
    auto iters = std::make_shared<int>(0);
    std::weak_ptr<MockAcceptor> acceptor = acceptor_;
    bool notify = enable_notifications_;

    EXPECT_CALL(*acceptor_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([accepted, accept_count, iters, iters_between_accept](auto) {
          if (*iters < iters_between_accept) {
            (*iters)++;
            return false;
          }
          *iters = 0;
          return *accepted < accept_count;
        }));

    if (accept_count > 0) {
      EXPECT_CALL(*acceptor_, accept(::testing::_))
          .Times(accept_count)
          .WillRepeatedly(::testing::Invoke([create_stream, accepted, acceptor, notify](Endpoint&) {
            auto stream = create_stream();
            (*accepted)++;
            if (notify) {
              if (const auto a = acceptor.lock()) {
                a->notify_operation_complete();
              }
            }
            return stream;
          }));
    }

    return *this;
  }

  AcceptorBuilder& enable_operation_notifications() {
    enable_notifications_ = true;
    return *this;
  }

private:
  std::shared_ptr<MockAcceptor> acceptor_;
  bool enable_notifications_ = false;
};

} // namespace rix
