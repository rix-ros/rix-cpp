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
  SocketBuilder(std::shared_ptr<MockSocket> socket) : socket_(socket), enable_notifications_(false) {}

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
    auto is_writable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_writable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_writable]() { return *is_writable; }));
    std::weak_ptr<MockSocket> socket = socket_;
    bool notify = enable_notifications_;
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(
            ::testing::Invoke([opcode, msg, socket, notify, is_writable](uint8_t op, const msg::Message& message) {
              *is_writable = false;
              EXPECT_EQ(op, opcode);
              auto _msg = dynamic_cast<const TMsg*>(&message);
              EXPECT_NE(_msg, nullptr);
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

  template <typename TMsg>
  SocketBuilder& send_message(uint8_t opcode, const std::vector<std::shared_ptr<TMsg>>& messages) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    int send_count = static_cast<int>(messages.size());
    auto send_index = std::make_shared<int>(0);
    EXPECT_CALL(*socket_, wait_writable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([send_index, send_count]() { return *send_index < send_count; }));
    std::weak_ptr<MockSocket> socket = socket_;
    bool notify = enable_notifications_;
    EXPECT_CALL(*socket_, send_message)
        .Times(::testing::AtLeast(send_count))
        .WillRepeatedly(::testing::Invoke(
            [send_index, messages, opcode, send_count, socket, notify](uint8_t op, const msg::Message& message) {
              int idx = *send_index;
              if (idx < send_count) {
                EXPECT_EQ(op, opcode);
                auto _msg = dynamic_cast<const TMsg*>(&message);
                EXPECT_NE(_msg, nullptr);
                if (_msg) {
                  auto msg = *(messages[idx]);
                  EXPECT_EQ(*_msg, msg);
                  (*send_index)++;
                  if (notify) {
                    if (auto s = socket.lock()) {
                      s->notify_operation_complete();
                    }
                  }
                  return true;
                }
              }
              return false;
            }));
    return *this;
  }

  template <typename TMsg> SocketBuilder& recv_message(const TMsg& msg, size_t len) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([msg, len, socket, notify](msg::Message& message, size_t size) {
          EXPECT_EQ(size, len);
          auto _msg = dynamic_cast<TMsg*>(&message);
          EXPECT_NE(_msg, nullptr);
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
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([opcode, msg](msg::Message& message, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&message);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = opcode;
            op->len = msg.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, msg, socket, notify](msg::Message& message, size_t size) {
          EXPECT_EQ(size, msg.size());
          *is_readable = false;
          auto _msg = dynamic_cast<TMsg*>(&message);
          EXPECT_NE(_msg, nullptr);
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

  template <typename TMsg> SocketBuilder& recv_message(uint8_t opcode, std::vector<std::shared_ptr<TMsg>> messages) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    int recv_count = static_cast<int>(messages.size() * 2); // Each message requires 2 recv calls
    auto recv_index = std::make_shared<int>(0);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([recv_index, recv_count]() { return *recv_index < recv_count; }));

    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, recv_message)
        .Times(::testing::AtLeast(recv_count))
        .WillRepeatedly(
            ::testing::Invoke([recv_index, messages, opcode, socket, notify](msg::Message& message, size_t len) {
              int idx = *recv_index;
              if (idx % 2 == 0) {
                // Even: Operation
                EXPECT_EQ(len, msg::mediator::Operation().size());
                auto op = dynamic_cast<msg::mediator::Operation*>(&message);
                if (op) {
                  op->len = messages[idx / 2]->size();
                  op->opcode = opcode;
                  ++(*recv_index);
                  return true;
                }
              } else {
                // Odd: TMsg
                if (idx / 2 < static_cast<int>(messages.size())) {
                  auto _msg = dynamic_cast<TMsg*>(&message);
                  EXPECT_NE(_msg, nullptr);
                  if (_msg) {
                    auto msg = *(messages[idx / 2]);
                    EXPECT_EQ(len, msg.size());
                    *_msg = msg;
                    ++(*recv_index);
                    if (notify) {
                      if (auto s = socket.lock()) {
                        s->notify_operation_complete();
                      }
                    }
                    return true;
                  }
                }
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
};

} // namespace rix