#pragma once

#include <gmock/gmock.h>

#include "rix/core/common.hpp"
#include "rix/test/mock_socket.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/Status.hpp"

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

  // Configure as a node registration socket
  SocketBuilder& as_node_register(const Endpoint& rixhub_endpoint, bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message).Times(1).WillOnce(::testing::Invoke([](uint8_t, const msg::Message& msg) {
      auto info = dynamic_cast<const msg::mediator::NodeInfo*>(&msg);
      EXPECT_NE(info, nullptr);
      if (info) {
        EXPECT_GT(info->id, 0);
        EXPECT_FALSE(info->name.empty());
        return true;
      }
      return false;
    }));

    expect_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a node deregistration socket
  SocketBuilder& as_node_deregister(const Endpoint& rixhub_endpoint) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message).Times(1).WillOnce(::testing::Invoke([](uint8_t, const msg::Message& msg) {
      auto info = dynamic_cast<const msg::mediator::NodeInfo*>(&msg);
      EXPECT_NE(info, nullptr);
      if (info) {
        EXPECT_GT(info->id, 0);
        EXPECT_FALSE(info->name.empty());
        return true;
      }
      return false;
    }));

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a publisher registration socket
  SocketBuilder&
  as_pub_register(const Endpoint& rixhub_endpoint, const msg::mediator::PubInfo& pub_info, bool should_fail = false) {
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
  SocketBuilder& as_pub_deregister(const Endpoint& rixhub_endpoint, const msg::mediator::PubInfo& pub_info) {
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
  SocketBuilder&
  as_sub_register(const Endpoint& rixhub_endpoint, const msg::mediator::SubInfo& sub_info, bool should_fail = false) {
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
  SocketBuilder& as_sub_deregister(const Endpoint& rixhub_endpoint, const msg::mediator::SubInfo& sub_info) {
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
  SocketBuilder&
  as_srv_register(const Endpoint& rixhub_endpoint, const msg::mediator::SrvInfo& srv_info, bool should_fail = false) {
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
  SocketBuilder& as_srv_deregister(const Endpoint& rixhub_endpoint, const msg::mediator::SrvInfo& srv_info) {
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
        .WillOnce(::testing::Invoke([should_fail, is_readable, srv_res](msg::Message& msg, size_t size) {
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
  SocketBuilder&
  as_param_set(const Endpoint& rixhub_endpoint, const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([param_info](uint8_t opcode, const msg::Message& msg) {
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
  SocketBuilder&
  as_param_get(const Endpoint& rixhub_endpoint, const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    connect_to_rixhub(rixhub_endpoint);

    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([param_info](uint8_t opcode, const msg::Message& msg) {
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
        .WillOnce(::testing::Invoke([param_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::PARAM_GET_RESPONSE;
            op->len = param_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([should_fail, is_readable, param_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, param_info.size());
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
              info->data = param_info.data;
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
        .WillOnce(::testing::Invoke([should_fail, is_readable, sys_info](msg::Message& msg, size_t size) {
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

    EXPECT_CALL(*socket_, close()).Times(1);

    return *this;
  }

  // Configure as a mediator socket for node registration
  SocketBuilder& as_med_node_register(const msg::mediator::NodeInfo& node_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([node_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::NODE_REGISTER;
            op->len = node_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, node_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, node_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::NodeInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->name = node_info.name;
            info->id = node_info.id;
            info->endpoint.address = node_info.endpoint.address;
            info->endpoint.port = node_info.endpoint.port;
            return true;
          }
          return false;
        }));
    send_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for node deregistration
  SocketBuilder& as_med_node_deregister(const msg::mediator::NodeInfo& node_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([node_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::NODE_DEREGISTER;
            op->len = node_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, node_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, node_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::NodeInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->name = node_info.name;
            info->id = node_info.id;
            info->endpoint.address = node_info.endpoint.address;
            info->endpoint.port = node_info.endpoint.port;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for pub registration
  SocketBuilder& as_med_pub_register(const msg::mediator::PubInfo& pub_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([pub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::PUB_REGISTER;
            op->len = pub_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, pub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, pub_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::PubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = pub_info.id;
            info->node_id = pub_info.node_id;
            info->topic_info.name = pub_info.topic_info.name;
            info->topic_info.message_hash = pub_info.topic_info.message_hash;
            info->endpoint.address = pub_info.endpoint.address;
            info->endpoint.port = pub_info.endpoint.port;
            return true;
          }
          return false;
        }));
    send_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for pub deregistration
  SocketBuilder& as_med_pub_deregister(const msg::mediator::PubInfo& pub_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([pub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::PUB_DEREGISTER;
            op->len = pub_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, pub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, pub_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::PubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = pub_info.id;
            info->node_id = pub_info.node_id;
            info->topic_info.name = pub_info.topic_info.name;
            info->topic_info.message_hash = pub_info.topic_info.message_hash;
            info->endpoint.address = pub_info.endpoint.address;
            info->endpoint.port = pub_info.endpoint.port;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for sub registration
  SocketBuilder& as_med_sub_register(const msg::mediator::SubInfo& sub_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([sub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SUB_REGISTER;
            op->len = sub_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, sub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, sub_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::SubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = sub_info.id;
            info->node_id = sub_info.node_id;
            info->topic_info.name = sub_info.topic_info.name;
            info->topic_info.message_hash = sub_info.topic_info.message_hash;
            info->endpoint.address = sub_info.endpoint.address;
            info->endpoint.port = sub_info.endpoint.port;
            return true;
          }
          return false;
        }));
    send_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for sub deregistration
  SocketBuilder& as_med_sub_deregister(const msg::mediator::SubInfo& sub_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([sub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SUB_DEREGISTER;
            op->len = sub_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, sub_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, sub_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::SubInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = sub_info.id;
            info->node_id = sub_info.node_id;
            info->topic_info.name = sub_info.topic_info.name;
            info->topic_info.message_hash = sub_info.topic_info.message_hash;
            info->endpoint.address = sub_info.endpoint.address;
            info->endpoint.port = sub_info.endpoint.port;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for srv registration
  SocketBuilder& as_med_srv_register(const msg::mediator::SrvInfo& srv_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([srv_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SRV_REGISTER;
            op->len = srv_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, srv_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, srv_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::SrvInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = srv_info.id;
            info->node_id = srv_info.node_id;
            info->name = srv_info.name;
            info->request_hash = srv_info.request_hash;
            info->response_hash = srv_info.response_hash;
            info->endpoint.address = srv_info.endpoint.address;
            info->endpoint.port = srv_info.endpoint.port;
            return true;
          }
          return false;
        }));
    send_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for srv deregistration
  SocketBuilder& as_med_srv_deregister(const msg::mediator::SrvInfo& srv_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([srv_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SRV_DEREGISTER;
            op->len = srv_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, srv_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, srv_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::SrvInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = srv_info.id;
            info->node_id = srv_info.node_id;
            info->name = srv_info.name;
            info->request_hash = srv_info.request_hash;
            info->response_hash = srv_info.response_hash;
            info->endpoint.address = srv_info.endpoint.address;
            info->endpoint.port = srv_info.endpoint.port;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as mediator socket for service client request
  SocketBuilder& as_med_srv_cli_request(const msg::mediator::SrvRequest& srv_req,
                                        const msg::mediator::SrvResponse& srv_res,
                                        bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([srv_req](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::SRV_REQUEST;
            op->len = srv_req.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, srv_req](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, srv_req.size());
          *is_readable = false;
          auto req = dynamic_cast<msg::mediator::SrvRequest*>(&msg);
          EXPECT_NE(req, nullptr);
          if (req) {
            req->node_id = srv_req.node_id;
            req->name = srv_req.name;
            req->request_hash = srv_req.request_hash;
            req->response_hash = srv_req.response_hash;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([should_fail, srv_res](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SRV_RESPONSE);
          auto res = dynamic_cast<const msg::mediator::SrvResponse*>(&msg);
          EXPECT_NE(res, nullptr);
          if (res) {
            if (should_fail) {
              EXPECT_NE(res->error, 0);
            } else {
              EXPECT_EQ(res->error, 0);
              EXPECT_EQ(res->srv_info.id, srv_res.srv_info.id);
              EXPECT_EQ(res->srv_info.node_id, srv_res.srv_info.node_id);
              EXPECT_EQ(res->srv_info.name, srv_res.srv_info.name);
              EXPECT_EQ(res->srv_info.request_hash, srv_res.srv_info.request_hash);
              EXPECT_EQ(res->srv_info.response_hash, srv_res.srv_info.response_hash);
              EXPECT_EQ(res->srv_info.endpoint.address, srv_res.srv_info.endpoint.address);
              EXPECT_EQ(res->srv_info.endpoint.port, srv_res.srv_info.endpoint.port);
            }
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for parameter get request
  SocketBuilder& as_med_param_get(const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([param_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::PARAM_GET_REQUEST;
            op->len = param_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, param_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, param_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::ParamInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = param_info.id;
            info->name = param_info.name;
            info->message_hash = param_info.message_hash;
            info->data.clear();
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([should_fail, param_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::PARAM_GET_RESPONSE);
          auto info = dynamic_cast<const msg::mediator::ParamInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            EXPECT_EQ(info->name, param_info.name);
            EXPECT_EQ(info->message_hash, param_info.message_hash);
            if (should_fail) {
              // Simulate failure by not setting the value
              EXPECT_TRUE(info->data.empty());
            } else {
              // Check that value data matches the serialized message
              EXPECT_FALSE(info->data.empty());
              EXPECT_EQ(info->data.size(), param_info.data.size());
              EXPECT_EQ(info->data, param_info.data);
            }
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for parameter set request
  SocketBuilder& as_med_param_set(const msg::mediator::ParamInfo& param_info, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke([param_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          EXPECT_NE(op, nullptr);
          if (op) {
            op->opcode = OPCODE::PARAM_SET_REQUEST;
            op->len = param_info.size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, param_info](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, param_info.size());
          *is_readable = false;
          auto info = dynamic_cast<msg::mediator::ParamInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->id = param_info.id;
            info->name = param_info.name;
            info->message_hash = param_info.message_hash;
            info->data = param_info.data;
            return true;
          }
          return false;
        }));
    send_status_response(should_fail);
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for system info request
  SocketBuilder&
  as_med_sys_info_request(const msg::mediator::SystemInfo& sys_info, uint64_t node_id, bool should_fail = false) {
    auto is_readable = std::make_shared<bool>(true);
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
            op->opcode = OPCODE::SYSTEM_GET_REQUEST;
            op->len = msg::standard::UInt64().size();
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([is_readable, node_id](msg::Message& msg, size_t size) {
          EXPECT_EQ(size, msg::standard::UInt64().size());
          *is_readable = false;
          auto info = dynamic_cast<msg::standard::UInt64*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            info->data = node_id;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([should_fail, sys_info](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SYSTEM_GET_RESPONSE);
          auto info = dynamic_cast<const msg::mediator::SystemInfo*>(&msg);
          EXPECT_NE(info, nullptr);
          if (info) {
            if (should_fail) {
              // Simulate failure by not setting the value
              EXPECT_TRUE(info->nodes.empty());
              EXPECT_TRUE(info->publishers.empty());
              EXPECT_TRUE(info->subscribers.empty());
              EXPECT_TRUE(info->services.empty());
              EXPECT_TRUE(info->actions.empty());
              EXPECT_TRUE(info->topics.empty());
            } else {
              EXPECT_EQ(info->nodes.size(), sys_info.nodes.size());
              EXPECT_EQ(info->publishers.size(), sys_info.publishers.size());
              EXPECT_EQ(info->subscribers.size(), sys_info.subscribers.size());
              EXPECT_EQ(info->services.size(), sys_info.services.size());
              EXPECT_EQ(info->topics.size(), sys_info.topics.size());
              for (size_t i = 0; i < info->nodes.size(); i++) {
                EXPECT_EQ(info->nodes[i].id, sys_info.nodes[i].id);
                EXPECT_EQ(info->nodes[i].name, sys_info.nodes[i].name);
                EXPECT_EQ(info->nodes[i].endpoint.address, sys_info.nodes[i].endpoint.address);
                EXPECT_EQ(info->nodes[i].endpoint.port, sys_info.nodes[i].endpoint.port);
              }
              for (size_t i = 0; i < info->publishers.size(); i++) {
                EXPECT_EQ(info->publishers[i].id, sys_info.publishers[i].id);
                EXPECT_EQ(info->publishers[i].node_id, sys_info.publishers[i].node_id);
                EXPECT_EQ(info->publishers[i].topic_info.name, sys_info.publishers[i].topic_info.name);
                EXPECT_EQ(info->publishers[i].topic_info.message_hash, sys_info.publishers[i].topic_info.message_hash);
                EXPECT_EQ(info->publishers[i].endpoint.address, sys_info.publishers[i].endpoint.address);
                EXPECT_EQ(info->publishers[i].endpoint.port, sys_info.publishers[i].endpoint.port);
              }
              for (size_t i = 0; i < info->subscribers.size(); i++) {
                EXPECT_EQ(info->subscribers[i].id, sys_info.subscribers[i].id);
                EXPECT_EQ(info->subscribers[i].node_id, sys_info.subscribers[i].node_id);
                EXPECT_EQ(info->subscribers[i].topic_info.name, sys_info.subscribers[i].topic_info.name);
                EXPECT_EQ(info->subscribers[i].topic_info.message_hash,
                          sys_info.subscribers[i].topic_info.message_hash);
                EXPECT_EQ(info->subscribers[i].endpoint.address, sys_info.subscribers[i].endpoint.address);
                EXPECT_EQ(info->subscribers[i].endpoint.port, sys_info.subscribers[i].endpoint.port);
              }
              for (size_t i = 0; i < info->services.size(); i++) {
                EXPECT_EQ(info->services[i].id, sys_info.services[i].id);
                EXPECT_EQ(info->services[i].node_id, sys_info.services[i].node_id);
                EXPECT_EQ(info->services[i].name, sys_info.services[i].name);
                EXPECT_EQ(info->services[i].request_hash, sys_info.services[i].request_hash);
                EXPECT_EQ(info->services[i].response_hash, sys_info.services[i].response_hash);
                EXPECT_EQ(info->services[i].endpoint.address, sys_info.services[i].endpoint.address);
                EXPECT_EQ(info->services[i].endpoint.port, sys_info.services[i].endpoint.port);
              }
            }
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a mediator socket for sub notification
  SocketBuilder& as_med_sub_notify(const msg::mediator::SubNotify& sub_notify) {
    EXPECT_CALL(*socket_, connect(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*socket_, send_message(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([sub_notify](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SUB_NOTIFY);
          auto notify = dynamic_cast<const msg::mediator::SubNotify*>(&msg);
          EXPECT_NE(notify, nullptr);
          if (notify) {
            EXPECT_EQ(notify->id, sub_notify.id);
            for (size_t i = 0; i < notify->publishers.size(); i++) {
              EXPECT_EQ(notify->publishers[i].id, sub_notify.publishers[i].id);
              EXPECT_EQ(notify->publishers[i].node_id, sub_notify.publishers[i].node_id);
              EXPECT_EQ(notify->publishers[i].topic_info.name, sub_notify.publishers[i].topic_info.name);
              EXPECT_EQ(notify->publishers[i].topic_info.message_hash,
                        sub_notify.publishers[i].topic_info.message_hash);
              EXPECT_EQ(notify->publishers[i].endpoint.address, sub_notify.publishers[i].endpoint.address);
              EXPECT_EQ(notify->publishers[i].endpoint.port, sub_notify.publishers[i].endpoint.port);
            }
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a publisher connection socket (accepts and sends messages)
  template <typename TMsg> SocketBuilder& as_pub_connection(const std::vector<std::shared_ptr<TMsg>>& messages) {
    int send_count = static_cast<int>(messages.size());
    auto send_index = std::make_shared<int>(0);
    EXPECT_CALL(*socket_, wait_writable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([send_index, send_count](auto) { return *send_index < send_count; }));
    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, send_message)
        .Times(send_count)
        .WillRepeatedly(
            ::testing::Invoke([send_index, messages, socket, notify](uint8_t opcode, const msg::Message& msg) {
              ++(*send_index);
              EXPECT_EQ(opcode, OPCODE::PUB_MESSAGE);
              auto message = dynamic_cast<const TMsg*>(&msg);
              EXPECT_NE(message, nullptr);
              if (message) {
                EXPECT_EQ(msg.size(), messages.at(*send_index - 1)->size());
                if (notify) {
                  if (auto s = socket.lock()) {
                    s->notify_operation_complete();
                  }
                }
                return true;
              }
              return false;
            }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a service connection socket (accepts and handles request/response)
  template <typename TRequest, typename TResponse>
  SocketBuilder& as_srv_connection(std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    static_assert(std::is_base_of<msg::Message, TRequest>::value, "TRequest must be derived from msg::Message");
    static_assert(std::is_base_of<msg::Message, TResponse>::value, "TResponse must be derived from msg::Message");
    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;

    // Service needs to wait for readable data before processing request
    auto is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));

    EXPECT_CALL(*socket_, recv_message)
        .Times(2)
        .WillOnce(::testing::Invoke([request](msg::Message& msg, size_t len) {
          EXPECT_EQ(len, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          if (op) {
            op->len = request->size();
            op->opcode = OPCODE::SRV_REQUEST_MESSAGE;
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([request, is_readable](msg::Message& msg, size_t len) {
          EXPECT_EQ(len, request->size());
          auto req = dynamic_cast<TRequest*>(&msg);
          if (req) {
            *req = *request;
            *is_readable = false; // After reading the request, no more data
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([response, socket, notify](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SRV_RESPONSE_MESSAGE);
          auto resp = dynamic_cast<const TResponse*>(&msg);
          if (resp) {
            EXPECT_EQ(resp->size(), response->size());
            if (notify) {
              if (auto s = socket.lock()) {
                s->notify_operation_complete();
              }
            }
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a subscriber client socket (connects and receives messages)
  template <typename TMsg>
  SocketBuilder& as_sub_client(const Endpoint& endpoint, const std::vector<std::shared_ptr<TMsg>>& messages) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");
    EXPECT_CALL(*socket_, set_blocking(false)).Times(1);
    EXPECT_CALL(*socket_, connect(endpoint)).Times(1).WillOnce(::testing::Return(true));

    int recv_count = static_cast<int>(messages.size() * 2); // Each message requires 2 recv calls
    auto recv_index = std::make_shared<int>(0);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([recv_index, recv_count]() { return *recv_index < recv_count; }));
    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, recv_message)
        .Times(::testing::AtLeast(recv_count))
        .WillRepeatedly(::testing::Invoke([recv_index, messages, socket, notify](msg::Message& msg, size_t len) {
          int idx = *recv_index;
          if (idx % 2 == 0) {
            // Even: Operation
            EXPECT_EQ(len, msg::mediator::Operation().size());
            auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
            if (op) {
              op->len = messages[idx / 2]->size();
              op->opcode = OPCODE::PUB_MESSAGE;
              ++(*recv_index);
              return true;
            }
          } else {
            // Odd: TMsg
            if (idx / 2 < static_cast<int>(messages.size())) {
              auto message = dynamic_cast<TMsg*>(&msg);
              if (message) {
                EXPECT_EQ(len, messages.at(idx / 2)->size());
                *message = *(messages.at(idx / 2));
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
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a service client connection socket (connects, sends request, receives
  // response)
  template <typename TRequest, typename TResponse>
  SocketBuilder&
  as_srv_cli_client(const Endpoint& endpoint, std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    static_assert(std::is_base_of<msg::Message, TRequest>::value, "TRequest must be derived from msg::Message");
    static_assert(std::is_base_of<msg::Message, TResponse>::value, "TResponse must be derived from msg::Message");
    std::weak_ptr<MockSocket> socket = socket_;
    auto notify = enable_notifications_;
    EXPECT_CALL(*socket_, connect(endpoint)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(*socket_, wait_writable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Return(true));

    std::shared_ptr<bool> is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([request](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::SRV_REQUEST_MESSAGE);
          auto info = dynamic_cast<const TRequest*>(&msg);
          if (info) {
            EXPECT_EQ(msg.size(), request->size());
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, recv_message)
        .Times(2)
        .WillOnce(::testing::Invoke([response](msg::Message& msg, size_t len) {
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          if (op) {
            op->len = response->size();
            op->opcode = OPCODE::SRV_RESPONSE_MESSAGE;
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([response, socket, notify, is_readable](msg::Message& msg, size_t len) {
          auto resp = dynamic_cast<TResponse*>(&msg);
          *is_readable = false; // After reading the response, no more data
          if (resp) {
            *resp = *response;
            if (notify) {
              if (auto s = socket.lock()) {
                s->notify_operation_complete();
              }
            }
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

  // Configure as a subscriber notification connection socket
  SocketBuilder& as_sub_connection(const msg::mediator::SubNotify& notify) {
    std::shared_ptr<bool> is_readable = std::make_shared<bool>(true);
    EXPECT_CALL(*socket_, wait_readable(::testing::_))
        .Times(::testing::AtLeast(0))
        .WillRepeatedly(::testing::Invoke([is_readable]() { return *is_readable; }));
    EXPECT_CALL(*socket_, recv_message)
        .Times(2)
        .WillOnce(::testing::Invoke([notify](msg::Message& msg, size_t len) {
          EXPECT_EQ(len, msg::mediator::Operation().size());
          auto op = dynamic_cast<msg::mediator::Operation*>(&msg);
          if (op) {
            op->len = notify.size();
            op->opcode = OPCODE::SUB_NOTIFY;
            return true;
          }
          return false;
        }))
        .WillOnce(::testing::Invoke([notify, is_readable](msg::Message& msg, size_t len) {
          EXPECT_EQ(len, notify.size());
          *is_readable = false; // After reading the response, no more data
          auto response = dynamic_cast<msg::mediator::SubNotify*>(&msg);
          if (response) {
            *response = notify;
            return true;
          }
          return false;
        }));
    EXPECT_CALL(*socket_, close()).Times(1);
    return *this;
  }

private:
  std::shared_ptr<MockSocket> socket_;
  bool enable_notifications_;

  // Helper to notify if notifications are enabled
  void maybe_notify() {
    if (enable_notifications_) {
      socket_->notify_operation_complete();
    }
  }

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
        .WillOnce(::testing::Invoke([should_fail, is_readable](msg::Message& msg, size_t size) {
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

  void send_status_response(bool should_fail) {
    EXPECT_CALL(*socket_, send_message)
        .Times(1)
        .WillOnce(::testing::Invoke([should_fail](uint8_t opcode, const msg::Message& msg) {
          EXPECT_EQ(opcode, OPCODE::STATUS_RESPONSE);
          auto status = dynamic_cast<const msg::mediator::Status*>(&msg);
          EXPECT_NE(status, nullptr);
          if (status) {
            if (should_fail) {
              EXPECT_NE(status->error, 0);
            } else {
              EXPECT_EQ(status->error, 0);
            }
            return true;
          }
          return false;
        }));
  }

  void connect_to_rixhub(const Endpoint& rixhub_endpoint) {
    EXPECT_CALL(*socket_, connect).Times(1).WillOnce(::testing::Invoke([rixhub_endpoint](const Endpoint& endpoint) {
      EXPECT_EQ(endpoint.address, rixhub_endpoint.address);
      EXPECT_EQ(endpoint.port, rixhub_endpoint.port);
      return true;
    }));
  }
};

} // namespace rix