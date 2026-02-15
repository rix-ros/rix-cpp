#pragma once

#include <gmock/gmock.h>

#include "rix/core/common.hpp"
#include "rix/test/mock_socket.hpp"

namespace rix {

// Builder for configuring mock sockets with a fluent API
class SocketBuilder {
public:
  SocketBuilder(const std::shared_ptr<MockSocket>& socket)
      : socket_(socket), enable_notifications_(false), send_count_(std::make_shared<int>(0)),
        recv_count_(std::make_shared<int>(0)), expected_send_count_(std::make_shared<int>(0)),
        expected_recv_count_(std::make_shared<int>(0)) {}

  // Enable automatic notifications on operation completion
  // Call this before setting up expectations if you want to use wait_for_operations
  SocketBuilder& enable_operation_notifications() {
    enable_notifications_ = true;
    return *this;
  }

  // Configure as a server socket (for pub/sub/service)
  SocketBuilder& as_server(const Endpoint& endpoint,
                           const Endpoint& bound_endpoint,
                           TransportFactory create_socket,
                           int accept_count = 0,
                           int iters_between_accept = 0) {
    EXPECT_CALL(*socket_, set_reuse_address(true)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*socket_, bind(endpoint))
        .Times(1)
        .WillOnce(::testing::Invoke([_endpoint = endpoint](const Endpoint& ep) {
          EXPECT_EQ(ep, _endpoint);
          return true;
        }));
    EXPECT_CALL(*socket_, listen(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*socket_, local_endpoint()).Times(1).WillOnce(::testing::Return(bound_endpoint));
    EXPECT_CALL(*socket_, wait_exception(::testing::_)).Times(1).WillOnce(::testing::Return(false));

    // Wait readable will return true until we have accepted the expected number of
    // connections
    auto accepted = std::make_shared<int>(0);
    std::weak_ptr<MockSocket> socket = socket_;
    auto iters = std::make_shared<int>(0);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
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
      EXPECT_CALL(*socket_, accept(::testing::_))
          .Times(accept_count)
          .WillRepeatedly(
              ::testing::Invoke([create_socket, accepted, socket, notify = enable_notifications_](Endpoint&) {
                auto sock = create_socket();
                (*accepted)++;
                if (notify) {
                  if (const auto s = socket.lock()) {
                    s->notify_operation_complete();
                  }
                }
                return sock;
              }));
    }
    close();
    return *this;
  }

  SocketBuilder& set_blocking(bool blocking) {
    EXPECT_CALL(*socket_, set_blocking(blocking)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*socket_, get_blocking()).Times(testing::AtLeast(0)).WillRepeatedly(::testing::Return(blocking));
    return *this;
  }

  SocketBuilder& connect(const Endpoint& endpoint) {
    EXPECT_CALL(*socket_, connect)
        .Times(1)
        .WillOnce(::testing::Invoke([_endpoint = endpoint](const Endpoint& endpoint) {
          EXPECT_EQ(endpoint, _endpoint);
          return true;
        }));
    return *this;
  }

  template <typename TMsg> SocketBuilder& send_message(uint8_t opcode, const TMsg& msg) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    // Setup wait_writable on first call
    if (*expected_send_count_ == 0) {
      EXPECT_CALL(*socket_, wait_writable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([send_count = send_count_, expected_send_count = expected_send_count_]() {
            return (*send_count) < *expected_send_count;
          }));
    }
    (*expected_send_count_)++;

    std::weak_ptr<MockSocket> socket = socket_;
    bool notify = enable_notifications_;

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke(
            [opcode, msg, socket, notify, send_count = send_count_](uint8_t operation, const Message& message) {
              EXPECT_EQ(operation, opcode);
              auto _msg = dynamic_cast<const TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
              (*send_count)++;
              if (_msg) {
                EXPECT_EQ(*_msg, msg);
                if (notify) {
                  if (auto s = socket.lock()) {
                    s->notify_operation_complete();
                  }
                }
                return true;
              }
              return false;
            }));
    return *this;
  }

  template <typename TMsg> SocketBuilder& recv_message(const TMsg& msg, size_t len) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    // Setup wait_readable on first call
    if (*expected_recv_count_ == 0) {
      EXPECT_CALL(*socket_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([recv_count = recv_count_, expected_recv_count = expected_recv_count_]() {
            return (*recv_count) < *expected_recv_count;
          }));
    }
    (*expected_recv_count_)++;

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;

    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(
            ::testing::Invoke([msg, len, socket, notify, recv_count = recv_count_](Message& message, size_t size) {
              EXPECT_EQ(size, len);
              auto _msg = dynamic_cast<TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
              (*recv_count)++;
              if (_msg) {
                *_msg = msg;
                if (notify) {
                  if (auto s = socket.lock()) {
                    s->notify_operation_complete();
                  }
                }
                return true;
              }
              return false;
            }));
    return *this;
  }

  template <typename TMsg> SocketBuilder& recv_message(uint8_t opcode, const TMsg& msg) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    // Setup wait_readable on first call (each recv_message with opcode needs 2 recv calls)
    if (*expected_recv_count_ == 0) {
      EXPECT_CALL(*socket_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([recv_count = recv_count_, expected_recv_count = expected_recv_count_]() {
            return (*recv_count) < *expected_recv_count;
          }));
    }
    (*expected_recv_count_) += 2;

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;

    // First recv: Operation
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke([opcode, msg, recv_count = recv_count_](Message& message, size_t size) {
          EXPECT_EQ(size, sys_msgs::Operation().get_prefix_len());
          auto operation = dynamic_cast<sys_msgs::Operation*>(&message);
          EXPECT_NE(operation, nullptr);
          (*recv_count)++;
          if (operation) {
            operation->opcode = opcode;
            operation->len = msg.get_prefix_len();
            return true;
          }
          return false;
        }));

    // Second recv: Message
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(
            ::testing::Invoke([msg, socket, notify, recv_count = recv_count_](Message& message, size_t size) {
              EXPECT_EQ(size, msg.get_prefix_len());
              auto _msg = dynamic_cast<TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
              (*recv_count)++;
              if (_msg) {
                *_msg = msg;
                if (notify) {
                  if (auto s = socket.lock()) {
                    s->notify_operation_complete();
                  }
                }
                return true;
              }
              return false;
            }));
    return *this;
  }

  SocketBuilder& close() {
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

private:
  std::shared_ptr<MockSocket> socket_;
  bool enable_notifications_;
  std::shared_ptr<int> send_count_;
  std::shared_ptr<int> recv_count_;
  std::shared_ptr<int> expected_send_count_;
  std::shared_ptr<int> expected_recv_count_;
  ::testing::Sequence seq_;
};

} // namespace rix