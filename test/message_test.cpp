#include "rix/core/node.hpp"
#include "rix/msg/mediator/SubNotify.hpp"
#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/Time.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/test/node_test_fixture.hpp"
#include <gtest/gtest.h>

TEST(MessageTest, PublisherAcceptConnectionsAndPublish) {
  rix::Endpoint pub_endpoint("127.0.0.1", 0);
  rix::Endpoint pub_bound_endpoint("127.0.0.1", 8001);

  // Create messages to publish
  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> messages;
  for (int i = 0; i < 3; ++i) {
    auto msg = std::make_shared<rix::msg::standard::UInt32>();
    msg->data = 42;
    messages.push_back(msg);
  }

  rix::NodeTestFixture<rix::Node>::enable_poller(3);
  auto fixture = rix::NodeTestFixture()
                     .enable_operation_notifications()
                     .register_node()
                     .create_server(pub_endpoint, pub_bound_endpoint, 3)
                     .register_publisher("test_topic",
                                         rix::msg::standard::UInt32().hash(),
                                         false,
                                         pub_bound_endpoint)
                     .create_pub_connection<rix::msg::standard::UInt32>(
                         3, // 3 messages per connection
                         messages)
                     .create_pub_connection<rix::msg::standard::UInt32>(
                         3, // 3 messages per connection
                         messages)
                     .create_pub_connection<rix::msg::standard::UInt32>(
                         3, // 3 messages per connection
                         messages)
                     .deregister_publisher("test_topic",
                                           rix::msg::standard::UInt32().hash(),
                                           pub_bound_endpoint)
                     .deregister_node();

  {
    auto server_socket = fixture.get_server_socket();
    auto connection_sockets = fixture.get_connection_sockets();

    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    // Create publisher with specified endpoint
    auto pub =
        node->create_publisher<rix::msg::standard::UInt32>("test_topic", pub_endpoint);
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
  }
  rix::NodeTestFixture<rix::Node>::disable_poller();
}

TEST(MessageTest, SubscriberConnectAndReceive) {
  rix::NodeTestFixture<rix::Node>::enable_poller(3);

  rix::Endpoint sub_endpoint("127.0.0.1", 0);
  rix::Endpoint sub_bound_endpoint("127.0.0.1", 8001);

  // Create messages to receive
  std::vector<std::shared_ptr<rix::msg::standard::UInt32>> messages;
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages.push_back(std::make_shared<rix::msg::standard::UInt32>());
  messages[0]->data = 42;
  messages[1]->data = 43;
  messages[2]->data = 44;

  // Create subscriber notification
  rix::msg::mediator::SubNotify sub_notify;
  sub_notify.publishers.resize(3);
  sub_notify.publishers[0].id = 1;
  sub_notify.publishers[0].node_id = 1;
  sub_notify.publishers[0].topic_info.name = "test_topic";
  sub_notify.publishers[0].topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_notify.publishers[0].endpoint.address = "127.0.0.1";
  sub_notify.publishers[0].endpoint.port = 8002;

  sub_notify.publishers[1].id = 2;
  sub_notify.publishers[1].node_id = 1;
  sub_notify.publishers[1].topic_info.name = "test_topic";
  sub_notify.publishers[1].topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_notify.publishers[1].endpoint.address = "127.0.0.1";
  sub_notify.publishers[1].endpoint.port = 8003;

  sub_notify.publishers[2].id = 3;
  sub_notify.publishers[2].node_id = 1;
  sub_notify.publishers[2].topic_info.name = "test_topic";
  sub_notify.publishers[2].topic_info.message_hash = rix::msg::standard::UInt32().hash();
  sub_notify.publishers[2].endpoint.address = "127.0.0.1";
  sub_notify.publishers[2].endpoint.port = 8004;

  auto fixture = rix::NodeTestFixture()
                     .enable_operation_notifications()
                     .register_node()
                     .create_server(sub_endpoint, sub_bound_endpoint, 1)
                     .register_subscriber("test_topic",
                                          rix::msg::standard::UInt32().hash(),
                                          false,
                                          sub_bound_endpoint)
                     .create_sub_connection(sub_notify)
                     .create_sub_client(rix::Endpoint("127.0.0.1", 8002), 3, messages)
                     .create_sub_client(rix::Endpoint("127.0.0.1", 8003), 3, messages)
                     .create_sub_client(rix::Endpoint("127.0.0.1", 8004), 3, messages)
                     .deregister_subscriber("test_topic",
                                            rix::msg::standard::UInt32().hash(),
                                            sub_bound_endpoint)
                     .deregister_node();

  {
    auto sub_clients = fixture.get_client_sockets();

    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    // Track received messages
    std::vector<uint32_t> received_data;
    auto callback = [&received_data](const rix::msg::standard::UInt32& msg) {
      received_data.push_back(msg.data);
    };

    // Create subscriber
    auto sub = node->create_subscriber<rix::msg::standard::UInt32>(
        "test_topic", callback, sub_endpoint);
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
  }
  rix::NodeTestFixture<rix::Node>::disable_poller();
}

