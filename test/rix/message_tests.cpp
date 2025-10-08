#include "rix/core/node.hpp"
#include "rix/ipc/mock_poller.hpp"
#include "rix/ipc/mock_socket.hpp"
#include <gtest/gtest.h>

#include "node_helper_functions.hpp"

std::vector<std::shared_ptr<rix::MockSocket>> sockets;
int socket_index = 0;

TEST(MessageTests, PublisherAcceptConnectionsAndPublish) {
  auto poller = std::make_shared<rix::MockPoller>();
  rix::GenericSocket::set_poller(poller);

  EXPECT_CALL(*poller, poll).Times(::testing::AtLeast(1));

  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Publisher server
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Publisher register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Publisher accept connection 1
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Publisher accept connection 2
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Publisher accept connection 3
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Publisher deregister
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::Endpoint pub_endpoint("127.0.0.1", 0);
  rix::Endpoint pub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::OPCODE::NODE_REGISTER, false);

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
    rix::Node node("test_node", rix::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // publisher uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto pub = node.create_publisher<rix::msg::standard::UInt32>("test_topic", rix::Endpoint("127.0.0.1", 0));
    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());

#ifndef RIX_MULTITHREADED
    node.spin_once();
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 1);

    node.spin_once();
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 2);

    node.spin_once();
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 3);
#else
    // Give some time for the publisher to accept connections in its own thread
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(pub->ok());
    EXPECT_EQ(pub->get_subscriber_count(), 3);
#endif

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

  poller = nullptr;
  rix::GenericSocket::set_poller(nullptr);
}

TEST(MessageTests, SubscriberConnectAndReceive) {
  auto poller = std::make_shared<rix::MockPoller>();
  rix::GenericSocket::set_poller(poller);

  auto poll_call_count = std::make_shared<int>(0);
  EXPECT_CALL(*poller, poll)
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Invoke(
          [poll_call_count](const std::vector<std::shared_ptr<rix::GenericSocket>> &all_sockets,
                            const rix::Duration &duration, rix::PollFlag flag,
                            std::vector<std::shared_ptr<rix::GenericSocket>> &sockets,
                            std::vector<std::shared_ptr<rix::GenericSocket>> &exception_sockets) -> bool {
            sockets.clear();
            exception_sockets.clear();
            if (*poll_call_count >= 3) {
              // After 3 polls, simulate no more activity
              return true;
            }
            sockets = all_sockets;
            (*poll_call_count)++;
            return true;
          }));

  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber server
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber connection to rixhub
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber client 1
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber client 2
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber client 3
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Subscriber deregister
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::Endpoint sub_endpoint("127.0.0.1", 0);
  rix::Endpoint sub_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::OPCODE::NODE_REGISTER, false);

  init_sub_server_socket(sockets[1], sub_endpoint, sub_bound_endpoint, 1);

  rix::msg::mediator::SubInfo sub_info;
  sub_info.topic_info.name = "test_topic";
  sub_info.topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_info.endpoint.address = sub_bound_endpoint.address;
  sub_info.endpoint.port = sub_bound_endpoint.port;
  init_sub_register_socket(sockets[2], rixhub_endpoint, sub_info, node_id, false);

  rix::msg::mediator::SubNotify sub_notify;
  sub_notify.error = 0;
  sub_notify.publishers.resize(3);
  sub_notify.publishers[0].topic_info = sub_info.topic_info;
  sub_notify.publishers[0].endpoint.address = "127.0.0.1";
  sub_notify.publishers[0].endpoint.port = 8002;
  sub_notify.publishers[1].topic_info = sub_info.topic_info;
  sub_notify.publishers[1].endpoint.address = "127.0.0.1";
  sub_notify.publishers[1].endpoint.port = 8003;
  sub_notify.publishers[2].topic_info = sub_info.topic_info;
  sub_notify.publishers[2].endpoint.address = "127.0.0.1";
  sub_notify.publishers[2].endpoint.port = 8004;
  init_sub_connection_socket(sockets[3], sub_notify);

  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> messages;
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages[0]->data = 42;
  messages[1]->data = 43;
  messages[2]->data = 44;
  rix::Endpoint pub_endpoint("127.0.0.1", 8002);
  init_sub_client_socket<rix::msg::standard::UInt32>(sockets[4], 3, pub_endpoint, messages);

  pub_endpoint.port = 8003;
  init_sub_client_socket<rix::msg::standard::UInt32>(sockets[5], 3, pub_endpoint, messages);

  pub_endpoint.port = 8004;
  init_sub_client_socket<rix::msg::standard::UInt32>(sockets[6], 3, pub_endpoint, messages);

  init_sub_deregister_socket(sockets[7], rixhub_endpoint, sub_info, node_id);

  init_node_deregister_socket(sockets[8], rixhub_endpoint, node_info);

  // Test creating a node and publisher
  {
    rix::Node node("test_node", rix::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // publisher uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    std::vector<uint32_t> received_data;
    auto callback = [&received_data](const rix::msg::standard::UInt32 &msg) { received_data.push_back(msg.data); };
    auto sub =
        node.create_subscriber<rix::msg::standard::UInt32>("test_topic", callback, rix::Endpoint("127.0.0.1", 0));
    EXPECT_NE(sub, nullptr);
    EXPECT_TRUE(sub->ok());

    auto wrong_callback = [](const rix::msg::standard::Time &msg) { FAIL() << "Should not receive Time message"; };
    sub->set_callback<rix::msg::standard::Time>(wrong_callback);
    EXPECT_TRUE(sub->ok());

#ifndef RIX_MULTITHREADED
    node.spin_once();
    EXPECT_TRUE(sub->ok());
    EXPECT_EQ(sub->get_publisher_count(), 3);
    EXPECT_EQ(received_data.size(), 3);
    EXPECT_EQ(received_data[0], 42); // First message from first publisher
    EXPECT_EQ(received_data[1], 42); // First message from second publisher
    EXPECT_EQ(received_data[2], 42); // First message from third publisher

    node.spin_once();
    EXPECT_TRUE(sub->ok());
    EXPECT_EQ(sub->get_publisher_count(), 3);
    EXPECT_EQ(received_data.size(), 6);
    EXPECT_EQ(received_data[3], 43); // Second message from first publisher
    EXPECT_EQ(received_data[4], 43); // Second message from second publisher
    EXPECT_EQ(received_data[5], 43); // Second message from third publisher

    node.spin_once();
    EXPECT_TRUE(sub->ok());
    EXPECT_EQ(sub->get_publisher_count(), 3);
    EXPECT_EQ(received_data.size(), 9);
    EXPECT_EQ(received_data[6], 44); // Third message from first publisher
    EXPECT_EQ(received_data[7], 44); // Third message from second publisher
    EXPECT_EQ(received_data[8], 44); // Third message from third publisher

#else
    // Give some time for the publisher to accept connections in its own thread
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(sub->ok());
    EXPECT_EQ(sub->get_publisher_count(), 3);
    EXPECT_EQ(received_data.size(), 9);
    EXPECT_EQ(received_data[0], 42); // First message from first publisher
    EXPECT_EQ(received_data[1], 42); // First message from second publisher
    EXPECT_EQ(received_data[2], 42); // First message from third publisher
    EXPECT_EQ(received_data[3], 43); // Second message from first publisher
    EXPECT_EQ(received_data[4], 43); // Second message from second publisher
    EXPECT_EQ(received_data[5], 43); // Second message from third publisher
    EXPECT_EQ(received_data[6], 44); // Third message from first publisher
    EXPECT_EQ(received_data[7], 44); // Third message from second publisher
    EXPECT_EQ(received_data[8], 44); // Third message from third publisher
#endif

    // Shutdown publisher, which should trigger deregistration
    sub->shutdown();
    EXPECT_FALSE(sub->ok());
    sub = nullptr;
    node.spin_once(); // Allow node to process subscriber shutdown
  }

  sockets.clear();
  socket_index = 0;

  poller = nullptr;
  rix::GenericSocket::set_poller(nullptr);
}

