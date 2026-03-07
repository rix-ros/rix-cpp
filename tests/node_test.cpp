/**
 * @file node_test_new.cpp
 * @brief Example tests using Fixture 1 (TestFixture from test_fixture_new.hpp).
 *
 * These tests exercise Node's IPC lifecycle through the updated Acceptor/Stream
 * mock infrastructure. The API mirrors the old TestFixture but uses MockAcceptor
 * and MockStream instead of the deleted GenericSocket/MockSocket.
 */

#include "rix/std_msgs/Header.hpp"
#include "rix/std_msgs/String.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/test/test_fixture.hpp"
#include <gtest/gtest.h>

using namespace rix;

// =============================================================================
// Basic Node registration / deregistration
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterNode) {
  sys_msgs::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info).destroy_node(node_info).build<Node>([](const TestFixture&) {
    Node node("test_node");
    EXPECT_TRUE(node.ok());
  });
}

TEST(NodeTestNew, RegisterNodeFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info, true).build<Node>([](const TestFixture&) {
    Node node("test_node");
    EXPECT_FALSE(node.ok());
  });
}

// =============================================================================
// Publisher lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterPublisher) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .destroy_node(node_info)
      .destroy_publisher(pub_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
      });
}

TEST(NodeTestNew, RegisterPublisherFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_FALSE(pub->ok());
      });
}

// =============================================================================
// Subscriber lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterSubscriber) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .destroy_node(node_info)
      .destroy_subscriber(sub_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
      });
}

// =============================================================================
// Service lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterService) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .destroy_node(node_info)
      .destroy_service(srv_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
      });
}

// =============================================================================
// Service client
// =============================================================================

TEST(NodeTestNew, RequestServiceClient) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
      });
}

// =============================================================================
// Action lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterAction) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
      });
}

// =============================================================================
// Publisher with messages (using clock + operation notifications)
// =============================================================================

TEST(NodeTestNew, PublisherSendsMessages) {
  // Prepare expected messages
  std::vector<std::shared_ptr<std_msgs::Header>> messages;
  for (int i = 0; i < 3; i++) {
    auto msg = std::make_shared<std_msgs::Header>();
    msg->seq = i + 1;
    msg->frame_id = "test_frame";
    msg->stamp.sec = i + 1;
    msg->stamp.nsec = 0;
    messages.push_back(msg);
  }

  std::shared_ptr<MockClock> clock;
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  std::shared_ptr<MockStream> conn;
  TestFixture()
      .enable_clock(clock)
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::Header>("test_topic", pub_info, node_info, false, 1)
      .enable_operation_notifications()
      .accept_subscriber(messages, conn)
      .disable_operation_notifications()
      .destroy_node(node_info)
      .destroy_publisher(pub_info)
      .build<Node>([&clock, &conn](const TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::Header>("test_topic");
        EXPECT_TRUE(pub->ok());

        // Accept the subscriber connection (Publisher::on_spin)
        node.spin_once();

        // Simulate timed message publication
        std_msgs::Header msg;
        msg.frame_id = "test_frame";
        for (int i = 0; i < 3; i++) {
          clock->sleep_for(Duration(1.0));
          msg.seq = i + 1;
          msg.stamp.sec = i + 1;
          msg.stamp.nsec = 0;
          pub->publish(msg);
          EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1000)));
        }
      });
}

// =============================================================================
// Multiple components
// =============================================================================

TEST(NodeTestNew, RegisterMultipleComponents) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  sys_msgs::SubInfo sub_info;
  sys_msgs::SrvInfo srv_info;
  sys_msgs::ActInfo act_info;

  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .destroy_publisher(pub_info)
      .destroy_subscriber(sub_info)
      .destroy_service(srv_info)
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        // Shutdown components manually
        pub->shutdown();
        node.spin_once();
        pub = nullptr;

        sub->shutdown();
        node.spin_once();
        sub = nullptr;

        srv->shutdown();
        node.spin_once();
        srv = nullptr;

        act->shutdown();
        node.spin_once();
        act = nullptr;

        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Ping handling
// =============================================================================

TEST(NodeTestNew, PingNode) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info, false, 1)
      .accept_ping(node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());
        node.spin_once();
      });
}