TEST(MessageTest, ServiceAcceptRequestAndRespond) {
  rix::Endpoint srv_endpoint("127.0.0.1", 0);
  rix::Endpoint srv_bound_endpoint("127.0.0.1", 8001);

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

  auto fixture = rix::NodeTestFixture()
                     .enable_operation_notifications()
                     .register_node()
                     .create_server(srv_endpoint, srv_bound_endpoint, 3)
                     .register_service("test_topic",
                                       rix::msg::standard::UInt32().hash(),
                                       rix::msg::standard::Time().hash(),
                                       false,
                                       srv_bound_endpoint)
                     .create_srv_connection(requests[0], responses[0])
                     .create_srv_connection(requests[1], responses[1])
                     .create_srv_connection(requests[2], responses[2])
                     .deregister_service("test_topic",
                                         rix::msg::standard::UInt32().hash(),
                                         rix::msg::standard::Time().hash(),
                                         srv_bound_endpoint)
                     .deregister_node();

  {
    auto server_socket = fixture.get_server_socket();
    auto srv_connections = fixture.get_connection_sockets();

    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    // Create service with callback
    rix::msg::standard::Time response;
    auto callback = [&response](const rix::msg::standard::UInt32& req,
                                rix::msg::standard::Time& res) {
      response.sec = req.data;
      response.nsec = req.data + 500;
    };

    auto srv = node->create_service<rix::msg::standard::UInt32,
    rix::msg::standard::Time>(
        "test_topic", callback, srv_endpoint);
    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());

    // Set wrong callback (should not be called)
    auto wrong_callback = [](const rix::msg::standard::Time& req,
                             rix::msg::standard::UInt32& res) {
      FAIL() << "Should not be called with wrong message types";
    };
    srv->set_callback<rix::msg::standard::Time, rix::msg::standard::UInt32>(
        wrong_callback);
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
    EXPECT_TRUE(server_socket->wait_for_operations(3,
    std::chrono::milliseconds(5000))); EXPECT_TRUE(fixture.wait_for_all_connections(1,
    std::chrono::milliseconds(5000))); EXPECT_TRUE(srv->ok());
#endif

    // Shutdown service
    srv->shutdown();
    EXPECT_FALSE(srv->ok());
    srv = nullptr;
    node->spin_once();
  }
}

TEST(MessageTest, ServiceClientRequestAndReceive) {
  rix::Endpoint srv_endpoint("127.0.0.1", 8001);

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

  auto fixture = rix::NodeTestFixture()
                     .register_node()
                     .request_service_client("test_service",
                                             rix::msg::standard::UInt32().hash(),
                                             rix::msg::standard::Time().hash(),
                                             false,
                                             srv_endpoint)
                     .create_srv_cli_client(srv_endpoint, requests[0], responses[0])
                     .create_srv_cli_client(srv_endpoint, requests[1], responses[1])
                     .create_srv_cli_client(srv_endpoint, requests[2], responses[2])
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    // Create service client
    auto srvcli =
        node->create_service_client<rix::msg::standard::UInt32,
        rix::msg::standard::Time>(
            "test_service");
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
  }
}