TEST(MessageTests, ServiceAcceptRequestAndRespond) {
  auto poller = std::make_shared<rix::MockPoller>();
  rix::GenericSocket::set_poller(poller);

  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service server
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service accept connection 1
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service accept connection 2
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service accept connection 3
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service deregister
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::Endpoint rixhub_endpoint("127.0.0.1", 8000);
  rix::Endpoint srv_endpoint("127.0.0.1", 0);
  rix::Endpoint srv_bound_endpoint("127.0.0.1", 8001);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::OPCODE::NODE_REGISTER, false);

  init_srv_server_socket(sockets[1], srv_endpoint, srv_bound_endpoint, 3);

  rix::msg::mediator::SrvInfo srv_info;
  srv_info.name = "test_topic";
  srv_info.request_hash = rix::msg::standard::UInt32().hash();
  srv_info.response_hash = rix::msg::standard::Time().hash();
  srv_info.endpoint.address = srv_bound_endpoint.address;
  srv_info.endpoint.port = srv_bound_endpoint.port;
  init_srv_register_socket(sockets[2], rixhub_endpoint, srv_info, node_id, false);

  rix::msg::standard::UInt32 request;
  rix::msg::standard::Time response;
  request.data = 1;
  response.sec = 1;
  response.nsec = 501;
  init_srv_connection_socket(sockets[3], request, response);
  request.data = 2;
  response.sec = 2;
  response.nsec = 502;
  init_srv_connection_socket(sockets[4], request, response);
  request.data = 3;
  response.sec = 3;
  response.nsec = 503;
  init_srv_connection_socket(sockets[5], request, response);

  init_srv_deregister_socket(sockets[6], rixhub_endpoint, srv_info, node_id);

  init_node_deregister_socket(sockets[7], rixhub_endpoint, node_info);

  // Test creating a node and service
  {
    rix::Node node("test_node", rix::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    rix::msg::standard::Time response;
    auto callback = [&response](const rix::msg::standard::UInt32 &req, rix::msg::standard::Time &res) {
      response.sec = req.data;        // Just an example implementation
      response.nsec = req.data + 500; // Just an example implementation
    };
    auto srv = node.create_service<rix::msg::standard::UInt32, rix::msg::standard::Time>(
        "test_topic", callback, rix::Endpoint("127.0.0.1", 0));
    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());

    auto wrong_callback = [](const rix::msg::standard::Time &req, rix::msg::standard::UInt32 &res) {
      FAIL() << "Should not be called with wrong message types";
    };
    srv->set_callback<rix::msg::standard::Time, rix::msg::standard::UInt32>(wrong_callback);
    EXPECT_TRUE(srv->ok());

#ifndef RIX_MULTITHREADED
    node.spin_once();
    EXPECT_TRUE(srv->ok());
    EXPECT_EQ(response.sec, 1);
    EXPECT_EQ(response.nsec, 501);

    node.spin_once();
    EXPECT_TRUE(srv->ok());
    EXPECT_EQ(response.sec, 2);
    EXPECT_EQ(response.nsec, 502);

    node.spin_once();
    EXPECT_TRUE(srv->ok());
    EXPECT_EQ(response.sec, 3);
    EXPECT_EQ(response.nsec, 503);
#else
    // Give some time for the service to accept connections in its own thread
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(srv->ok());
#endif

    // Shutdown service, which should trigger deregistration
    srv->shutdown();
    EXPECT_FALSE(srv->ok());
    srv = nullptr;
    node.spin_once(); // Allow node to process service shutdown
  }

  sockets.clear();
  socket_index = 0;

  poller = nullptr;
  rix::GenericSocket::set_poller(nullptr);
}

