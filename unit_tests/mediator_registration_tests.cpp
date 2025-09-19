#include "rix/core/mediator.hpp"
#include "rix/core/node.hpp"
#include "rix/ipc/mock_socket.hpp"
#include <gtest/gtest.h>

#include "helper_functions.hpp"

std::vector<std::shared_ptr<rix::ipc::MockSocket>> sockets;
int socket_index = 0;

std::shared_ptr<rix::ipc::GenericSocket> mock_create_socket() { return sockets[socket_index++]; }

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
  init_node_register_socket_med(sockets[1], node_info, false);

  node_info.name = "test_node_2";
  node_info.id = 2;
  init_node_register_socket_med(sockets[2], node_info, false);

  node_info.name = "test_node_3";
  node_info.id = 1;
  init_node_register_socket_med(sockets[3], node_info, true); // Duplicate ID error

  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_deregister_socket_med(sockets[4], node_info);

  node_info.name = "test_node_2";
  node_info.id = 2;
  init_node_deregister_socket_med(sockets[5], node_info);

  node_info.name = "test_node_3";
  node_info.id = 3;
  init_node_deregister_socket_med(sockets[6], node_info); // Never registered

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
  init_node_register_socket_med(sockets[i], node_info, false);
  i++;

  // 2  8002 Publisher register connection (topic A, id 2)
  rix::msg::mediator::PubInfo pub_info;
  pub_info.id = 2;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8002;
  init_pub_register_socket_med(sockets[i], pub_info, false);
  i++;

  // 3  8003 Publisher register connection (topic B, id 3)
  pub_info.id = 3;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8003;
  init_pub_register_socket_med(sockets[i], pub_info, false);
  i++;

  // 4  8004 Publisher register connection (topic A, id 2, duplicate id)
  pub_info.id = 2; // Duplicate ID
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8004;
  init_pub_register_socket_med(sockets[i], pub_info, true);
  i++;

  // 5  8005 Node register connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_register_socket_med(sockets[i], node_info, false);
  i++;

  // 6  8006 Publisher register connection (topic A, id 5)
  pub_info.id = 5;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8006;
  init_pub_register_socket_med(sockets[i], pub_info, false);
  i++;

  // 7  8007 Publisher register connection (topic B, id 6)
  pub_info.id = 6;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8007;
  init_pub_register_socket_med(sockets[i], pub_info, false);
  i++;

  // 8  8008 Publisher register connection (topic A, id 7, wrong hash)
  pub_info.id = 7;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash(); // Wrong hash
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8008;
  init_pub_register_socket_med(sockets[i], pub_info, true);
  i++;

  // 9  8009 Publisher register connection (topic B, id 8, node not registered)
  pub_info.id = 8;
  pub_info.node_id = 5; // Invalid node ID
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8009;
  init_pub_register_socket_med(sockets[i], pub_info, true);
  i++;

  // 10 8001 Node deregister connection (id 1)
  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_deregister_socket_med(sockets[i], node_info);
  i++;

  // 11 8005 Node deregister connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_deregister_socket_med(sockets[i], node_info);
  i++;

  // 12 8002 Publisher deregister connection (topic A, id 2)
  pub_info.id = 2;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8002;
  init_pub_deregister_socket_med(sockets[i], pub_info);
  i++;

  // 13 8003 Publisher deregister connection (topic B, id 3)
  pub_info.id = 3;
  pub_info.node_id = 1;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8003;
  init_pub_deregister_socket_med(sockets[i], pub_info);
  i++;

  // 14 8006 Publisher deregister connection (topic A, id 5)
  pub_info.id = 5;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8006;
  init_pub_deregister_socket_med(sockets[i], pub_info);
  i++;

  // 15 8009 Publisher deregister connection (topic B, id 8, publisher not registered but attempted)
  pub_info.id = 8;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8009;
  init_pub_deregister_socket_med(sockets[i], pub_info);
  i++;

  // 16 8010 Publisher deregister connection (topic A, id 9, publisher not registered)
  pub_info.id = 9;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_A";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8010;
  init_pub_deregister_socket_med(sockets[i], pub_info);
  i++;

  // 17 8007 Publisher deregister connection (topic B, id 6)
  pub_info.id = 6;
  pub_info.node_id = 4;
  pub_info.topic_info.name = "topic_B";
  pub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8007;
  init_pub_deregister_socket_med(sockets[i], pub_info);

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

