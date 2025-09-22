#include "rix/core/node.hpp"
#include "rix/ipc/mock_socket.hpp"
#include <gtest/gtest.h>

#include "node_helper_functions.hpp"

std::vector<std::shared_ptr<rix::ipc::MockSocket>> sockets;
int socket_index = 0;

TEST(RegistrationTests, PublisherAcceptConnections) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher server
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher accept connection 1
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher accept connection 2 (failure)
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher accept connection 3
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

TEST(RegistrationTests, SystemInfoRequest) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // System info request
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id = 1;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::SystemInfo sys_info;
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  node_info.id = node_id;
  sys_info.nodes.push_back(node_info);
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);
  
  rix::msg::mediator::NodeInfo other_node_info;
  other_node_info.name = "other_node";
  other_node_info.id = 2;
  sys_info.nodes.push_back(other_node_info);

  rix::msg::mediator::PubInfo pub_info;
  pub_info.topic_info.name = "test_topic";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = "127.0.0.1";
  pub_info.endpoint.port = 8001;
  pub_info.id = 3;
  pub_info.node_id = 2;
  sys_info.publishers.push_back(pub_info);

  rix::msg::mediator::SubInfo sub_info;
  sub_info.topic_info.name = "test_topic";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = "127.0.0.1";
  sub_info.endpoint.port = 8001;
  sub_info.id = 4;
  sub_info.node_id = 2;
  sys_info.subscribers.push_back(sub_info);

  rix::msg::mediator::SrvInfo srv_info;
  srv_info.name = "test_service";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = "127.0.0.1";
  srv_info.endpoint.port = 8001;
  srv_info.id = 5;
  srv_info.node_id = 2;
  sys_info.services.push_back(srv_info);

  init_sys_info_request_socket(sockets[1], rixhub_endpoint, sys_info, node_id);

  init_node_deregister_socket(sockets[2], rixhub_endpoint, node_info);

  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    rix::msg::mediator::SystemInfo info;
    EXPECT_TRUE(node.get_system_info(info));
    EXPECT_EQ(info.nodes.size(), 2);
    EXPECT_EQ(info.nodes[0].name, "test_node");
    EXPECT_EQ(info.nodes[0].id, 1);
    EXPECT_EQ(info.nodes[1].name, "other_node");
    EXPECT_EQ(info.nodes[1].id, 2);

    EXPECT_EQ(info.publishers.size(), 1);
    EXPECT_EQ(info.publishers[0].topic_info.name, "test_topic");
    EXPECT_EQ(info.publishers[0].topic_info.message_hash, rix::msg::standard::UInt32().hash());
    EXPECT_EQ(info.publishers[0].endpoint.address, "127.0.0.1");
    EXPECT_EQ(info.publishers[0].endpoint.port, 8001);
    EXPECT_EQ(info.publishers[0].id, 3);
    EXPECT_EQ(info.publishers[0].node_id, 2);

    EXPECT_EQ(info.subscribers.size(), 1);
    EXPECT_EQ(info.subscribers[0].topic_info.name, "test_topic");
    EXPECT_EQ(info.subscribers[0].topic_info.message_hash, rix::msg::standard::UInt32().hash());
    EXPECT_EQ(info.subscribers[0].endpoint.address, "127.0.0.1");
    EXPECT_EQ(info.subscribers[0].endpoint.port, 8001);
    EXPECT_EQ(info.subscribers[0].id, 4);
    EXPECT_EQ(info.subscribers[0].node_id, 2);

    EXPECT_EQ(info.services.size(), 1);
    EXPECT_EQ(info.services[0].name, "test_service");
    EXPECT_EQ(info.services[0].request_hash, rix::msg::standard::UInt32().hash());
    EXPECT_EQ(info.services[0].response_hash, rix::msg::standard::Time().hash());
    EXPECT_EQ(info.services[0].endpoint.address, "127.0.0.1");
    EXPECT_EQ(info.services[0].endpoint.port, 8001);
    EXPECT_EQ(info.services[0].id, 5);
    EXPECT_EQ(info.services[0].node_id, 2);
  }

  sockets.clear();
  socket_index = 0;
}

TEST(RegistrationTests, ParameterGetAndSet) {
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Parameter set
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Parameter set wrong type
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Parameter get
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Parameter get non-existent
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Parameter get wrong type
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Node deregister

  uint64_t node_id;
  rix::ipc::Endpoint rixhub_endpoint("127.0.0.1", 8000);

  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  node_info.id = node_id;
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::core::OPCODE::NODE_REGISTER, false);
  
  rix::msg::mediator::ParamInfo parameter;
  rix::msg::standard::Time param_a_value;
  param_a_value.sec = 42;
  param_a_value.nsec = 84;
  parameter.name = "test_param_a";
  parameter.message_hash = param_a_value.hash();
  parameter.data.resize(param_a_value.size());
  size_t offset = 0;
  param_a_value.serialize(parameter.data.data(), offset);
  init_param_set_request_socket(sockets[1], rixhub_endpoint, parameter, node_id, false);

  rix::msg::standard::UInt64 wrong_type_value;
  wrong_type_value.data = 12345678;
  parameter.name = "test_param_a";
  parameter.message_hash = wrong_type_value.hash();
  parameter.data.resize(wrong_type_value.size());
  offset = 0;
  wrong_type_value.serialize(parameter.data.data(), offset);
  init_param_set_request_socket(sockets[2], rixhub_endpoint, parameter, node_id, true);

  parameter.name = "test_param_a";
  parameter.message_hash = param_a_value.hash();
  parameter.data.resize(param_a_value.size());
  offset = 0;
  param_a_value.serialize(parameter.data.data(), offset);
  init_param_get_request_socket(sockets[3], rixhub_endpoint, parameter, node_id, false);

  parameter.name = "non_existent_param";
  parameter.message_hash = wrong_type_value.hash();
  init_param_get_request_socket(sockets[4], rixhub_endpoint, parameter, node_id, true);

  parameter.name = "test_param_a";
  parameter.message_hash = wrong_type_value.hash();
  init_param_get_request_socket(sockets[5], rixhub_endpoint, parameter, node_id, true);

  init_node_deregister_socket(sockets[6], rixhub_endpoint, node_info);

  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Set parameter
    rix::msg::standard::Time set_value;
    set_value.sec = 42;
    set_value.nsec = 84;
    EXPECT_TRUE(node.set_parameter("test_param_a", set_value));

    // Set parameter with wrong type
    rix::msg::standard::UInt64 wrong_type_value;
    wrong_type_value.data = 12345678;
    EXPECT_FALSE(node.set_parameter("test_param_a", wrong_type_value));

    // Get parameter
    rix::msg::standard::Time get_value;
    EXPECT_TRUE(node.get_parameter("test_param_a", get_value));
    EXPECT_EQ(get_value.sec, 42);
    EXPECT_EQ(get_value.nsec, 84);

    // Attempt to get non-existent parameter
    rix::msg::standard::UInt64 non_existent_value;
    EXPECT_FALSE(node.get_parameter("non_existent_param", non_existent_value));

    // Get parameter with wrong type
    rix::msg::standard::UInt64 wrong_type_get;
    EXPECT_FALSE(node.get_parameter("test_param_a", wrong_type_get));
  }

  sockets.clear();
  socket_index = 0;
}