TEST(MessageTests, ServiceClientRequestAndReceive) {
  auto poller = std::make_shared<rix::MockPoller>();
  rix::GenericSocket::set_poller(poller);

  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node register
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service Client request
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service Client client 1
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service Client client 2
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Service Client client 3
  sockets.push_back(std::make_shared<rix::MockSocket>()); // Node deregister

  uint64_t node_id = 0;
  rix::Endpoint rixhub_endpoint("127.0.0.1", 8000);

  // Node register client socket will be used to send a register message to rixhub
  rix::msg::mediator::NodeInfo node_info;
  node_info.name = "test_node";
  init_node_register_socket(sockets[0], rixhub_endpoint, node_info, node_id, rix::OPCODE::NODE_REGISTER, false);

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

  rix::Endpoint srv_endpoint("127.0.0.1", 8001);
  rix::msg::standard::UInt32 request;
  rix::msg::standard::Time response;
  request.data = 1;
  response.sec = 1;
  response.nsec = 501;
  init_srvcli_client_socket(sockets[2], srv_endpoint, request, response);
  request.data = 2;
  response.sec = 2;
  response.nsec = 502;
  init_srvcli_client_socket(sockets[3], srv_endpoint, request, response);
  request.data = 3;
  response.sec = 3;
  response.nsec = 503;
  init_srvcli_client_socket(sockets[4], srv_endpoint, request, response);

  init_node_deregister_socket(sockets[5], rixhub_endpoint, node_info);

  // Test creating a node and subscriber
  {
    rix::Node node("test_node", rix::Endpoint("127.0.0.1", 8000), mock_create_socket);
    EXPECT_TRUE(node.ok());

    // Specify endpoint as different than the one that will be assigned by the OS to ensure that the
    // service uses the correct endpoint (the one returned by the server socket's local_endpoint() method).
    auto srvcli = node.create_service_client<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_service");
    EXPECT_NE(srvcli, nullptr);
    EXPECT_TRUE(srvcli->ok());

    rix::msg::standard::UInt32 request;
    rix::msg::standard::Time response;
    request.data = 1;
    bool call_result = srvcli->call(request, response);
    EXPECT_TRUE(call_result);
    EXPECT_EQ(response.sec, 1);
    EXPECT_EQ(response.nsec, 501);

    request.data = 2;
    call_result = srvcli->call(request, response);
    EXPECT_TRUE(call_result);
    EXPECT_EQ(response.sec, 2);
    EXPECT_EQ(response.nsec, 502);

    request.data = 3;
    call_result = srvcli->call(request, response);
    EXPECT_TRUE(call_result);
    EXPECT_EQ(response.sec, 3);
    EXPECT_EQ(response.nsec, 503);

    // Shutdown service client, which should not trigger a deregistration
    srvcli->shutdown();
    EXPECT_FALSE(srvcli->ok());
    srvcli = nullptr;
    node.spin_once(); // Allow node to process service client shutdown
  }

  sockets.clear();
  socket_index = 0;

  poller = nullptr;
  rix::GenericSocket::set_poller(nullptr);
}