TEST(RegistrationTests, MediatorRegisterAndDeregisterSubscriber) {
  // 0  8000 Mediator server
  // 1  8001 Node register connection (id 1)
  // 2  8002 Subscriber register connection (topic A, id 2)
  // 3  8003 Subscriber register connection (topic B, id 3)
  // 4  8004 Subscriber register connection (topic A, id 2, duplicate id)
  // 5  8005 Node register connection (id 4)
  // 6  8006 Subscriber register connection (topic A, id 5)
  // 7  8007 Subscriber register connection (topic B, id 6)
  // 8  8008 Subscriber register connection (topic A, id 7, wrong hash)
  // 9  8009 Subscriber register connection (topic B, id 8, node not registered)
  // 10 8001 Node deregister connection (id 1)
  // 11 8005 Node deregister connection (id 4)
  // 12 8002 Subscriber deregister connection (topic A, id 2)
  // 13 8003 Subscriber deregister connection (topic B, id 3)
  // 14 8006 Subscriber deregister connection (topic A, id 5)
  // 15 8009 Subscriber deregister connection (topic B, id 8, subscriber not registered but attempted)
  // 16 8010 Subscriber deregister connection (topic A, id 9, subscriber not registered)
  // 17 8007 Subscriber deregister connection (topic B, id 6)

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
  init_node_register_socket_med(sockets[i], node_info, false);
  i++;

  // 2  8002 Subscriber register connection (topic A, id 2)
  rix::msg::mediator::SubInfo sub_info;
  sub_info.id = 2;
  sub_info.node_id = 1;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8002;
  init_sub_register_socket_med(sockets[i], sub_info, false);
  i++;

  // 3  8003 Subscriber register connection (topic B, id 3)
  sub_info.id = 3;
  sub_info.node_id = 1;
  sub_info.topic_info.name = "topic_B";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8003;
  init_sub_register_socket_med(sockets[i], sub_info, false);
  i++;

  // 4  8004 Subscriber register connection (topic A, id 2, duplicate id)
  sub_info.id = 2; // Duplicate ID
  sub_info.node_id = 1;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8004;
  init_sub_register_socket_med(sockets[i], sub_info, true);
  i++;

  // 5  8005 Node register connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_register_socket_med(sockets[i], node_info, false);
  i++;

  // 6  8006 Subscriber register connection (topic A, id 5)
  sub_info.id = 5;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8006;
  init_sub_register_socket_med(sockets[i], sub_info, false);
  i++;

  // 7  8007 Subscriber register connection (topic B, id 6)
  sub_info.id = 6;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_B";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8007;
  init_sub_register_socket_med(sockets[i], sub_info, false);
  i++;

  // 8  8008 Subscriber register connection (topic A, id 7, wrong hash)
  sub_info.id = 7;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash(); // Wrong hash
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8008;
  init_sub_register_socket_med(sockets[i], sub_info, true);
  i++;

  // 9  8009 Subscriber register connection (topic B, id 8, node not registered)
  sub_info.id = 8;
  sub_info.node_id = 5; // Invalid node ID
  sub_info.topic_info.name = "topic_B";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8009;
  init_sub_register_socket_med(sockets[i], sub_info, true);
  i++;

  // 10 8001 Node deregister connection (id 1)
  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_deregister_socket_med(sockets[i], node_info);
  i++;

  // 11 8005 Node deregister connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_deregister_socket_med(sockets[i], node_info);
  i++;

  // 12 8002 Subscriber deregister connection (topic A, id 2)
  sub_info.id = 2;
  sub_info.node_id = 1;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8002;
  init_sub_deregister_socket_med(sockets[i], sub_info);
  i++;

  // 13 8003 Subscriber deregister connection (topic B, id 3)
  sub_info.id = 3;
  sub_info.node_id = 1;
  sub_info.topic_info.name = "topic_B";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8003;
  init_sub_deregister_socket_med(sockets[i], sub_info);
  i++;

  // 14 8006 Subscriber deregister connection (topic A, id 5)
  sub_info.id = 5;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8006;
  init_sub_deregister_socket_med(sockets[i], sub_info);
  i++;

  // 15 8009 Subscriber deregister connection (topic B, id 8, subscriber not registered but attempted)
  sub_info.id = 8;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_B";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8009;
  init_sub_deregister_socket_med(sockets[i], sub_info);
  i++;

  // 16 8010 Subscriber deregister connection (topic A, id 9, subscriber not registered)
  sub_info.id = 9;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_A";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8010;
  init_sub_deregister_socket_med(sockets[i], sub_info);
  i++;

  // 17 8007 Subscriber deregister connection (topic B, id 6)
  sub_info.id = 6;
  sub_info.node_id = 4;
  sub_info.topic_info.name = "topic_B";
  sub_info.topic_info.message_hash = rix::msg::standard::Time().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8007;
  init_sub_deregister_socket_med(sockets[i], sub_info);

  {
    auto med = rix::core::Mediator(rixhub_endpoint, mock_create_socket);
    med.spin_once(); // Register node 1
    EXPECT_EQ(med.get_node_count(), 1);
    med.spin_once(); // Register subscriber topic A, id 2
    EXPECT_EQ(med.get_subscriber_count(), 1);
    med.spin_once(); // Register subscriber topic B, id 3
    EXPECT_EQ(med.get_subscriber_count(), 2);
    med.spin_once(); // Fail to register subscriber topic A, id 2 (duplicate ID)
    EXPECT_EQ(med.get_subscriber_count(), 2);
    med.spin_once(); // Register node 2
    EXPECT_EQ(med.get_node_count(), 2);
    med.spin_once(); // Register subscriber topic A, id 5
    EXPECT_EQ(med.get_subscriber_count(), 3);
    med.spin_once(); // Register subscriber topic B, id 6
    EXPECT_EQ(med.get_subscriber_count(), 4);
    med.spin_once(); // Fail to register subscriber topic A, id 7 (wrong hash)
    EXPECT_EQ(med.get_subscriber_count(), 4);
    med.spin_once(); // Fail to register subscriber topic B, id 8 (node not registered)
    EXPECT_EQ(med.get_subscriber_count(), 4);
    med.spin_once(); // Deregister node 1
    EXPECT_EQ(med.get_node_count(), 1);
    med.spin_once(); // Deregister node 2
    EXPECT_EQ(med.get_node_count(), 0);
    med.spin_once(); // Deregister subscriber topic A, id 2
    EXPECT_EQ(med.get_subscriber_count(), 3);
    med.spin_once(); // Deregister subscriber topic B, id 3
    EXPECT_EQ(med.get_subscriber_count(), 2);
    med.spin_once(); // Deregister subscriber topic A, id 5
    EXPECT_EQ(med.get_subscriber_count(), 1);
    med.spin_once(); // Fail to deregister subscriber topic B, id 8 (not registered but attempted)
    EXPECT_EQ(med.get_subscriber_count(), 1);
    med.spin_once(); // Fail to deregister subscriber topic A, id 9 (not registered)
    EXPECT_EQ(med.get_subscriber_count(), 1);
    med.spin_once(); // Deregister subscriber topic B, id 6
    EXPECT_EQ(med.get_subscriber_count(), 0);
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, MediatorRegisterAndDeregisterService) {
  // 0  8000 Mediator server
  // 1  8001 Node register connection (id 1)
  // 2  8002 Service register connection (service A, id 2)
  // 3  8003 Service register connection (service B, id 3)
  // 4  8004 Service Client request connection (service A, id 4)
  // 5  8005 Node register connection (id 4)
  // 6  8006 Service register connection (service A, id 5, duplicate service)
  // 7  8007 Service register connection (service B, id 6, node not registered)
  // 8  8008 Service Client request connection (service A, id 7, wrong hash)
  // 16  8011 Service Client request connection (service C, id 10, service not registered)
  // 17  8011 Service Client request connection (service A, id 11, node not registered)
  // 9  8009 Service Client request connection (service B, id 8)
  // 10 8001 Node deregister connection (id 1)
  // 11 8005 Node deregister connection (id 4)
  // 12 8009 Service deregister connection (service C, id 8, service not registered)
  // 13 8002 Service deregister connection (service A, id 2)
  // 14 8003 Service deregister connection (service B, id 3)
  // 15 8010 Service Client request connection (service B, id 9, service not registered)
  
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
  init_node_register_socket_med(sockets[i], node_info, false);
  i++;

  // 2  8002 Service register connection (service A, id 2)
  rix::msg::mediator::SrvInfo srv_info;
  srv_info.id = 2;
  srv_info.node_id = 1;
  srv_info.name = "service_A";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8002;
  init_srv_register_socket_med(sockets[i], srv_info, false);
  i++;

  // 3  8003 Service register connection (service B, id 3)
  srv_info.id = 3;
  srv_info.node_id = 1;
  srv_info.name = "service_B";
  srv_info.request_hash = rix::msg::standard::Time().hash();
  srv_info.response_hash = rix::msg::standard::UInt32().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8003;
  init_srv_register_socket_med(sockets[i], srv_info, false);
  i++;

  // 4  8004 Service Client request connection (service A, id 4)
  rix::msg::mediator::SrvRequest srv_request;
  rix::msg::mediator::SrvResponse srv_response;
  srv_request.id = 2; // Duplicate ID
  srv_request.node_id = 1;
  srv_request.name = "service_A";
  srv_request.request_hash = rix::msg::standard::UInt32().hash();
  srv_request.response_hash = rix::msg::standard::Time().hash();
  srv_response.error = 0;
  srv_response.srv_info.id = 2;
  srv_response.srv_info.node_id = 1;
  srv_response.srv_info.name = "service_A";
  srv_response.srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_response.srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_response.srv_info.endpoint.address = "127.0.0.1";
  srv_response.srv_info.endpoint.port = 8002;
  init_srvcli_request_socket_med(sockets[i], srv_request, srv_response);
  i++;

  // 5  8005 Node register connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_register_socket_med(sockets[i], node_info, false);
  i++;

  // 6  8006 Service register connection (service A, id 5, duplicate service)
  srv_info.id = 5;
  srv_info.node_id = 4;
  srv_info.name = "service_A";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8006;
  init_srv_register_socket_med(sockets[i], srv_info, true);
  i++;

  // 7  8007 Service register connection (service C, id 6, node not registered)
  srv_info.id = 6;
  srv_info.node_id = 5; // Node not registered
  srv_info.name = "service_C";
  srv_info.request_hash = rix::msg::standard::Duration().hash();
  srv_info.response_hash = rix::msg::standard::UInt32().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8007;
  init_srv_register_socket_med(sockets[i], srv_info, true);
  i++;

  // 8  8008 Service register connection (service A, id 7, wrong hash)
  srv_info.id = 7;
  srv_info.node_id = 4;
  srv_info.name = "service_A";
  srv_info.request_hash = rix::msg::standard::Time().hash();    // Wrong hash
  srv_info.response_hash = rix::msg::standard::UInt32().hash(); // Wrong hash
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8008;
  init_srv_register_socket_med(sockets[i], srv_info, true);
  i++;

  // 16  8011 Service Client request connection (service C, id 10, service not registered)
  srv_request.id = 10;
  srv_request.node_id = 1;
  srv_request.name = "service_C"; // Service not registered
  srv_request.request_hash = rix::msg::standard::UInt32().hash();
  srv_request.response_hash = rix::msg::standard::Time().hash();
  srv_response = rix::msg::mediator::SrvResponse();
  srv_response.error = -1;
  init_srvcli_request_socket_med(sockets[i], srv_request, srv_response);
  i++;

  // 17  8011 Service Client request connection (service A, id 11, node not registered)
  srv_request.id = 11;
  srv_request.node_id = 5; // Node not registered
  srv_request.name = "service_A";
  srv_request.request_hash = rix::msg::standard::UInt32().hash();
  srv_request.response_hash = rix::msg::standard::Time().hash();
  srv_response = rix::msg::mediator::SrvResponse();
  srv_response.error = -1;
  init_srvcli_request_socket_med(sockets[i], srv_request, srv_response);
  i++;

  // 9  8009 Service Client request connection (service B, id 8)
  srv_request.id = 8; // Duplicate ID
  srv_request.node_id = 1;
  srv_request.name = "service_B";
  srv_request.request_hash = rix::msg::standard::Time().hash();
  srv_request.response_hash = rix::msg::standard::UInt32().hash();
  srv_response.error = 0;
  srv_response.srv_info.id = 3;
  srv_response.srv_info.node_id = 1;
  srv_response.srv_info.name = "service_B";
  srv_response.srv_info.request_hash = rix::msg::standard::Time().hash();
  srv_response.srv_info.response_hash = rix::msg::standard::UInt32().hash();
  srv_response.srv_info.endpoint.address = "127.0.0.1";
  srv_response.srv_info.endpoint.port = 8003;
  init_srvcli_request_socket_med(sockets[i], srv_request, srv_response);
  i++;

  // 10 8001 Node deregister connection (id 1)
  node_info.name = "test_node_1";
  node_info.id = 1;
  init_node_deregister_socket_med(sockets[i], node_info);
  i++;

  // 11 8005 Node deregister connection (id 4)
  node_info.name = "test_node_2";
  node_info.id = 4;
  init_node_deregister_socket_med(sockets[i], node_info);
  i++;

  // 12 8009 Service deregister connection (service C, id 8, service not registered)
  srv_info.id = 5;
  srv_info.node_id = 4;
  srv_info.name = "service_C";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8006;
  init_srv_deregister_socket_med(sockets[i], srv_info);
  i++;

  // 13 8002 Service deregister connection (service A, id 2)
  srv_info.id = 2;
  srv_info.node_id = 1;
  srv_info.name = "service_A";
  srv_info.request_hash = rix::msg::standard::Time().hash();
  srv_info.response_hash = rix::msg::standard::UInt32().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8002;
  init_srv_deregister_socket_med(sockets[i], srv_info);
  i++;

  // 14 8003 Service deregister connection (service B, id 3)
  srv_info.id = 3;
  srv_info.node_id = 1;
  srv_info.name = "service_B";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8003;
  init_srv_deregister_socket_med(sockets[i], srv_info);
  i++;

  // 15 8010 Service Client request connection (service B, id 9, service not registered)
  srv_request.id = 9;
  srv_request.node_id = 1;
  srv_request.name = "service_B"; // Service not registered
  srv_request.request_hash = rix::msg::standard::Time().hash();
  srv_request.response_hash = rix::msg::standard::UInt32().hash();
  srv_response = rix::msg::mediator::SrvResponse();
  srv_response.error = -1;
  init_srvcli_request_socket_med(sockets[i], srv_request, srv_response);

  {
    auto med = rix::core::Mediator(rixhub_endpoint, mock_create_socket);
    med.spin_once(); // 1  8001 Node register connection (id 1)
    EXPECT_EQ(med.get_node_count(), 1);
    med.spin_once(); // 2  8002 Service register connection (service A, id 2)
    EXPECT_EQ(med.get_service_count(), 1);
    med.spin_once(); // 3  8003 Service register connection (service B, id 3)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 4  8004 Service Client request connection (service A, id 4)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 5  8005 Node register connection (id 4)
    EXPECT_EQ(med.get_node_count(), 2);
    med.spin_once(); // 6  8006 Service register connection (service A, id 5, duplicate service)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 7  8007 Service register connection (service B, id 6, node not registered)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 8  8008 Service Client request connection (service A, id 7, wrong hash)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 16  8011 Service Client request connection (service C, id 10, service not registered)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 17  8011 Service Client request connection (service A, id 11, node not registered)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 9  8009 Service Client request connection (service B, id 8)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 10 8001 Node deregister connection (id 1)
    EXPECT_EQ(med.get_node_count(), 1);
    med.spin_once(); // 11 8005 Node deregister connection (id 4)
    EXPECT_EQ(med.get_node_count(), 0);
    med.spin_once(); // 12 8009 Service deregister connection (service C, id 8, service not registered)
    EXPECT_EQ(med.get_service_count(), 2);
    med.spin_once(); // 13 8002 Service deregister connection (service A, id 2)
    EXPECT_EQ(med.get_service_count(), 1);
    med.spin_once(); // 14 8003 Service deregister connection (service B, id 3)
    EXPECT_EQ(med.get_service_count(), 0);
    med.spin_once(); // 15 8010 Service Client request connection (service B, id 9, service not registered)
    EXPECT_EQ(med.get_service_count(), 0);
  }

  sockets.clear();
  socket_index = 0;
}