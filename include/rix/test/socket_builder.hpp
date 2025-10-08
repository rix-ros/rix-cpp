#pragma once

#include <gmock/gmock.h>

#include "rix/core/common.hpp"
#include "rix/ipc/mock_socket.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/Status.hpp"

namespace rix {

// Builder for configuring mock sockets with a fluent API
class SocketBuilder {
public:
  SocketBuilder(std::shared_ptr<MockSocket> socket) : socket_(socket) {}

  // Configure as a node registration socket
  SocketBuilder& as_node_register(const Endpoint& rixhub_endpoint,
                                  const msg::mediator::NodeInfo& node_info,
                                  bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([](uint8_t, const msg::Message& msg) {
          auto info = dynamic_cast<const msg::mediator::NodeInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          return true;
        }));

    expect_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a node deregistration socket
  SocketBuilder& as_node_deregister(const Endpoint& rixhub_endpoint,
                                    const msg::mediator::NodeInfo& node_info) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([node_info](uint8_t, const msg::Message& msg) {
          auto info = dynamic_cast<const msg::mediator::NodeInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->name, node_info.name);
            return true;
          }
          return false;
        }));

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a publisher registration socket
  SocketBuilder& as_pub_register(const Endpoint& rixhub_endpoint,
                                 const msg::mediator::PubInfo& pub_info,
                                 bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([pub_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::PUB_REGISTER);
          auto info = dynamic_cast<const msg::mediator::PubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->topic_info.name, pub_info.topic_info.name);
            EXPECT_EQ(info->topic_info.message_hash, pub_info.topic_info.message_hash);
            EXPECT_EQ(info->endpoint.address, pub_info.endpoint.address);
            EXPECT_EQ(info->endpoint.port, pub_info.endpoint.port);
            return true;
          }
          return false;
        }));

    expect_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a publisher deregistration socket
  SocketBuilder& as_pub_deregister(const Endpoint& rixhub_endpoint,
                                   const msg::mediator::PubInfo& pub_info) {
    connect_to_rixhub(rixhub_endpoint);

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([pub_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::PUB_DEREGISTER);
          auto info = dynamic_cast<const msg::mediator::PubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->topic_info.name, pub_info.topic_info.name);
            EXPECT_EQ(info->topic_info.message_hash, pub_info.topic_info.message_hash);
            EXPECT_EQ(info->endpoint.address, pub_info.endpoint.address);
            EXPECT_EQ(info->endpoint.port, pub_info.endpoint.port);
            return true;
          }
          return false;
        }));

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a subscriber registration socket
  SocketBuilder& as_sub_register(const Endpoint& rixhub_endpoint,
                                 const msg::mediator::SubInfo& sub_info,
                                 bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([sub_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SUB_REGISTER);
          auto info = dynamic_cast<const msg::mediator::SubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->topic_info.name, sub_info.topic_info.name);
            EXPECT_EQ(info->topic_info.message_hash, sub_info.topic_info.message_hash);
            EXPECT_EQ(info->endpoint.address, sub_info.endpoint.address);
            EXPECT_EQ(info->endpoint.port, sub_info.endpoint.port);
            return true;
          }
          return false;
        }));

    expect_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a subscriber deregistration socket
  SocketBuilder& as_sub_deregister(const Endpoint& rixhub_endpoint,
                                   const msg::mediator::SubInfo& sub_info) {
    connect_to_rixhub(rixhub_endpoint);

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([sub_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SUB_DEREGISTER);
          auto info = dynamic_cast<const msg::mediator::SubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->topic_info.name, sub_info.topic_info.name);
            EXPECT_EQ(info->topic_info.message_hash, sub_info.topic_info.message_hash);
            EXPECT_EQ(info->endpoint.address, sub_info.endpoint.address);
            EXPECT_EQ(info->endpoint.port, sub_info.endpoint.port);
            return true;
          }
          return false;
        }));

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a service registration socket
  SocketBuilder& as_srv_register(const Endpoint& rixhub_endpoint,
                                 const msg::mediator::SrvInfo& srv_info,
                                 bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([srv_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SRV_REGISTER);
          auto info = dynamic_cast<const msg::mediator::SrvInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->name, srv_info.name);
            EXPECT_EQ(info->request_hash, srv_info.request_hash);
            EXPECT_EQ(info->response_hash, srv_info.response_hash);
            EXPECT_EQ(info->endpoint.address, srv_info.endpoint.address);
            EXPECT_EQ(info->endpoint.port, srv_info.endpoint.port);
            return true;
          }
          return false;
        }));

    expect_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a service deregistration socket
  SocketBuilder& as_srv_deregister(const Endpoint& rixhub_endpoint,
                                   const msg::mediator::SrvInfo& srv_info) {
    connect_to_rixhub(rixhub_endpoint);

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([srv_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SRV_DEREGISTER);
          auto info = dynamic_cast<const msg::mediator::SrvInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->name, srv_info.name);
            EXPECT_EQ(info->request_hash, srv_info.request_hash);
            EXPECT_EQ(info->response_hash, srv_info.response_hash);
            EXPECT_EQ(info->endpoint.address, srv_info.endpoint.address);
            EXPECT_EQ(info->endpoint.port, srv_info.endpoint.port);
            return true;
          }
          return false;
        }));

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a service client request socket
  SocketBuilder& as_srv_cli_request(const Endpoint& rixhub_endpoint,
                                    const msg::mediator::SrvRequest& srv_req,
                                    const msg::mediator::SrvResponse& srv_res,
                                    bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([srv_req](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SRV_REQUEST);
          auto info = dynamic_cast<const msg::mediator::SrvRequest*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->name, srv_req.name);
            EXPECT_EQ(info->request_hash, srv_req.request_hash);
            EXPECT_EQ(info->response_hash, srv_req.response_hash);
            return true;
          }
          return false;
        }));

    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([srv_res](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SRV_RESPONSE;
            op->len = srv_res.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke(
            [should_fail, is_readable, srv_res](msg::Message& msg, size_t size) {
              EXPECT_EQ(size, srv_res.size());
              *is_readable = false;
              auto response = dynamic_cast<msg::mediator::SrvResponse*>(&msg);
              EXPECT_NE(response, nullptr);
              if (response) {
                if (should_fail) {
                  response->error = -1;
                } else {
                  response->error = 0;
                  response->srv_info = srv_res.srv_info;
                }
                return true;
              }
              return false;
            }));

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure parameter set request
  SocketBuilder& as_param_set(const Endpoint& rixhub_endpoint,
                              const msg::mediator::ParamInfo& param_info,
                              bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(
            ::testing::Invoke([param_info](uint8_t opcode, const msg::Message& msg) {
              EXPECT_EQ(opcode, OPCODE::PARAM_SET_REQUEST);
              auto info = dynamic_cast<const msg::mediator::ParamInfo*>(&msg);
              EXPECT_NE(info, nullptr);
              if (info) {
                EXPECT_EQ(info->name, param_info.name);
                EXPECT_EQ(info->message_hash, param_info.message_hash);
                return true;
              }
              return false;
            }));

    expect_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure parameter get request
  SocketBuilder& as_param_get(const Endpoint& rixhub_endpoint,
                              const msg::mediator::ParamInfo& param_info,
                              std::shared_ptr<msg::Message> value,
                              bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(
            ::testing::Invoke([param_info](uint8_t opcode, const msg::Message& msg) {
              EXPECT_EQ(opcode, OPCODE::PARAM_GET_REQUEST);
              auto info = dynamic_cast<const msg::mediator::ParamInfo*>(&msg);
              EXPECT_NE(info, nullptr);
              if (info) {
                EXPECT_EQ(info->name, param_info.name);
                EXPECT_EQ(info->message_hash, param_info.message_hash);
                return true;
              }
              return false;
            }));

    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([value](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::PARAM_GET_RESPONSE;
            op->len = value->size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([should_fail, is_readable, param_info, value](
                                        msg::Message& msg, size_t size) {
          EXPECT_EQ(size, value->size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::ParamInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            if (should_fail) {
              // Simulate failure by not setting the value
              return true;
            } else {
              info->name = param_info.name;
              info->message_hash = param_info.message_hash;
              info->data.resize(value->size());
              size_t offset = 0;
              value->serialize(info->data.data(), offset);
              return true;
            }
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure system info get request
  SocketBuilder& as_sys_info_request(const Endpoint& rixhub_endpoint,
                                     const msg::mediator::SystemInfo& sys_info,
                                     bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SYSTEM_GET_REQUEST);
          auto info = dynamic_cast<const msg::standard::UInt64*>(&msg);
          EXPECT_NE(info, nullptr);
          return true;
        }));
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([sys_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SYSTEM_GET_RESPONSE;
            op->len = sys_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke(
            [should_fail, is_readable, sys_info](msg::Message& msg, size_t size) {
              EXPECT_EQ(size, sys_info.size());
              *is_readable = false;
              auto info = dynamic_cast<msg::mediator::SystemInfo*>(&msg);
              EXPECT_NE(info, nullptr);
              if (info) {
                if (should_fail) {
                  // Simulate failure by not setting the value
                  return true;
                } else {
                  info->nodes = sys_info.nodes;
                  info->publishers = sys_info.publishers;
                  info->subscribers = sys_info.subscribers;
                  info->services = sys_info.services;
                  info->actions = sys_info.actions;
                  info->topics = sys_info.topics;
                  return true;
                }
              }
              return false;
            }));
    EXPECT_CALL(*socket_, close()).Times(1);
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
    EXPECT_CALL(*socket_, local_endpoint())
        .Times(1)
        .WillOnce(::testing::Return(bound_endpoint));
    EXPECT_CALL(*socket_, wait_exception(::testing::_)).Times(1);

    if (accept_count > 0) {
      // Wait readable will return true until we have accepted the expected number of
      // connections
      auto accepted = std::make_shared<int>(0);
      EXPECT_CALL(*socket_, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke(
              [accepted, accept_count](auto) { return *accepted < accept_count; }));

      EXPECT_CALL(*socket_, accept(::testing::_))
          .Times(accept_count)
          .WillRepeatedly(::testing::Invoke([create_socket, accepted](Endpoint&) {
            auto sock = create_socket();
            (*accepted)++;
            return sock;
          }));
    }

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

private:
  std::shared_ptr<MockSocket> socket_;

  void expect_status_response(bool should_fail) {
    // Wait readable will return true until we have read both messages
    auto is_readable = std::make_shared<bool>(true);
    // wait_readable can be called multiple times in a loop, so expect 0 or more times
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::STATUS_RESPONSE;
            op->len = msg::mediator::Status().size();
            return true;
          }
          return false;
        }))
        .WillOnce(
            ::testing::Invoke([should_fail, is_readable](msg::Message& msg, size_t size) {
              EXPECT_EQ(size, msg::mediator::Status().size());
              *is_readable = false;
              auto status = dynamic_cast<msg::mediator::Status*>(&msg);
              EXPECT_NE(status, nullptr);
              if (status) {
                status->error = should_fail ? -1 : 0;
                return true;
              }
              return false;
            }));
  }

  void connect_to_rixhub(const Endpoint& rixhub_endpoint) {
    EXPECT_CALL(*socket_, connect)
        .Times(1)
        .WillOnce(::testing::Invoke([rixhub_endpoint](const Endpoint& endpoint) {
          EXPECT_EQ(endpoint.address, rixhub_endpoint.address);
          EXPECT_EQ(endpoint.port, rixhub_endpoint.port);
          return true;
        }));
  }
};

} // namespace rix