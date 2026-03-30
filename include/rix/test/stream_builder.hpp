#pragma once

#include <gmock/gmock.h>

#include "rix/core/common.hpp"
#include "rix/test/mock_stream.hpp"

namespace rix {

/**
 * @brief Fluent builder for programming MockStream expectations.
 *
 * Configures a MockStream to expect a specific sequence of send_message and
 * recv_message calls with content verification.
 */
class StreamBuilder {
public:
  explicit StreamBuilder(const std::shared_ptr<MockStream>& stream)
      : stream_(stream), enable_notifications_(false), send_count_(std::make_shared<int>(0)),
        recv_count_(std::make_shared<int>(0)), expected_send_count_(std::make_shared<int>(0)),
        expected_recv_count_(std::make_shared<int>(0)) {}

  StreamBuilder& enable_operation_notifications() {
    enable_notifications_ = true;
    return *this;
  }

  StreamBuilder& set_blocking(bool blocking) {
    EXPECT_CALL(*stream_, set_blocking(blocking)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*stream_, get_blocking()).Times(testing::AtLeast(0)).WillRepeatedly(::testing::Return(blocking));
    return *this;
  }

  /**
   * @brief Program the stream to expect a send_message call with the given opcode and message.
   */
  template <typename TMsg> StreamBuilder& send_message(uint8_t opcode, const TMsg& msg) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    if (*expected_send_count_ == 0) {
      EXPECT_CALL(*stream_, wait_writable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke(
              [send_count = send_count_, expected = expected_send_count_]() { return (*send_count) < *expected; }));
    }
    (*expected_send_count_)++;

    std::weak_ptr<MockStream> stream = stream_;
    bool notify = enable_notifications_;

    EXPECT_CALL(*stream_, send_message)
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke(
            [opcode, msg, stream, notify, send_count = send_count_](uint8_t op, const Message& message) {
              EXPECT_EQ(op, opcode);
              auto typed_msg = dynamic_cast<const TMsg*>(&message);
              EXPECT_NE(typed_msg, nullptr);
              (*send_count)++;
              if (typed_msg) {
                EXPECT_EQ(*typed_msg, msg);
                if (notify) {
                  if (auto s = stream.lock()) {
                    s->notify_operation_complete();
                  }
                }
                return true;
              }
              return false;
            }));
    return *this;
  }

  /**
   * @brief Program the stream to return a message on recv_message (raw form: no operation header).
   */
  template <typename TMsg> StreamBuilder& recv_message(const TMsg& msg, size_t len) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    if (*expected_recv_count_ == 0) {
      EXPECT_CALL(*stream_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke(
              [recv_count = recv_count_, expected = expected_recv_count_]() { return (*recv_count) < *expected; }));
    }
    (*expected_recv_count_)++;

    std::weak_ptr<MockStream> stream = stream_;
    bool notify = enable_notifications_;

    EXPECT_CALL(*stream_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke([msg, len, stream, notify, recv_count = recv_count_](Message& message, size_t sz) {
          EXPECT_EQ(sz, len);
          auto typed_msg = dynamic_cast<TMsg*>(&message);
          EXPECT_NE(typed_msg, nullptr);
          (*recv_count)++;
          if (typed_msg) {
            *typed_msg = msg;
            if (notify) {
              if (auto s = stream.lock()) {
                s->notify_operation_complete();
              }
            }
            return true;
          }
          return false;
        }));
    return *this;
  }

  /**
   * @brief Program the stream to return an Operation header followed by a message body.
   *        This is the typical recv pattern: first the Operation (opcode + length),
   *        then the actual message.
   */
  template <typename TMsg> StreamBuilder& recv_message(uint8_t opcode, const TMsg& msg) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    if (*expected_recv_count_ == 0) {
      EXPECT_CALL(*stream_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke(
              [recv_count = recv_count_, expected = expected_recv_count_]() { return (*recv_count) < *expected; }));
    }
    (*expected_recv_count_) += 2;

    std::weak_ptr<MockStream> stream = stream_;
    bool notify = enable_notifications_;

    // First recv: Operation header
    EXPECT_CALL(*stream_, recv_message(::testing::_, ::testing::_))
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

    // Second recv: Message body
    EXPECT_CALL(*stream_, recv_message(::testing::_, ::testing::_))
        .Times(1)
        .InSequence(seq_)
        .WillOnce(::testing::Invoke([msg, stream, notify, recv_count = recv_count_](Message& message, size_t size) {
          EXPECT_EQ(size, msg.get_prefix_len());
          auto typed_msg = dynamic_cast<TMsg*>(&message);
          EXPECT_NE(typed_msg, nullptr);
          (*recv_count)++;
          if (typed_msg) {
            *typed_msg = msg;
            if (notify) {
              if (auto s = stream.lock()) {
                s->notify_operation_complete();
              }
            }
            return true;
          }
          return false;
        }));
    return *this;
  }

  StreamBuilder& close() {
    // No-op for MockStream; kept for API symmetry with the old SocketBuilder
    return *this;
  }

private:
  std::shared_ptr<MockStream> stream_;
  bool enable_notifications_;
  std::shared_ptr<int> send_count_;
  std::shared_ptr<int> recv_count_;
  std::shared_ptr<int> expected_send_count_;
  std::shared_ptr<int> expected_recv_count_;
  ::testing::Sequence seq_;
};

} // namespace rix
