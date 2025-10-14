#include "rix/core/node.hpp"
#include "rix/msg/mediator/SubNotify.hpp"
#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/Time.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/test/node_test_fixture.hpp"
#include <gtest/gtest.h>

TEST(MessageTest, PublisherAcceptConnectionsAndPublish) {
  // Create messages to publish
  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> messages;
  for (int i = 0; i < 3; ++i) {
    auto msg = std::make_shared<rix::msg::standard::UInt32>();
    msg->data = 42;
    messages.push_back(msg);
  }

  rix::NodeTestFixture()
      .enable_poller(3)
      .enable_operation_notifications()
      .register_node()
      .register_publisher<rix::msg::standard::UInt32>("test_topic", false, 3)
      .create_pub_connection(messages)
      .create_pub_connection(messages)
      .create_pub_connection(messages)
      .deregister_publisher<rix::msg::standard::UInt32>("test_topic")
      .deregister_node()
      .build<rix::Node>(
          [](rix::NodeTestFixture& fixture, std::unique_ptr<rix::Node> node) {
            auto server_socket = fixture.get_server_socket();
            auto connection_sockets = fixture.get_connection_sockets();

            EXPECT_TRUE(node->ok());

            // Create publisher with specified endpoint
            auto pub = node->create_publisher<rix::msg::standard::UInt32>("test_topic");
            EXPECT_NE(pub, nullptr);
            EXPECT_TRUE(pub->ok());

#ifndef RIX_MULTITHREADED
            // Process connection acceptances
            node->spin_once();
            EXPECT_TRUE(pub->ok());
            EXPECT_EQ(pub->get_subscriber_count(), 1);

            node->spin_once();
            EXPECT_TRUE(pub->ok());
            EXPECT_EQ(pub->get_subscriber_count(), 2);

            node->spin_once();
            EXPECT_TRUE(pub->ok());
            EXPECT_EQ(pub->get_subscriber_count(), 3);
#else
            // Wait for all 3 connections to be accepted (with timeout)
            EXPECT_TRUE(server_socket->wait_for_operations(3, std::chrono::milliseconds(5000)));
            EXPECT_TRUE(pub->ok());
            EXPECT_EQ(pub->get_subscriber_count(), 3);
#endif

            // Publish messages
            auto msg = std::make_shared<rix::msg::standard::UInt32>();
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
            auto wrong_msg = std::make_shared<rix::msg::standard::Time>();
            pub->publish(*wrong_msg);

            // Shutdown publisher
            pub->shutdown();
            EXPECT_FALSE(pub->ok());
            pub = nullptr;
            node->spin_once();
          },
          "test_node");
}

TEST(MessageTest, SubscriberConnectAndReceive) {
  // Create messages to receive
  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> messages;
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages[0]->data = 42;
  messages[1]->data = 43;
  messages[2]->data = 44;

  rix::NodeTestFixture()
      .enable_poller(3)
      .enable_operation_notifications()
      .register_node()
      .register_subscriber<rix::msg::standard::UInt32>("test_topic", false, 1)
      .create_sub_connection<rix::msg::standard::UInt32>(
          "test_topic",
          {rix::Endpoint("127.0.0.1", 8002), rix::Endpoint("127.0.0.1", 8003), rix::Endpoint("127.0.0.1", 8004)})
      .create_sub_client(rix::Endpoint("127.0.0.1", 8002), messages)
      .create_sub_client(rix::Endpoint("127.0.0.1", 8003), messages)
      .create_sub_client(rix::Endpoint("127.0.0.1", 8004), messages)
      .deregister_subscriber<rix::msg::standard::UInt32>("test_topic")
      .deregister_node()
      .build<rix::Node>(
          [](rix::NodeTestFixture& fixture, std::unique_ptr<rix::Node> node) {
            auto sub_clients = fixture.get_client_sockets();

            EXPECT_TRUE(node->ok());

            // Track received messages
            std::vector<uint32_t> received_data;
            auto callback = [&received_data](const rix::msg::standard::UInt32& msg) {
              received_data.push_back(msg.data);
            };

            // Create subscriber
            auto sub = node->create_subscriber<rix::msg::standard::UInt32>("test_topic", callback);
            EXPECT_NE(sub, nullptr);
            EXPECT_TRUE(sub->ok());

            // Set wrong callback (should not be called)
            auto wrong_callback = [](const rix::msg::standard::Time& msg) {
              FAIL() << "Should not receive Time message";
            };
            sub->set_callback<rix::msg::standard::Time>(wrong_callback);
            EXPECT_TRUE(sub->ok());

#ifndef RIX_MULTITHREADED
            // Process messages
            node->spin_once();
            EXPECT_TRUE(sub->ok());
            EXPECT_EQ(sub->get_publisher_count(), 3);
            EXPECT_EQ(received_data.size(), 3);
            EXPECT_EQ(received_data[0], 42);
            EXPECT_EQ(received_data[1], 42);
            EXPECT_EQ(received_data[2], 42);

            node->spin_once();
            EXPECT_TRUE(sub->ok());
            EXPECT_EQ(sub->get_publisher_count(), 3);
            EXPECT_EQ(received_data.size(), 6);
            EXPECT_EQ(received_data[3], 43);
            EXPECT_EQ(received_data[4], 43);
            EXPECT_EQ(received_data[5], 43);

            node->spin_once();
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
            node->spin_once();
          },
          "test_node");
}

