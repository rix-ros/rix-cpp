#pragma once

#include "rix/core/common.hpp"
#include "rix/ipc/generic_socket.hpp"
#include "rix/ipc/mock_socket.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/SrvInfo.hpp"
#include "rix/msg/mediator/SrvRequest.hpp"
#include "rix/msg/mediator/SrvResponse.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/mediator/SubInfo.hpp"
#include "rix/msg/standard/Void.hpp"
#include <gmock/gmock.h>

extern std::vector<std::shared_ptr<rix::MockSocket>> sockets;
extern int socket_index;

std::shared_ptr<rix::GenericSocket> mock_create_socket() { return sockets[socket_index++]; }

void init_med_server_socket(std::shared_ptr<rix::MockSocket> socket, const rix::Endpoint &rixhub_endpoint,
                            const rix::Endpoint &rixhub_bound_endpoint, int expected_spin_count) {
  EXPECT_CALL(*socket, set_reuse_address(true)).Times(1);
  EXPECT_CALL(*socket, bind)
      .With(::testing::Args<0>(::testing::Truly([&rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, listen).Times(1);
  EXPECT_CALL(*socket, local_endpoint).Times(1).WillOnce(::testing::Return(rixhub_bound_endpoint));
  EXPECT_CALL(*socket, wait_exception).Times(1);
  EXPECT_CALL(*socket, wait_readable).Times(expected_spin_count).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*socket, accept).Times(expected_spin_count).WillRepeatedly(::testing::Invoke([](rix::Endpoint &ep) {
    ep = rix::Endpoint("127.0.0.1", 1234);
    return mock_create_socket();
  }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_pub_register_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                  const rix::msg::mediator::PubInfo &pub_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([pub_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = pub_info.size();
          op->opcode = rix::OPCODE::PUB_REGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([pub_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::PubInfo *>(&msg);
        if (response) {
          *response = pub_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([pub_info, error](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::STATUS_RESPONSE);
        auto status = dynamic_cast<const rix::msg::mediator::Status *>(&msg);
        if (status) {
          EXPECT_EQ(status->id, pub_info.id);
          if (!error) {
            EXPECT_EQ(status->error, 0);
          } else {
            EXPECT_NE(status->error, 0);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_sub_register_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                  const rix::msg::mediator::SubInfo &sub_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([sub_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = sub_info.size();
          op->opcode = rix::OPCODE::SUB_REGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([sub_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SubInfo *>(&msg);
        if (response) {
          *response = sub_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([sub_info, error](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::STATUS_RESPONSE);
        auto status = dynamic_cast<const rix::msg::mediator::Status *>(&msg);
        if (status) {
          EXPECT_EQ(status->id, sub_info.id);
          if (!error) {
            EXPECT_EQ(status->error, 0);
          } else {
            EXPECT_NE(status->error, 0);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_srv_register_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                  const rix::msg::mediator::SrvInfo &srv_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([srv_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = srv_info.size();
          op->opcode = rix::OPCODE::SRV_REGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([srv_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SrvInfo *>(&msg);
        if (response) {
          *response = srv_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([srv_info, error](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::STATUS_RESPONSE);
        auto status = dynamic_cast<const rix::msg::mediator::Status *>(&msg);
        if (status) {
          EXPECT_EQ(status->id, srv_info.id);
          if (!error) {
            EXPECT_EQ(status->error, 0);
          } else {
            EXPECT_NE(status->error, 0);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_node_register_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                   const rix::msg::mediator::NodeInfo &node_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([node_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = node_info.size();
          op->opcode = rix::OPCODE::NODE_REGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([node_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::NodeInfo *>(&msg);
        if (response) {
          *response = node_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([node_info, error](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::STATUS_RESPONSE);
        auto status = dynamic_cast<const rix::msg::mediator::Status *>(&msg);
        if (status) {
          EXPECT_EQ(status->id, node_info.id);
          if (!error) {
            EXPECT_EQ(status->error, 0);
          } else {
            EXPECT_NE(status->error, 0);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_srvcli_request_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                    const rix::msg::mediator::SrvRequest &srv_request,
                                    const rix::msg::mediator::SrvResponse &srv_response) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([srv_request](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = srv_request.size();
          op->opcode = rix::OPCODE::SRV_REQUEST;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([srv_request](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SrvRequest *>(&msg);
        if (response) {
          *response = srv_request;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([srv_response](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::SRV_RESPONSE);
        auto response = dynamic_cast<const rix::msg::mediator::SrvResponse *>(&msg);
        if (response) {
          if (srv_response.error == 0) {
            EXPECT_EQ(response->error, 0);
            EXPECT_EQ(response->srv_info.name, srv_response.srv_info.name);
            EXPECT_EQ(response->srv_info.id, srv_response.srv_info.id);
            EXPECT_EQ(response->srv_info.request_hash, srv_response.srv_info.request_hash);
            EXPECT_EQ(response->srv_info.response_hash, srv_response.srv_info.response_hash);
            EXPECT_EQ(response->srv_info.endpoint.address, srv_response.srv_info.endpoint.address);
            EXPECT_EQ(response->srv_info.endpoint.port, srv_response.srv_info.endpoint.port);
          } else {
            EXPECT_NE(response->error, 0);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_sys_info_request_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                      const rix::msg::mediator::SystemInfo &sys_info, uint64_t node_id) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = 0;
          op->opcode = rix::OPCODE::SYSTEM_GET_REQUEST;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([node_id](rix::msg::Message &msg, size_t len) {
        auto request = dynamic_cast<rix::msg::standard::UInt64 *>(&msg);
        if (request) {
          request->data = node_id;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([sys_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::SYSTEM_GET_RESPONSE);
        auto info = dynamic_cast<const rix::msg::mediator::SystemInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->publishers.size(), sys_info.publishers.size());
          for (int i = 0; i < info->publishers.size(); ++i) {
            EXPECT_EQ(info->publishers[i].id, sys_info.publishers[i].id);
            EXPECT_EQ(info->publishers[i].node_id, sys_info.publishers[i].node_id);
            EXPECT_EQ(info->publishers[i].topic_info.name, sys_info.publishers[i].topic_info.name);
            EXPECT_EQ(info->publishers[i].topic_info.message_hash, sys_info.publishers[i].topic_info.message_hash);
            EXPECT_EQ(info->publishers[i].endpoint.address, sys_info.publishers[i].endpoint.address);
            EXPECT_EQ(info->publishers[i].endpoint.port, sys_info.publishers[i].endpoint.port);
          }
          EXPECT_EQ(info->subscribers.size(), sys_info.subscribers.size());
          for (int i = 0; i < info->subscribers.size(); ++i) {
            EXPECT_EQ(info->subscribers[i].id, sys_info.subscribers[i].id);
            EXPECT_EQ(info->subscribers[i].node_id, sys_info.subscribers[i].node_id);
            EXPECT_EQ(info->subscribers[i].topic_info.name, sys_info.subscribers[i].topic_info.name);
            EXPECT_EQ(info->subscribers[i].topic_info.message_hash, sys_info.subscribers[i].topic_info.message_hash);
            EXPECT_EQ(info->subscribers[i].endpoint.address, sys_info.subscribers[i].endpoint.address);
            EXPECT_EQ(info->subscribers[i].endpoint.port, sys_info.subscribers[i].endpoint.port);
          }
          EXPECT_EQ(info->services.size(), sys_info.services.size());
          for (int i = 0; i < info->services.size(); ++i) {
            EXPECT_EQ(info->services[i].id, sys_info.services[i].id);
            EXPECT_EQ(info->services[i].node_id, sys_info.services[i].node_id);
            EXPECT_EQ(info->services[i].name, sys_info.services[i].name);
            EXPECT_EQ(info->services[i].request_hash, sys_info.services[i].request_hash);
            EXPECT_EQ(info->services[i].response_hash, sys_info.services[i].response_hash);
            EXPECT_EQ(info->services[i].endpoint.address, sys_info.services[i].endpoint.address);
            EXPECT_EQ(info->services[i].endpoint.port, sys_info.services[i].endpoint.port);
          }
          EXPECT_EQ(info->nodes.size(), sys_info.nodes.size());
          for (int i = 0; i < info->nodes.size(); ++i) {
            EXPECT_EQ(info->nodes[i].id, sys_info.nodes[i].id);
            EXPECT_EQ(info->nodes[i].name, sys_info.nodes[i].name);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_param_set_request_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                       const rix::msg::mediator::ParamInfo &param_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([param_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = param_info.size();
          op->opcode = rix::OPCODE::PARAM_SET_REQUEST;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([param_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::ParamInfo *>(&msg);
        if (response) {
          *response = param_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([param_info, error](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::STATUS_RESPONSE);
        auto status = dynamic_cast<const rix::msg::mediator::Status *>(&msg);
        if (status) {
          EXPECT_EQ(status->id, param_info.id);
          if (!error) {
            EXPECT_EQ(status->error, 0);
          } else {
            EXPECT_NE(status->error, 0);
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_param_get_request_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                       const rix::msg::mediator::ParamInfo &param_get_request,
                                       const rix::msg::mediator::ParamInfo &param_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([param_get_request](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = param_get_request.size();
          op->opcode = rix::OPCODE::PARAM_GET_REQUEST;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([param_get_request](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::ParamInfo *>(&msg);
        if (response) {
          *response = param_get_request;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([param_get_request, param_info, error](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::OPCODE::PARAM_GET_RESPONSE);
        auto info = dynamic_cast<const rix::msg::mediator::ParamInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->id, param_get_request.id);
          if (!error) {
            EXPECT_EQ(info->name, param_info.name);
            EXPECT_EQ(info->message_hash, param_info.message_hash);
            EXPECT_EQ(info->data, param_info.data);
          } else {
            EXPECT_TRUE(info->data.empty());
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_pub_deregister_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                    const rix::msg::mediator::PubInfo &pub_info) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([pub_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = pub_info.size();
          op->opcode = rix::OPCODE::PUB_DEREGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([pub_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::PubInfo *>(&msg);
        if (response) {
          *response = pub_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_sub_deregister_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                    const rix::msg::mediator::SubInfo &sub_info) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([sub_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = sub_info.size();
          op->opcode = rix::OPCODE::SUB_DEREGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([sub_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SubInfo *>(&msg);
        if (response) {
          *response = sub_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_srv_deregister_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                    const rix::msg::mediator::SrvInfo &srv_info) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([srv_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = srv_info.size();
          op->opcode = rix::OPCODE::SRV_DEREGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([srv_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SrvInfo *>(&msg);
        if (response) {
          *response = srv_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_node_deregister_socket_med(std::shared_ptr<rix::MockSocket> socket,
                                     const rix::msg::mediator::NodeInfo &node_info) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([node_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = node_info.size();
          op->opcode = rix::OPCODE::NODE_DEREGISTER;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([node_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::NodeInfo *>(&msg);
        if (response) {
          *response = node_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}