#include "rix/core/mediator.hpp"
#include "rix/core/node.hpp"
#include "rix/ipc/mock_socket.hpp"
#include <gtest/gtest.h>

#include "helper_functions.hpp"

std::vector<std::shared_ptr<rix::ipc::MockSocket>> sockets;
int socket_index = 0;

TEST(RegistrationTests, RegisterAndDeregisterNode) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);

  // Node register client socket will be used to send a register message to rixhub
  uint64_t node_id = 0;
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  // Node deregister client socket will be used to send a deregister message to rixhub
  init_node_deregister_socket(sockets[1], rixhub_endpoint, node_info);

  // Create a node, which should trigger the registration process
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterNodeFailure) {
  // Only need one socket, since registration will fail. Node should not attempt to deregister.
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register

  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  uint64_t node_id = 0;

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, true);

  // Create a node, which should trigger the registration process
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_FALSE(node.ok());

    auto sub = node.create_subscriber<rix::msg::standard::UInt32>(
        "test_topic", [](const rix::msg::standard::UInt32 &msg) { EXPECT_TRUE(false); });
    EXPECT_EQ(sub, nullptr);

    auto pub = node.create_publisher<rix::msg::standard::UInt32>("test_topic");
    EXPECT_EQ(pub, nullptr);

    auto srv = node.create_service<rix::msg::standard::UInt32, rix::msg::standard::UInt32>(
        "test_service",
        [](const rix::msg::standard::UInt32 &req, rix::msg::standard::UInt32 &res) { EXPECT_TRUE(false); });
    EXPECT_EQ(srv, nullptr);

    auto cli = node.create_service_client<rix::msg::standard::UInt32, rix::msg::standard::UInt32>("test_service");
    EXPECT_EQ(cli, nullptr);

    auto timer = node.create_timer(rix::util::Duration(1.0), [](rix::core::Timer::Event) { EXPECT_TRUE(false); });
    EXPECT_EQ(timer, nullptr);
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterAndDeregisterPublisher) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher deregister
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint pub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint pub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], pub_endpoint, pub_bound_endpoint);

  rix::msg::mediator::PubInfo pub_info;
  pub_info.topic_info.name = "test_topic";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = pub_bound_endpoint.address;
  pub_info.endpoint.port = pub_bound_endpoint.port;
  init_pub_register_socket(sockets[2], rixhub_endpoint, pub_info, node_id, false);

  init_pub_deregister_socket(sockets[3], rixhub_endpoint, pub_info, node_id);

  init_node_deregister_socket(sockets[4], rixhub_endpoint, node_info);

  // Test creating a node and publisher
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // publisher uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto pub = node.create_publisher<rix::msg::standard::UInt32>("test_topic", rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());

    // Shutdown publisher, which should trigger deregistration
    pub->shutdown();
    EXPECT_FALSE(pub->ok());
    pub = nullptr;
    node.spin_once(); // Allow node to process publisher shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, DeregisterPublisherFromNodeDtor) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint pub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint pub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], pub_endpoint, pub_bound_endpoint);

  rix::msg::mediator::PubInfo pub_info;
  pub_info.topic_info.name = "test_topic";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = pub_bound_endpoint.address;
  pub_info.endpoint.port = pub_bound_endpoint.port;
  init_pub_register_socket(sockets[2], rixhub_endpoint, pub_info, node_id, false);

  init_node_deregister_socket(sockets[3], rixhub_endpoint, node_info);

  init_pub_deregister_socket(sockets[4], rixhub_endpoint, pub_info, node_id);

  // Test creating a node and publisher
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // publisher uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto pub = node.create_publisher<rix::msg::standard::UInt32>("test_topic", rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterPublisherFailure) {
  // Publisher registration will fail, but node should still deregister properly
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint pub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint pub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], pub_endpoint, pub_bound_endpoint);

  rix::msg::mediator::PubInfo pub_info;
  pub_info.topic_info.name = "test_topic";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = pub_bound_endpoint.address;
  pub_info.endpoint.port = pub_bound_endpoint.port;
  init_pub_register_socket(sockets[2], rixhub_endpoint, pub_info, node_id, true);

  init_node_deregister_socket(sockets[3], rixhub_endpoint, node_info);

  // Test creating a node and publisher
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // publisher uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto pub = node.create_publisher<rix::msg::standard::UInt32>("test_topic", rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(pub, nullptr);
    EXPECT_FALSE(pub->ok());

    // Shutdown publisher, which should not trigger deregistration
    pub = nullptr;
    node.spin_once(); // Allow node to process publisher shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterAndDeregisterSubscriber) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber deregister
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint sub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint sub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], sub_endpoint, sub_bound_endpoint);

  rix::msg::mediator::SubInfo sub_info;
  sub_info.topic_info.name = "test_topic";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = sub_bound_endpoint.address;
  sub_info.endpoint.port = sub_bound_endpoint.port;
  init_sub_register_socket(sockets[2], rixhub_endpoint, sub_info, node_id, false);

  init_sub_deregister_socket(sockets[3], rixhub_endpoint, sub_info, node_id);

  init_node_deregister_socket(sockets[4], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // subscriber uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto sub = node.create_subscriber<rix::msg::standard::UInt32>(
        "test_topic", [](const rix::msg::standard::UInt32 &) {}, rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(sub, nullptr);
    EXPECT_TRUE(sub->ok());

    // Shutdown subscriber, which should trigger deregistration
    sub->shutdown();
    EXPECT_FALSE(sub->ok());
    sub = nullptr;
    node.spin_once(); // Allow node to process subscriber shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, DeregisterSubscriberFromNodeDtor) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint sub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint sub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], sub_endpoint, sub_bound_endpoint);

  rix::msg::mediator::SubInfo sub_info;
  sub_info.topic_info.name = "test_topic";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = sub_bound_endpoint.address;
  sub_info.endpoint.port = sub_bound_endpoint.port;
  init_sub_register_socket(sockets[2], rixhub_endpoint, sub_info, node_id, false);

  init_node_deregister_socket(sockets[3], rixhub_endpoint, node_info);

  init_sub_deregister_socket(sockets[4], rixhub_endpoint, sub_info, node_id);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // subscriber uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto sub = node.create_subscriber<rix::msg::standard::UInt32>(
        "test_topic", [](const rix::msg::standard::UInt32 &) {}, rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(sub, nullptr);
    EXPECT_TRUE(sub->ok());
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterSubscriberFailure) {
  // Subscriber registration will fail, but node should still deregister properly
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Subscriber register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint sub_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint sub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], sub_endpoint, sub_bound_endpoint);

  rix::msg::mediator::SubInfo sub_info;
  sub_info.topic_info.name = "test_topic";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = sub_bound_endpoint.address;
  sub_info.endpoint.port = sub_bound_endpoint.port;
  init_sub_register_socket(sockets[2], rixhub_endpoint, sub_info, node_id, true);

  init_node_deregister_socket(sockets[3], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // subscriber uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto sub = node.create_subscriber<rix::msg::standard::UInt32>(
        "test_topic", [](const rix::msg::standard::UInt32 &) {}, rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(sub, nullptr);
    EXPECT_FALSE(sub->ok());

    // Shutdown subscriber, which should not trigger deregistration
    sub = nullptr;
    node.spin_once(); // Allow node to process subscriber shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterAndDeregisterService) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service deregister
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint srv_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint srv_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], srv_endpoint, srv_bound_endpoint);

  rix::msg::mediator::SrvInfo srv_info;
  srv_info.name = "test_service";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = srv_bound_endpoint.address;
  srv_info.endpoint.port = srv_bound_endpoint.port;
  init_srv_register_socket(sockets[2], rixhub_endpoint, srv_info, node_id, false);

  init_srv_deregister_socket(sockets[3], rixhub_endpoint, srv_info, node_id);

  init_node_deregister_socket(sockets[4], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto srv = node.create_service<rix::msg::standard::UInt32, rix::msg::standard::Time>(
        "test_service", [](const rix::msg::standard::UInt32 &, rix::msg::standard::Time &) {},
        rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());

    // Shutdown service, which should trigger deregistration
    srv->shutdown();
    EXPECT_FALSE(srv->ok());
    srv = nullptr;
    node.spin_once(); // Allow node to process service shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, DeregisterServiceFromNodeDtor) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint srv_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint srv_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], srv_endpoint, srv_bound_endpoint);

  rix::msg::mediator::SrvInfo srv_info;
  srv_info.name = "test_service";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = srv_bound_endpoint.address;
  srv_info.endpoint.port = srv_bound_endpoint.port;
  init_srv_register_socket(sockets[2], rixhub_endpoint, srv_info, node_id, false);

  init_node_deregister_socket(sockets[3], rixhub_endpoint, node_info);

  init_srv_deregister_socket(sockets[4], rixhub_endpoint, srv_info, node_id);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto srv = node.create_service<rix::msg::standard::UInt32, rix::msg::standard::Time>(
        "test_service", [](const rix::msg::standard::UInt32 &, rix::msg::standard::Time &) {},
        rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RegisterServiceFailure) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::ipc::Endpoint srv_endpoint("127.0.0.1", 0);
  rix::ipc::Endpoint srv_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  init_server_socket(sockets[1], srv_endpoint, srv_bound_endpoint);

  rix::msg::mediator::SrvInfo srv_info;
  srv_info.name = "test_service";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = srv_bound_endpoint.address;
  srv_info.endpoint.port = srv_bound_endpoint.port;
  init_srv_register_socket(sockets[2], rixhub_endpoint, srv_info, node_id, true);

  init_node_deregister_socket(sockets[3], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto srv = node.create_service<rix::msg::standard::UInt32, rix::msg::standard::Time>(
        "test_service", [](const rix::msg::standard::UInt32 &, rix::msg::standard::Time &) {},
        rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(srv, nullptr);
    EXPECT_FALSE(srv->ok());

    // Shutdown service, which should not trigger deregistration
    srv = nullptr;
    node.spin_once(); // Allow node to process service shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RequestServiceClient) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service request
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  rix::msg::mediator::SrvResponse srv_response;
  srv_response.srv_info.name = "test_service";
  srv_response.srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_response.srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_response.srv_info.node_id = node_id;
  srv_response.srv_info.id = node_id + 1; // Just a different ID than the node ID
  srv_response.srv_info.endpoint.address = "127.0.0.1";
  srv_response.srv_info.endpoint.port = 8001;
  srv_response.error = 0;
  init_srvcli_socket(sockets[1], rixhub_endpoint, srv_response, node_id);

  init_node_deregister_socket(sockets[2], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto srvcli = node.create_service_client<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_service");
    EXPECT_NE(srvcli, nullptr);
    EXPECT_TRUE(srvcli->ok());

    // Shutdown service client, which should not trigger a deregistration
    srvcli->shutdown();
    EXPECT_FALSE(srvcli->ok());
    srvcli = nullptr;
    node.spin_once(); // Allow node to process service client shutdown
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, RequestServiceClientFailure) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Service request
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);

  rix::msg::mediator::SrvResponse srv_response;
  srv_response.error = -1; // Indicate failure
  init_srvcli_socket(sockets[1], rixhub_endpoint, srv_response, node_id);

  init_node_deregister_socket(sockets[2], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto srvcli = node.create_service_client<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_service");
    EXPECT_NE(srvcli, nullptr);
    EXPECT_FALSE(srvcli->ok());

    // Shutdown service client, which should not trigger a deregistration
    srvcli = nullptr;
    node.spin_once(); // Allow node to process service client shutdown
  }

  sockets.clear();
  socket_index = 0;
}