TEST(MessageTest, ServiceAcceptRequestAndRespond) {
  // Create request/response pairs
  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> requests;
  std::vector<std::shared_ptr<rix::msg::standard::Time>> responses;
  for (int i = 1; i <= 3; ++i) {
    auto req = std::make_shared<rix::msg::standard::UInt32>();
    auto res = std::make_shared<rix::msg::standard::Time>();
    req->data = i;
    res->sec = i;
    res->nsec = i + 500;
    requests.push_back(req);
    responses.push_back(res);
  }

  rix::NodeTestFixture()
      .enable_operation_notifications()
      .register_node()
      .register_service<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_topic", false, 3)
      .create_srv_connection(requests[0], responses[0])
      .create_srv_connection(requests[1], responses[1])
      .create_srv_connection(requests[2], responses[2])
      .deregister_service<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_topic")
      .deregister_node()
      .build<rix::Node>(
          [](rix::NodeTestFixture& fixture, std::unique_ptr<rix::Node> node) {
            auto server_socket = fixture.get_server_socket();
            auto srv_connections = fixture.get_connection_sockets();

            EXPECT_TRUE(node->ok());

            // Create service with callback
            rix::msg::standard::Time response;
            auto callback = [&response](const rix::msg::standard::UInt32& req, rix::msg::standard::Time& res) {
              response.sec = req.data;
              response.nsec = req.data + 500;
            };

            auto srv =
                node->create_service<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_topic", callback);
            EXPECT_NE(srv, nullptr);
            EXPECT_TRUE(srv->ok());

            // Set wrong callback (should not be called)
            auto wrong_callback = [](const rix::msg::standard::Time& req, rix::msg::standard::UInt32& res) {
              FAIL() << "Should not be called with wrong message types";
            };
            srv->set_callback<rix::msg::standard::Time, rix::msg::standard::UInt32>(wrong_callback);
            EXPECT_TRUE(srv->ok());

#ifndef RIX_MULTITHREADED
            // Process requests
            node->spin_once();
            EXPECT_TRUE(srv->ok());
            EXPECT_EQ(response.sec, 1);
            EXPECT_EQ(response.nsec, 501);

            node->spin_once();
            EXPECT_TRUE(srv->ok());
            EXPECT_EQ(response.sec, 2);
            EXPECT_EQ(response.nsec, 502);

            node->spin_once();
            EXPECT_TRUE(srv->ok());
            EXPECT_EQ(response.sec, 3);
            EXPECT_EQ(response.nsec, 503);
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
            node->spin_once();
          },
          "test_node");
}

TEST(MessageTest, ServiceClientRequestAndReceive) {
  // Create request/response pairs
  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> requests;
  std::vector<std::shared_ptr<rix::msg::standard::Time>> responses;
  for (int i = 1; i <= 3; ++i) {
    auto req = std::make_shared<rix::msg::standard::UInt32>();
    auto res = std::make_shared<rix::msg::standard::Time>();
    req->data = i;
    res->sec = i;
    res->nsec = i + 500;
    requests.push_back(req);
    responses.push_back(res);
  }

  rix::NodeTestFixture()
      .register_node()
      .request_service_client<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_service")
      .create_srv_cli_client(requests[0], responses[0])
      .create_srv_cli_client(requests[1], responses[1])
      .create_srv_cli_client(requests[2], responses[2])
      .deregister_node()
      .build<rix::Node>(
          [](rix::NodeTestFixture& fixture, std::unique_ptr<rix::Node> node) {
            EXPECT_TRUE(node->ok());

            // Create service client
            auto srvcli =
                node->create_service_client<rix::msg::standard::UInt32, rix::msg::standard::Time>("test_service");
            EXPECT_NE(srvcli, nullptr);
            EXPECT_TRUE(srvcli->ok());

            // Make service calls
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

            // Shutdown service client
            srvcli->shutdown();
            EXPECT_FALSE(srvcli->ok());
            srvcli = nullptr;
            node->spin_once();
          },
          "test_node");
}
