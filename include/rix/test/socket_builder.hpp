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
        .WillOnce(::testing::Invoke([opcode, msg, socket, notify, is_writable](uint8_t op, const msg::Message& message) {
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

  // Configure as a node registration socket
  SocketBuilder& as_node_register(const msg::mediator::NodeInfo& node_info,
                                  const Endpoint& rixhub_endpoint,
                                  bool should_fail = false) {
    connect(rixhub_endpoint);
    send_message(OPCODE::NODE_REGISTER, node_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = node_info.id;
    recv_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a node deregistration socket
  SocketBuilder& as_node_deregister(const msg::mediator::NodeInfo& node_info, const Endpoint& rixhub_endpoint) {
    connect(rixhub_endpoint);
    send_message(OPCODE::NODE_DEREGISTER, node_info);
    close();
    return *this;
  }

  // Configure as a publisher registration socket
  SocketBuilder&
  as_pub_register(const Endpoint& rixhub_endpoint, const msg::mediator::PubInfo& pub_info, bool should_fail = false) {
    connect(rixhub_endpoint);
    send_message(OPCODE::PUB_REGISTER, pub_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = pub_info.id;
    recv_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a publisher deregistration socket
  SocketBuilder& as_pub_deregister(const Endpoint& rixhub_endpoint, const msg::mediator::PubInfo& pub_info) {
    connect(rixhub_endpoint);
    send_message(OPCODE::PUB_DEREGISTER, pub_info);
    close();
    return *this;
  }

  // Configure as a subscriber registration socket
  SocketBuilder&
  as_sub_register(const Endpoint& rixhub_endpoint, const msg::mediator::SubInfo& sub_info, bool should_fail = false) {
    connect(rixhub_endpoint);
    send_message(OPCODE::SUB_REGISTER, sub_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = sub_info.id;
    recv_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a subscriber deregistration socket
  SocketBuilder& as_sub_deregister(const Endpoint& rixhub_endpoint, const msg::mediator::SubInfo& sub_info) {
    connect(rixhub_endpoint);
    send_message(OPCODE::SUB_DEREGISTER, sub_info);
    close();
    return *this;
  }

  // Configure as a service registration socket
  SocketBuilder&
  as_srv_register(const Endpoint& rixhub_endpoint, const msg::mediator::SrvInfo& srv_info, bool should_fail = false) {
    connect(rixhub_endpoint);
    send_message(OPCODE::SRV_REGISTER, srv_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = srv_info.id;
    recv_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a service deregistration socket
  SocketBuilder& as_srv_deregister(const Endpoint& rixhub_endpoint, const msg::mediator::SrvInfo& srv_info) {
    connect(rixhub_endpoint);
    send_message(OPCODE::SRV_DEREGISTER, srv_info);
    close();
    return *this;
  }

  // Configure as a service client request socket
  SocketBuilder& as_srv_cli_request(const Endpoint& rixhub_endpoint,
                                    const msg::mediator::SrvRequest& srv_req,
                                    const msg::mediator::SrvResponse& srv_res,
                                    bool should_fail = false) {
    connect(rixhub_endpoint);
    send_message(OPCODE::SRV_REQUEST, srv_req);
    msg::mediator::SrvResponse response;
    response.error = should_fail ? -1 : 0;
    if (!should_fail) {
      response.srv_info = srv_res.srv_info;
    }
    recv_message(OPCODE::SRV_RESPONSE, response);
    close();
    return *this;
  }

  // Configure parameter set request
  SocketBuilder&
  as_param_set(const Endpoint& rixhub_endpoint, const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    connect(rixhub_endpoint);
    send_message(OPCODE::PARAM_SET_REQUEST, param_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = param_info.id;
    recv_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure parameter get request
  SocketBuilder&
  as_param_get(const Endpoint& rixhub_endpoint, const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    connect(rixhub_endpoint);
    msg::mediator::ParamInfo info;
    info.id = param_info.id;
    info.name = param_info.name;
    info.message_hash = param_info.message_hash;
    send_message(OPCODE::PARAM_GET_REQUEST, info);
    if (!should_fail) {
      info.data = param_info.data;
    }
    recv_message(OPCODE::PARAM_GET_RESPONSE, info);
    close();
    return *this;
  }

  // Configure system info get request
  SocketBuilder& as_sys_info_request(const Endpoint& rixhub_endpoint,
                                     uint64_t node_id,
                                     const msg::mediator::SystemInfo& sys_info,
                                     bool should_fail = false) {
    connect(rixhub_endpoint);
    msg::standard::UInt64 id;
    id.data = node_id;
    send_message(OPCODE::SYSTEM_GET_REQUEST, id);
    if (should_fail) {
      msg::mediator::SystemInfo empty_info;
      recv_message(OPCODE::SYSTEM_GET_RESPONSE, empty_info);
    } else {
      recv_message(OPCODE::SYSTEM_GET_RESPONSE, sys_info);
    }
    close();
    return *this;
  }

  // Configure as a mediator socket for node registration
  SocketBuilder& as_med_node_register(const msg::mediator::NodeInfo& node_info, bool should_fail = false) {
    recv_message(OPCODE::NODE_REGISTER, node_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = node_info.id;
    send_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a mediator socket for node deregistration
  SocketBuilder& as_med_node_deregister(const msg::mediator::NodeInfo& node_info) {
    recv_message(OPCODE::NODE_DEREGISTER, node_info);
    close();
    return *this;
  }

  // Configure as a mediator socket for pub registration
  SocketBuilder& as_med_pub_register(const msg::mediator::PubInfo& pub_info, bool should_fail = false) {
    recv_message(OPCODE::PUB_REGISTER, pub_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = pub_info.id;
    send_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a mediator socket for pub deregistration
  SocketBuilder& as_med_pub_deregister(const msg::mediator::PubInfo& pub_info) {
    recv_message(OPCODE::PUB_DEREGISTER, pub_info);
    close();
    return *this;
  }

  // Configure as a mediator socket for sub registration
  SocketBuilder& as_med_sub_register(const msg::mediator::SubInfo& sub_info, bool should_fail = false) {
    recv_message(OPCODE::SUB_REGISTER, sub_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = sub_info.id;
    send_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a mediator socket for sub deregistration
  SocketBuilder& as_med_sub_deregister(const msg::mediator::SubInfo& sub_info) {
    recv_message(OPCODE::SUB_DEREGISTER, sub_info);
    close();
    return *this;
  }

  // Configure as a mediator socket for srv registration
  SocketBuilder& as_med_srv_register(const msg::mediator::SrvInfo& srv_info, bool should_fail = false) {
    recv_message(OPCODE::SRV_REGISTER, srv_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = srv_info.id;
    send_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a mediator socket for srv deregistration
  SocketBuilder& as_med_srv_deregister(const msg::mediator::SrvInfo& srv_info) {
    recv_message(OPCODE::SRV_DEREGISTER, srv_info);
    close();
    return *this;
  }

  // Configure as mediator socket for service client request
  SocketBuilder& as_med_srv_cli_request(const msg::mediator::SrvRequest& srv_req,
                                        const msg::mediator::SrvResponse& srv_res,
                                        bool should_fail = false) {
    recv_message(OPCODE::SRV_REQUEST, srv_req);
    msg::mediator::SrvResponse response;
    response.error = should_fail ? -1 : 0;
    if (!should_fail) {
      response.srv_info = srv_res.srv_info;
    }
    send_message(OPCODE::SRV_RESPONSE, response);
    close();
    return *this;
  }

  // Configure as a mediator socket for parameter get request
  SocketBuilder& as_med_param_get(const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    msg::mediator::ParamInfo info;
    info.id = param_info.id;
    info.name = param_info.name;
    info.message_hash = param_info.message_hash;
    recv_message(OPCODE::PARAM_GET_REQUEST, info);
    if (!should_fail) {
      info.data = param_info.data;
    }
    send_message(OPCODE::PARAM_GET_RESPONSE, info);
    close();
    return *this;
  }

  // Configure as a mediator socket for parameter set request
  SocketBuilder& as_med_param_set(const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    recv_message(OPCODE::PARAM_SET_REQUEST, param_info);
    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = param_info.id;
    send_message(OPCODE::STATUS_RESPONSE, status);
    close();
    return *this;
  }

  // Configure as a mediator socket for system info request
  SocketBuilder&
  as_med_sys_info_request(const msg::mediator::SystemInfo& sys_info, uint64_t node_id, bool should_fail = false) {
    msg::standard::UInt64 id;
    id.data = node_id;
    recv_message(OPCODE::SYSTEM_GET_REQUEST, id);
    if (should_fail) {
      msg::mediator::SystemInfo empty_info;
      send_message(OPCODE::SYSTEM_GET_RESPONSE, empty_info);
    } else {
      send_message(OPCODE::SYSTEM_GET_RESPONSE, sys_info);
    }
    close();
    return *this;
  }

  // Configure as a mediator socket for sub notification
  SocketBuilder& as_med_sub_notify(const Endpoint& endpoint, const msg::mediator::SubNotify& sub_notify) {
    connect(endpoint); // Dummy endpoint since we don't actually connect
    send_message(OPCODE::SUB_NOTIFY, sub_notify);
    close();
    return *this;
  }

  // Configure as a publisher connection socket (accepts and sends messages)
  template <typename TMsg> SocketBuilder& as_pub_connection(const std::vector<std::shared_ptr<TMsg>>& messages) {
    send_message(OPCODE::PUB_MESSAGE, messages);
    close();
    return *this;
  }

  // Configure as a service connection socket (accepts and handles request/response)
  template <typename TRequest, typename TResponse>
  SocketBuilder& as_srv_connection(std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    recv_message(OPCODE::SRV_REQUEST_MESSAGE, *request);
    send_message(OPCODE::SRV_RESPONSE_MESSAGE, *response);
    close();
    return *this;
  }

  // Configure as a subscriber client socket (connects and receives messages)
  template <typename TMsg>
  SocketBuilder& as_sub_client(const Endpoint& endpoint, const std::vector<std::shared_ptr<TMsg>>& messages) {
    set_blocking(false);
    connect(endpoint);
    recv_message(OPCODE::PUB_MESSAGE, messages);
    close();
    return *this;
  }

  // Configure as a service client connection socket (connects, sends request, receives
  // response)
  template <typename TRequest, typename TResponse>
  SocketBuilder&
  as_srv_cli_client(const Endpoint& endpoint, std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    connect(endpoint);
    send_message(OPCODE::SRV_REQUEST_MESSAGE, *request);
    recv_message(OPCODE::SRV_RESPONSE_MESSAGE, *response);
    close();
    return *this;
  }

  // Configure as a subscriber notification connection socket
  SocketBuilder& as_sub_connection(const msg::mediator::SubNotify& notify) {
    recv_message(OPCODE::SUB_NOTIFY, notify);
    close();
    return *this;
  }

private:
  std::shared_ptr<MockSocket> socket_;
  bool enable_notifications_;
};

} // namespace rix