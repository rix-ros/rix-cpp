#include "rix/test/test_fixture.hpp"
#include "simple_publisher.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimplePublisherTest, Create) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("simple_publisher", node_info)
      .create_publisher<std_msgs::Header>("/chatter", pub_info, node_info)
      .destroy_node(node_info)
      .destroy_publisher(pub_info)
      .build<SimplePublisher>([](TestFixture& fixture) {
        SimplePublisher node(1.0, 0);
        EXPECT_TRUE(node.ok());
      });
}

TEST(SimplePublisherTest, CreateNodeRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_publisher", node_info, true) // Simulate failure
      .build<SimplePublisher>([](TestFixture& fixture) {
        SimplePublisher node(1.0, 0);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimplePublisherTest, CreatePublisherRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("simple_publisher", node_info)
      .create_publisher<std_msgs::Header>("/chatter", pub_info, node_info, true) // Simulate failure
      .destroy_node(node_info)
      .build<SimplePublisher>([](TestFixture& fixture) {
        SimplePublisher node(1.0, 0);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimplePublisherTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<std_msgs::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<std_msgs::Header>();
    msg->seq = i + 1;
    msg->frame_id = "Hello, world!";
    msg->stamp.sec = i + 1;
    msg->stamp.nsec = 0;
    messages.push_back(msg);
  }

  std::shared_ptr<MockClock> clock;
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .enable_clock(clock)
      .create_node("simple_publisher", node_info)
      .create_publisher<std_msgs::Header>("/chatter", pub_info, node_info, false, 1)
      .enable_operation_notifications()
      .accept_subscriber(messages)
      .disable_operation_notifications()
      .destroy_node(node_info)
      .destroy_publisher(pub_info)
      .build<SimplePublisher>([clock](TestFixture& fixture) {
        SimplePublisher node(1.0, 0);
        auto conn = fixture.get_connection_socket(0);

        EXPECT_TRUE(node.ok());
        for (int i = 0; i < 5; i++) {
          clock->sleep_for(Duration(1.0));
          node.spin_once();
          EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
        }
      });
}

// Recommended way to run tests for multithreaded nodes that use poller
TEST(SimplePublisherTest, SpinWithPollerAndOperationNotifications) {
  std::vector<std::shared_ptr<std_msgs::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<std_msgs::Header>();
    msg->seq = i + 1;
    msg->frame_id = "Hello, world!";
    msg->stamp.sec = i + 1;
    msg->stamp.nsec = 0;
    messages.push_back(msg);
  }

  std::shared_ptr<MockClock> clock;
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .enable_clock(clock)
      .enable_poller(5)
      .enable_operation_notifications()
      .create_node("simple_publisher", node_info)
      .create_publisher<std_msgs::Header>("/chatter", pub_info, node_info, false, 1)
      .accept_subscriber(messages)
      .destroy_node(node_info)
      .destroy_publisher(pub_info)
      .build<SimplePublisher>([clock](TestFixture& fixture) {
        SimplePublisher node(1.0, 0);
        auto conn = fixture.get_connection_socket(0);

        EXPECT_TRUE(node.ok());
        for (int i = 0; i < 5; i++) {
          clock->sleep_for(Duration(1.0));
          node.spin_once();
          EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
        }
      });
}
