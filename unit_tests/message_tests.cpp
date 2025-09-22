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
  sockets.push_back(std::make_shared<rix::ipc::MockSocket>()); // Publisher accept connection 2
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

  init_pub_server_socket(sockets[1], pub_endpoint, pub_bound_endpoint, 3);

  rix::msg::mediator::PubInfo pub_info;
  pub_info.topic_info.name = "test_topic";
  pub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  pub_info.endpoint.address = pub_bound_endpoint.address;
  pub_info.endpoint.port = pub_bound_endpoint.port;
  init_pub_register_socket(sockets[2], rixhub_endpoint, pub_info, node_id, false);

  init_pub_connection_socket<rix::msg::standard::UInt32>(sockets[3], 3,
                                                         {
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                         });
  init_pub_connection_socket<rix::msg::standard::UInt32>(sockets[4], 3,
                                                         {
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                         });
  init_pub_connection_socket<rix::msg::standard::UInt32>(sockets[5], 3,
                                                         {
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                             std::make_shared<rix::msg::standard::UInt32>(),
                                                         });

  init_pub_deregister_socket(sockets[6], rixhub_endpoint, pub_info, node_id);

  init_node_deregister_socket(sockets[7], rixhub_endpoint, node_info);

  // Test creating a node and publisher
  {
    rix::core::Node node("test_node", rix::ipc::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // publisher uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto pub = node.create_publisher<rix::msg::standard::UInt32>("test_topic", rix::ipc::Endpoint("127.0.0.1", 0));
    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());

    node.spin_once();
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 1);
    
    node.spin_once();
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 2);
    
    node.spin_once();
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 3);

    auto msg = std::make_shared<rix::msg::standard::UInt32>();
    msg->data = 42;
    pub->publish(*msg);
    pub->publish(*msg);
    pub->publish(*msg);

    auto wrong_msg = std::make_shared<rix::msg::standard::Time>();
    pub->publish(*wrong_msg); // Should not be sent

    // Shutdown publisher, which should trigger deregistration
    pub->shutdown();
    EXPECT_FALSE(pub->ok());
    pub = nullptr;
    node.spin_once(); // Allow node to process publisher shutdown
  }

  sockets.clear();
  socket_index = 0;
}