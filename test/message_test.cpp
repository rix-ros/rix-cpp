#include "rix/core/node.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/SubNotify.hpp"
#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/Time.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/test/node_test_fixture.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(MessageTest, PublisherAcceptConnectionsAndPublish) {
  // Create messages to publish
  std::vector<std::shared_ptr<msg::standard::UInt32>> messages;
  for (int i = 0; i < 3; ++i) {
    auto msg = std::make_shared<msg::standard::UInt32>();
    msg->data = 42;
    messages.push_back(msg);
  }

  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  TestFixture()
      .enable_poller(3)
      .enable_operation_notifications()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info, false, 3)
      .accept_subscriber(messages)
      .accept_subscriber(messages)
      .accept_subscriber(messages)
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto connection_sockets = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create publisher with specified endpoint
        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

#ifndef RIX_MULTITHREADED
        // Process connection acceptances
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
        // Wait for all 3 connections to be accepted (with timeout)
        EXPECT_TRUE(server_socket->wait_for_operations(3, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(pub->ok());
        EXPECT_EQ(pub->get_subscriber_count(), 3);
#endif

        // Publish messages
        auto msg = std::make_shared<msg::standard::UInt32>();
        msg->data = 42;
        pub->publish(*msg);
        pub->publish(*msg);
        pub->publish(*msg);

#ifdef RIX_MULTITHREADED
        // Wait for all messages to be sent on each connection
        // Each connection socket will notify 3 times (once per message)
        EXPECT_TRUE(fixture.wait_for_all_connections(3, std::chrono::milliseconds(5000)));
#endif

        // Try publishing wrong message type (should not be sent)
        auto wrong_msg = std::make_shared<msg::standard::Time>();
        pub->publish(*wrong_msg);

        // Shutdown publisher
        pub->shutdown();
        EXPECT_FALSE(pub->ok());
        pub = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, SubscriberConnectAndReceive) {
  // Create messages to receive
  std::vector<std::shared_ptr<msg::standard::UInt32>> messages;
  messages.push_back(std::make_shared<msg::standard::UInt32>());
  messages.push_back(std::make_shared<msg::standard::UInt32>());
  messages.push_back(std::make_shared<msg::standard::UInt32>());
  messages[0]->data = 42;
  messages[1]->data = 43;
  messages[2]->data = 44;

  msg::mediator::NodeInfo node_info;
  msg::mediator::SubInfo sub_info;
  TestFixture()
      .enable_poller(3)
      .create_node("test_node", node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info, false, 1)
      .accept_notification<msg::standard::UInt32>(
        "test_topic", {Endpoint("127.0.0.1", 8002), Endpoint("127.0.0.1", 8003), Endpoint("127.0.0.1", 8004)})
      .enable_operation_notifications()
      .connect_to_publisher(Endpoint("127.0.0.1", 8002), messages)
      .connect_to_publisher(Endpoint("127.0.0.1", 8003), messages)
      .connect_to_publisher(Endpoint("127.0.0.1", 8004), messages)
      .disable_operation_notifications()
      .destroy_subscriber(sub_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        auto sub_clients = fixture.get_client_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Track received messages
        std::vector<uint32_t> received_data;
        auto callback = [&received_data](const msg::standard::UInt32& msg) { received_data.push_back(msg.data); };

        // Create subscriber
        auto sub = node.create_subscriber<msg::standard::UInt32>("test_topic", callback);
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback = [](const msg::standard::Time& msg) { FAIL() << "Should not receive Time message"; };
        sub->set_callback<msg::standard::Time>(wrong_callback);
        EXPECT_TRUE(sub->ok());

#ifndef RIX_MULTITHREADED
        // Process messages
        node.spin_once();
        EXPECT_TRUE(sub->ok());
        EXPECT_EQ(sub->get_publisher_count(), 3);
        EXPECT_EQ(received_data.size(), 3);
        EXPECT_EQ(received_data[0], 42);
        EXPECT_EQ(received_data[1], 42);
        EXPECT_EQ(received_data[2], 42);

        node.spin_once();
        EXPECT_TRUE(sub->ok());
        EXPECT_EQ(sub->get_publisher_count(), 3);
        EXPECT_EQ(received_data.size(), 6);
        EXPECT_EQ(received_data[3], 43);
        EXPECT_EQ(received_data[4], 43);
        EXPECT_EQ(received_data[5], 43);

        node.spin_once();
        EXPECT_TRUE(sub->ok());
        EXPECT_EQ(sub->get_publisher_count(), 3);
        EXPECT_EQ(received_data.size(), 9);
        EXPECT_EQ(received_data[6], 44);
        EXPECT_EQ(received_data[7], 44);
        EXPECT_EQ(received_data[8], 44);
#else
        // Wait for all 9 messages to be received (3 messages from 3 clients)
        // Each client socket will notify once per message
        EXPECT_TRUE(fixture.wait_for_all_clients(3, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(sub->ok());
        EXPECT_EQ(sub->get_publisher_count(), 3);
        EXPECT_EQ(received_data.size(), 9);
        EXPECT_EQ(received_data[0], 42);
        EXPECT_EQ(received_data[1], 42);
        EXPECT_EQ(received_data[2], 42);
        EXPECT_EQ(received_data[3], 43);
        EXPECT_EQ(received_data[4], 43);
        EXPECT_EQ(received_data[5], 43);
        EXPECT_EQ(received_data[6], 44);
        EXPECT_EQ(received_data[7], 44);
        EXPECT_EQ(received_data[8], 44);
#endif

        // Shutdown subscriber
        sub->shutdown();
        EXPECT_FALSE(sub->ok());
        sub = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, ServiceAcceptRequestAndRespond) {
  // Create request/response pairs
  std::vector<std::shared_ptr<msg::standard::UInt32>> requests;
  std::vector<std::shared_ptr<msg::standard::Time>> responses;
  for (int i = 1; i <= 3; ++i) {
    auto req = std::make_shared<msg::standard::UInt32>();
    auto res = std::make_shared<msg::standard::Time>();
    req->data = i;
    res->sec = i;
    res->nsec = i + 500;
    requests.push_back(req);
    responses.push_back(res);
  }

  msg::mediator::NodeInfo node_info;
  msg::mediator::SrvInfo srv_info;
  TestFixture()
      .enable_operation_notifications()
      .create_node("test_node", node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_topic", srv_info, node_info, false, 3)
      .accept_service_client(requests[0], responses[0])
      .accept_service_client(requests[1], responses[1])
      .accept_service_client(requests[2], responses[2])
      .destroy_service(srv_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto srv_connections = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto callback = [](const msg::standard::UInt32& req, msg::standard::Time& res) {
          res.sec = req.data;
          res.nsec = req.data + 500;
        };

        auto srv = node.create_service<msg::standard::UInt32, msg::standard::Time>("test_topic", callback);
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback = [](const msg::standard::Time& req, msg::standard::UInt32& res) {
          FAIL() << "Should not be called with wrong message types";
        };
        srv->set_callback<msg::standard::Time, msg::standard::UInt32>(wrong_callback);
        EXPECT_TRUE(srv->ok());

#ifndef RIX_MULTITHREADED
        // Process requests
        node.spin_once();
        EXPECT_TRUE(srv->ok());

        node.spin_once();
        EXPECT_TRUE(srv->ok());

        node.spin_once();
        EXPECT_TRUE(srv->ok());
#else
        // Wait for server to accept all 3 connections first
        EXPECT_TRUE(server_socket->wait_for_operations(3, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(fixture.wait_for_all_connections(1, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(srv->ok());
#endif

        // Shutdown service
        srv->shutdown();
        EXPECT_FALSE(srv->ok());
        srv = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, ServiceClientRequestAndReceive) {
  // Create request/response pairs
  std::vector<std::shared_ptr<msg::standard::UInt32>> requests;
  std::vector<std::shared_ptr<msg::standard::Time>> responses;
  for (int i = 1; i <= 3; ++i) {
    auto req = std::make_shared<msg::standard::UInt32>();
    auto res = std::make_shared<msg::standard::Time>();
    req->data = i;
    res->sec = i;
    res->nsec = i + 500;
    requests.push_back(req);
    responses.push_back(res);
  }

  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info)
      .call_service_client(requests[0], responses[0])
      .call_service_client(requests[1], responses[1])
      .call_service_client(requests[2], responses[2])
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service client
        auto srvcli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");
        EXPECT_NE(srvcli, nullptr);
        EXPECT_TRUE(srvcli->ok());

        // Make service calls
        msg::standard::UInt32 request;
        msg::standard::Time response;

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

        // Shutdown service client
        srvcli->shutdown();
        EXPECT_FALSE(srvcli->ok());
        srvcli = nullptr;
        node.spin_once();
      });
}
