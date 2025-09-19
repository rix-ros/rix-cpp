#include "rix/core/mediator.hpp"
#include "rix/core/node.hpp"
#include "rix/ipc/mock_socket.hpp"
#include <gtest/gtest.h>

#include "helper_functions.hpp"

std::vector<std::shared_ptr<rix::ipc::MockSocket>> sockets;
int socket_index = 0;

std::shared_ptr<rix::ipc::GenericSocket> mock_create_socket() { return sockets[socket_index++]; }

void init_pub_register_socket(std::shared_ptr<rix::ipc::MockSocket> socket, const rix::msg::mediator::PubInfo &pub_info,
                              bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([pub_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = pub_info.size();
          op->opcode = rix::core::OPCODE::PUB_REGISTER;
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
        EXPECT_EQ(opcode, rix::core::OPCODE::STATUS_RESPONSE);
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

void init_node_register_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                               const rix::msg::mediator::NodeInfo &node_info, bool error) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([node_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = node_info.size();
          op->opcode = rix::core::OPCODE::NODE_REGISTER;
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
        EXPECT_EQ(opcode, rix::core::OPCODE::STATUS_RESPONSE);
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

void init_pub_deregister_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                                const rix::msg::mediator::PubInfo &pub_info) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([pub_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = pub_info.size();
          op->opcode = rix::core::OPCODE::PUB_DEREGISTER;
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

void init_node_deregister_socket(std::shared_ptr<rix::ipc::MockSocket> socket,
                                 const rix::msg::mediator::NodeInfo &node_info) {
  EXPECT_CALL(*socket, recv_message)
      .Times(2)
      .WillOnce(::testing::Invoke([node_info](rix::msg::Message &msg, size_t len) {
        auto op = dynamic_cast<rix::msg::mediator::Operation *>(&msg);
        if (op) {
          op->len = node_info.size();
          op->opcode = rix::core::OPCODE::NODE_DEREGISTER;
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

TEST(RegistrationTests, MediatorRegisterAndDeregisterNode) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Mediator server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register connection 1
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register connection 2
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register connection 3
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister connection 1
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister connection 2
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister connection 3

  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint rixhub_bound_endpoint("127.0.0.1", 8000);

  EXPECT_CALL(*sockets[0], set_reuse_address(true)).Times(1);
  EXPECT_CALL(*sockets[0], bind)
      .With(::testing::Args<0>(::testing::Truly([&rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*sockets[0], listen).Times(1);
  EXPECT_CALL(*sockets[0], local_endpoint).Times(1).WillOnce(::testing::Return(rixhub_bound_endpoint));
  EXPECT_CALL(*sockets[0], wait_exception).Times(1);
  EXPECT_CALL(*sockets[0], wait_readable).Times(6).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*sockets[0], accept).Times(6).WillRepeatedly(::testing::Invoke([](rix::ipc::Endpoint &ep) {
    ep = rix::ipc::Endpoint("127.0.0.1", 1234);
    return mock_create_socket();
  }));
  EXPECT_CALL(*sockets[0], close()).Times(1);

  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_register_socket(sockets[1], node_info, false);

  node_info.name = "test_node_2";
  node_info.id = 2;
  init_node_register_socket(sockets[2], node_info, false);

  node_info.name = "test_node_3";
  node_info.id = 1;
  init_node_register_socket(sockets[3], node_info, true); // Duplicate ID error

  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_deregister_socket(sockets[4], node_info);

  node_info.name = "test_node_2";
  node_info.id = 2;
  init_node_deregister_socket(sockets[5], node_info);

  node_info.name = "test_node_3";
  node_info.id = 3;
  init_node_deregister_socket(sockets[6], node_info); // Never registered

  {
    auto med = rix::core::Mediator(rixhub_endpoint, mock_create_socket);
    med.spin_once(); // Register node 1
    med.spin_once(); // Register node 2
    med.spin_once(); // Fail to register node 3 (a duplicate of node 1)
    med.spin_once(); // Deregister node 1
    med.spin_once(); // Deregister node 2
    med.spin_once(); // Fail to deregister node 3 (never registered)
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, MediatorRegisterAndDeregisterPublisher) {
  // 0  8000 Mediator server
  // 1  8001 Node register connection (id 1)
  // 2  8002 Publisher register connection (topic A, id 2)
  // 3  8003 Publisher register connection (topic B, id 3)
  // 4  8004 Publisher register connection (topic A, id 2, duplicate id)
  // 5  8005 Node register connection (id 4)
  // 6  8006 Publisher register connection (topic A, id 5)
  // 7  8007 Publisher register connection (topic B, id 6)
  // 8  8008 Publisher register connection (topic A, id 7, wrong hash)
  // 9  8009 Publisher register connection (topic B, id 8, node not registered)
  // 10 8001 Node deregister connection (id 1)
  // 11 8005 Node deregister connection (id 4)
  // 12 8002 Publisher deregister connection (topic A, id 2)
  // 13 8003 Publisher deregister connection (topic B, id 3)
  // 14 8006 Publisher deregister connection (topic A, id 5)
  // 15 8009 Publisher deregister connection (topic B, id 8, publisher not registered but attempted)
  // 16 8010 Publisher deregister connection (topic A, id 9, publisher not registered)
  // 17 8007 Publisher deregister connection (topic B, id 6)
  sockets.resize(18, nullptr);
  for (auto &s : sockets) {
    s = std::make_shared<rix::ipc::MockSocket>();
  }

  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint rixhub_bound_endpoint("127.0.0.1", 8000);

  size_t i = 0;

  // 0  8000 Mediator server
  EXPECT_CALL(*sockets[i], set_reuse_address(true)).Times(1);
  EXPECT_CALL(*sockets[i], bind)
      .With(::testing::Args<0>(::testing::Truly([&rixhub_endpoint](const auto &args) {
        return std::get<0>(args).address == rixhub_endpoint.address && std::get<0>(args).port == rixhub_endpoint.port;
      })))
      .Times(1);
  EXPECT_CALL(*sockets[i], listen).Times(1);
  EXPECT_CALL(*sockets[i], local_endpoint).Times(1).WillOnce(::testing::Return(rixhub_bound_endpoint));
  EXPECT_CALL(*sockets[i], wait_exception).Times(1);
  EXPECT_CALL(*sockets[i], wait_readable).Times(17).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*sockets[i], accept).Times(17).WillRepeatedly(::testing::Invoke([](rix::ipc::Endpoint &ep) {
    ep = rix::ipc::Endpoint("127.0.0.1", 1234);
    return mock_create_socket();
  }));
  EXPECT_CALL(*sockets[i], close()).Times(1);
  i++;

  // 1  8001 Node register connection (id 1)
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_register_socket(sockets[i], node_info, false);
  i++;

  // 2  8002 Publisher register connection (topic A, id 2)
  rix::msg::mediator::PubInfo pub_info;
  pub_info.id = 2;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8002;
  init_pub_register_socket(sockets[i], pub_info, false);
  i++;

  // 3  8003 Publisher register connection (topic B, id 3)
  pub_info.id = 3;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8003;
  init_pub_register_socket(sockets[i], pub_info, false);
  i++;

  // 4  8004 Publisher register connection (topic A, id 2, duplicate id)
  pub_info.id = 2; // Duplicate ID
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8004;
  init_pub_register_socket(sockets[i], pub_info, true);
  i++;

  // 5  8005 Node register connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_register_socket(sockets[i], node_info, false);
  i++;

  // 6  8006 Publisher register connection (topic A, id 5)
  pub_info.id = 5;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8006;
  init_pub_register_socket(sockets[i], pub_info, false);
  i++;

  // 7  8007 Publisher register connection (topic B, id 6)
  pub_info.id = 6;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8007;
  init_pub_register_socket(sockets[i], pub_info, false);
  i++;

  // 8  8008 Publisher register connection (topic A, id 7, wrong hash)
  pub_info.id = 7;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash(); // Wrong hash
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8008;
  init_pub_register_socket(sockets[i], pub_info, true);
  i++;

  // 9  8009 Publisher register connection (topic B, id 8, node not registered)
  pub_info.id = 8;
  pub_info.node_id = 5; // Invalid node ID
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8009;
  init_pub_register_socket(sockets[i], pub_info, true);
  i++;

  // 10 8001 Node deregister connection (id 1)
  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_deregister_socket(sockets[i], node_info);
  i++;

  // 11 8005 Node deregister connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_deregister_socket(sockets[i], node_info);
  i++;

  // 12 8002 Publisher deregister connection (topic A, id 2)
  pub_info.id = 2;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8002;
  init_pub_deregister_socket(sockets[i], pub_info);
  i++;

  // 13 8003 Publisher deregister connection (topic B, id 3)
  pub_info.id = 3;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8003;
  init_pub_deregister_socket(sockets[i], pub_info);
  i++;

  // 14 8006 Publisher deregister connection (topic A, id 5)
  pub_info.id = 5;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8006;
  init_pub_deregister_socket(sockets[i], pub_info);
  i++;

  // 15 8009 Publisher deregister connection (topic B, id 8, publisher not registered but attempted)
  pub_info.id = 8;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8009;
  init_pub_deregister_socket(sockets[i], pub_info);
  i++;

  // 16 8010 Publisher deregister connection (topic A, id 9, publisher not registered)
  pub_info.id = 9;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8010;
  init_pub_deregister_socket(sockets[i], pub_info);
  i++;

  // 17 8007 Publisher deregister connection (topic B, id 6)
  pub_info.id = 6;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8007;
  init_pub_deregister_socket(sockets[i], pub_info);

  {
    auto med = rix::core::Mediator(rixhub_endpoint, mock_create_socket);
    med.spin_once(); // Register node 1
    EXPECT_EQ(med.get_node_count(), 1);
    med.spin_once(); // Register publisher topic A, id 2
    EXPECT_EQ(med.get_publisher_count(), 1);
    med.spin_once(); // Register publisher topic B, id 3
    EXPECT_EQ(med.get_publisher_count(), 2);
    med.spin_once(); // Fail to register publisher topic A, id 2 (duplicate ID)
    EXPECT_EQ(med.get_publisher_count(), 2);
    med.spin_once(); // Register node 2
    EXPECT_EQ(med.get_node_count(), 2);
    med.spin_once(); // Register publisher topic A, id 5
    EXPECT_EQ(med.get_publisher_count(), 3);
    med.spin_once(); // Register publisher topic B, id 6
    EXPECT_EQ(med.get_publisher_count(), 4);
    med.spin_once(); // Fail to register publisher topic A, id 7 (wrong hash)
    EXPECT_EQ(med.get_publisher_count(), 4);
    med.spin_once(); // Fail to register publisher topic B, id 8 (node not registered)
    EXPECT_EQ(med.get_publisher_count(), 4);
    med.spin_once(); // Deregister node 1
    EXPECT_EQ(med.get_node_count(), 1);
    med.spin_once(); // Deregister node 2
    EXPECT_EQ(med.get_node_count(), 0);
    med.spin_once(); // Deregister publisher topic A, id 2
    EXPECT_EQ(med.get_publisher_count(), 3);
    med.spin_once(); // Deregister publisher topic B, id 3
    EXPECT_EQ(med.get_publisher_count(), 2);
    med.spin_once(); // Deregister publisher topic A, id 5
    EXPECT_EQ(med.get_publisher_count(), 1);
    med.spin_once(); // Fail to deregister publisher topic B, id 8 (not registered but attempted)
    EXPECT_EQ(med.get_publisher_count(), 1);
    med.spin_once(); // Fail to deregister publisher topic A, id 9 (not registered)
    EXPECT_EQ(med.get_publisher_count(), 1);
    med.spin_once(); // Deregister publisher topic B, id 6
    EXPECT_EQ(med.get_publisher_count(), 0);
  }

  sockets.clear();
  socket_index = 0;
}