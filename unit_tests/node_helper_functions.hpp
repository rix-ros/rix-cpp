#pragma once

#include <gmock/gmock.h>

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

extern std::vector<std::shared_ptr<rix::ipc::MockSocket>> sockets;
extern int socket_index;

std::shared_ptr<rix::ipc::GenericSocket> mock_create_socket() { return sockets[socket_index++]; }

void init_node_register_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                               const rix::msg::mediator::NodeInfo &node_info, uint64_t &node_id, uint8_t op,
                               bool error) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([op, node_info, &node_id](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, op);
        auto info = dynamic_cast<const rix::msg::mediator::NodeInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->name, node_info.name);
          EXPECT_GT(info->id, 0);
          node_id = info->id;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = rix::msg::mediator::Status().size();
          op->opcode = rix::core::OPCODE::STATUS_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([error](rix::msg::Message &msg, size_t len) {
        auto status = dynamic_cast<rix::msg::mediator::Status *>(&msg);
        if (status) {
          if (error)
            status->error = -1;
          else
            status->error = 0;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_node_deregister_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                                 const rix::ipc::Endpoint &rixhub_endpoint,
                                 const rix::msg::mediator::NodeInfo &node_info) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([node_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::NODE_DEREGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::NodeInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->name, node_info.name);
          EXPECT_GT(info->id, 0);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_server_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &endpoint,
                        const rix::ipc::Endpoint &bound_endpoint) {
  EXPECT_CALL(*socket, set_reuse_address(true)).Times(1);
  EXPECT_CALL(*socket, bind)
      .With(::testing::Args<0>(::testing::Truly([&endpoint](const auto &args) {
        return std::get<0>(args).address == endpoint.address && std::get<0>(args).port == endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, listen).Times(1);
  EXPECT_CALL(*socket, local_endpoint).Times(1).WillOnce(::testing::Return(bound_endpoint));
  EXPECT_CALL(*socket, wait_exception).Times(1);
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_pub_register_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                              const rix::msg::mediator::PubInfo &pub_info, uint64_t &node_id, bool error) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id, pub_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::PUB_REGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::PubInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->topic_info.name, pub_info.topic_info.name);
          EXPECT_EQ(info->topic_info.message_hash, pub_info.topic_info.message_hash);
          EXPECT_EQ(info->endpoint.address, pub_info.endpoint.address);
          EXPECT_EQ(info->endpoint.port, pub_info.endpoint.port);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = rix::msg::mediator::Status().size();
          op->opcode = rix::core::OPCODE::STATUS_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([error](rix::msg::Message &msg, size_t len) {
        auto status = dynamic_cast<rix::msg::mediator::Status *>(&msg);
        if (status) {
          if (error)
            status->error = -1;
          else
            status->error = 0;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_pub_deregister_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                                const rix::msg::mediator::PubInfo &pub_info, uint64_t &node_id) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id, pub_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::PUB_DEREGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::PubInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->topic_info.name, pub_info.topic_info.name);
          EXPECT_EQ(info->topic_info.message_hash, pub_info.topic_info.message_hash);
          EXPECT_EQ(info->endpoint.address, pub_info.endpoint.address);
          EXPECT_EQ(info->endpoint.port, pub_info.endpoint.port);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_sub_register_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                              const rix::msg::mediator::SubInfo &sub_info, uint64_t &node_id, bool error) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id, sub_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::SUB_REGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::SubInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->topic_info.name, sub_info.topic_info.name);
          EXPECT_EQ(info->topic_info.message_hash, sub_info.topic_info.message_hash);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = rix::msg::mediator::Status().size();
          op->opcode = rix::core::OPCODE::STATUS_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([error](rix::msg::Message &msg, size_t len) {
        auto status = dynamic_cast<rix::msg::mediator::Status *>(&msg);
        if (status) {
          if (error)
            status->error = -1;
          else
            status->error = 0;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_sub_deregister_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                                const rix::msg::mediator::SubInfo &sub_info, uint64_t &node_id) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id, sub_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::SUB_DEREGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::SubInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->topic_info.name, sub_info.topic_info.name);
          EXPECT_EQ(info->topic_info.message_hash, sub_info.topic_info.message_hash);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_srv_register_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                              const rix::msg::mediator::SrvInfo &srv_info, uint64_t &node_id, bool error) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id, srv_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::SRV_REGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::SrvInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->name, srv_info.name);
          EXPECT_EQ(info->request_hash, srv_info.request_hash);
          EXPECT_EQ(info->response_hash, srv_info.response_hash);
          EXPECT_EQ(info->endpoint.address, srv_info.endpoint.address);
          EXPECT_EQ(info->endpoint.port, srv_info.endpoint.port);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = rix::msg::mediator::Status().size();
          op->opcode = rix::core::OPCODE::STATUS_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([error](rix::msg::Message &msg, size_t len) {
        auto status = dynamic_cast<rix::msg::mediator::Status *>(&msg);
        if (status) {
          if (error)
            status->error = -1;
          else
            status->error = 0;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_srv_deregister_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                                const rix::msg::mediator::SrvInfo &srv_info, uint64_t &node_id) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id, srv_info](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::SRV_DEREGISTER);
        auto info = dynamic_cast<const rix::msg::mediator::SrvInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->name, srv_info.name);
          EXPECT_EQ(info->request_hash, srv_info.request_hash);
          EXPECT_EQ(info->response_hash, srv_info.response_hash);
          EXPECT_EQ(info->endpoint.address, srv_info.endpoint.address);
          EXPECT_EQ(info->endpoint.port, srv_info.endpoint.port);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_srvcli_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::ipc::Endpoint &rixhub_endpoint,
                        const rix::msg::mediator::SrvResponse &srv_response, uint64_t &node_id) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::SRV_REQUEST);
        auto info = dynamic_cast<const rix::msg::mediator::SrvRequest *>(&msg);
        if (info) {
          EXPECT_EQ(info->node_id, node_id);
          EXPECT_GT(info->id, 0);
          EXPECT_NE(info->id, node_id);
          EXPECT_EQ(info->name, "test_service");
          EXPECT_EQ(info->request_hash, rix::msg::standard::UInt32().hash());
          EXPECT_EQ(info->response_hash, rix::msg::standard::Time().hash());
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([&node_id, &srv_response](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = srv_response.size();
          op->opcode = rix::core::OPCODE::SRV_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([&srv_response](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SrvResponse *>(&msg);
        if (response) {
          *response = srv_response;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_sys_info_request_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                                  const rix::ipc::Endpoint &rixhub_endpoint,
                                  const rix::msg::mediator::SystemInfo &sys_info, uint64_t &node_id) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([&node_id](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::SYSTEM_GET_REQUEST);
        auto info = dynamic_cast<const rix::msg::standard::UInt64 *>(&msg);
        if (info) {
          EXPECT_EQ(info->data, node_id);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([&node_id, sys_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = sys_info.size();
          op->opcode = rix::core::OPCODE::SYSTEM_GET_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([sys_info](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::SystemInfo *>(&msg);
        if (response) {
          *response = sys_info;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_param_get_request_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                                   const rix::ipc::Endpoint &rixhub_endpoint, rix::msg::mediator::ParamInfo parameter,
                                   uint64_t &node_id, bool error) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([parameter, error, &node_id](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::PARAM_GET_REQUEST);
        auto info = dynamic_cast<const rix::msg::mediator::ParamInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->name, parameter.name);
          EXPECT_EQ(info->id, node_id);
          EXPECT_EQ(info->message_hash, parameter.message_hash);
          EXPECT_TRUE(info->data.empty());
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([parameter](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = parameter.size();
          op->opcode = rix::core::OPCODE::PARAM_GET_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([parameter, &node_id, error](rix::msg::Message &msg, size_t len) {
        auto response = dynamic_cast<rix::msg::mediator::ParamInfo *>(&msg);
        if (response) {
          if (error) {
            response->name = "";
            response->id = node_id;
            response->message_hash = {};
            response->data.clear();
          } else {
            *response = parameter;
            response->id = node_id;
          }
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_param_set_request_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                                   const rix::ipc::Endpoint &rixhub_endpoint, rix::msg::mediator::ParamInfo param_info,
                                   uint64_t &node_id, bool error) {
  EXPECT_CALL(*socket, connect)
      .With(::testing::Args<0>(::testing::Truly([rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*socket, send_message)
      .Times(1)
      .WillOnce(::testing::Invoke([param_info, &node_id](uint8_t opcode, const rix::msg::Message &msg) {
        EXPECT_EQ(opcode, rix::core::OPCODE::PARAM_SET_REQUEST);
        auto info = dynamic_cast<const rix::msg::mediator::ParamInfo *>(&msg);
        if (info) {
          EXPECT_EQ(info->name, param_info.name);
          EXPECT_EQ(info->id, node_id);
          EXPECT_EQ(info->message_hash, param_info.message_hash);
          EXPECT_EQ(info->data, param_info.data);
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([param_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = 0;
          op->opcode = rix::core::OPCODE::STATUS_RESPONSE;
          return true;
        }
        return false;
      }))
      .WillOnce(::testing::Invoke([error](rix::msg::Message &msg, size_t len) {
        auto status = dynamic_cast<rix::msg::mediator::Status *>(&msg);
        if (status) {
          if (error)
            status->error = -1;
          else
            status->error = 0;
          return true;
        }
        return false;
      }));
  EXPECT_CALL(*socket, close()).Times(1);
}

void init_pub_connection_socket(std::shared_ptr<rix::ipc::MockSocket> socket, int times, bool error) {
  EXPECT_CALL(*socket, accept)
      .Times(times)
      .WillRepeatedly(
          ::testing::Invoke([error](rix::ipc::Endpoint &remote_endpoint) -> std::shared_ptr<rix::ipc::GenericSocket> {
            if (error) {
              return nullptr;
            }
            return mock_create_socket();
          }));
}