#pragma once

#include <gmock/gmock.h>

#include "rix/core/common.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/test/mock_socket.hpp"

namespace rix {

// Builder for configuring mock sockets with a fluent API
class SocketBuilder {
public:
  SocketBuilder(std::shared_ptr<MockSocket> socket)
      : socket_(socket), enable_notifications_(false), send_count_(0), recv_count_(0) {}

  // Enable automatic notifications on operation completion
  // Call this before setting up expectations if you want to use wait_for_operations
  SocketBuilder& enable_operation_notifications() {
    enable_notifications_ = true;
    return *this;
  }

  // Configure as a server socket (for pub/sub/service)
  SocketBuilder& as_server(const Endpoint& endpoint,
                           const Endpoint& bound_endpoint,
                           SocketFactory create_socket,
                           int accept_count = 0) {
    EXPECT_CALL(*socket_, set_reuse_address(true)).Times(1);
    EXPECT_CALL(*socket_, bind(endpoint)).Times(1);
    EXPECT_CALL(*socket_, listen(::testing::_)).Times(1);
    EXPECT_CALL(*socket_, local_endpoint()).Times(1).WillOnce(::testing::Return(bound_endpoint));
    EXPECT_CALL(*socket_, wait_exception(::testing::_)).Times(1);

    // Wait readable will return true until we have accepted the expected number of
    // connections
    auto accepted = std::make_shared<int>(0);
    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([accepted, accept_count](auto) { return *accepted < accept_count; }));
    if (accept_count > 0) {
      EXPECT_CALL(*socket_, accept(::testing::_))
          .Times(accept_count)
          .WillRepeatedly(::testing::Invoke([create_socket, accepted, socket, notify](Endpoint&) {
            auto sock = create_socket();
            (*accepted)++;
            if (notify) {
              if (auto s = socket.lock()) {
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
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    // Setup wait_writable on first call
    int current_index = send_count_;
    if (current_index == 0) {
      auto send_index = send_index_;
      EXPECT_CALL(*socket_, wait_writable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([send_index, current_index]() {
            return (*send_index)[current_index] < static_cast<int>(send_index->capacity());
          }));
    }
    send_count_++;

    send_index_->push_back(0);

    std::weak_ptr<MockSocket> socket = socket_;
    bool notify = enable_notifications_;
    auto send_index = send_index_;

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke(
            [opcode, msg, socket, notify, send_index, current_index](uint8_t op, const msg::Message& message) {
              EXPECT_EQ(op, opcode);
              auto _msg = dynamic_cast<const TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
              if (_msg) {
                EXPECT_EQ(*_msg, msg);
                (*send_index)[current_index]++;
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
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    // Setup wait_readable on first call
    int current_index = recv_count_;
    if (current_index == 0) {
      auto recv_index = recv_index_;
      EXPECT_CALL(*socket_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([recv_index, current_index]() {
            return (*recv_index)[current_index] < static_cast<int>(recv_index->capacity());
          }));
    }
    recv_count_++;
    recv_index_->push_back(0);

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    auto recv_index = recv_index_;

    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke(
            [msg, len, socket, notify, recv_index, current_index](msg::Message& message, size_t size) {
              EXPECT_EQ(size, len);
              auto _msg = dynamic_cast<TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
              if (_msg) {
                *_msg = msg;
                (*recv_index)[current_index]++;
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
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    // Setup wait_readable on first call (each recv_message with opcode needs 2 recv calls)
    int current_index = recv_count_;
    if (current_index == 0) {
      auto recv_index = recv_index_;
      EXPECT_CALL(*socket_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([recv_index, current_index]() {
            return (*recv_index)[current_index] < static_cast<int>(recv_index->capacity());
          }));
    }
    recv_count_ += 2;

    recv_index_->push_back(0);
    recv_index_->push_back(0);

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    auto recv_index = recv_index_;

    // First recv: Operation
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke([opcode, msg, recv_index, current_index](msg::Message& message, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&message);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = opcode;
            op->len = msg.size();
            (*recv_index)[current_index]++;
            return true;
          }
          return false;
        }));

    // Second recv: Message
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(
            ::testing::Invoke([msg, socket, notify, recv_index, current_index](msg::Message& message, size_t size) {
              EXPECT_EQ(size, msg.size());
              auto _msg = dynamic_cast<TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
              if (_msg) {
                *_msg = msg;
                (*recv_index)[current_index + 1]++;
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
  int send_count_;
  int recv_count_;
  std::shared_ptr<std::vector<int>> send_index_ = std::make_shared<std::vector<int>>();
  std::shared_ptr<std::vector<int>> recv_index_ = std::make_shared<std::vector<int>>();
  ::testing::Sequence seq_;
};

} // namespace rix