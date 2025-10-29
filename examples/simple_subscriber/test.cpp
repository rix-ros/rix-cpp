#include "rix/test/test_fixture.hpp"
#include "simple_subscriber.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleSubscriberTest, Create) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .create_node("simple_subscriber", node_info)
      .create_subscriber<std_msgs::Header>("/chatter", sub_info, node_info)
      .destroy_node(node_info)
      .destroy_subscriber(sub_info)
      .build<SimpleSubscriber>([](TestFixture& fixture) {
        SimpleSubscriber node(0);
        EXPECT_TRUE(node.ok());
      });
}

TEST(SimpleSubscriberTest, CreateNodeRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_subscriber", node_info, true) // Simulate failure
      .build<SimpleSubscriber>([](TestFixture& fixture) {
        SimpleSubscriber node(0);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleSubscriberTest, CreatePublisherRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .create_node("simple_subscriber", node_info)
      .create_subscriber<std_msgs::Header>("/chatter", sub_info, node_info, true) // Simulate failure
      .destroy_node(node_info)
      .build<SimpleSubscriber>([](TestFixture& fixture) {
        SimpleSubscriber node(0);
        EXPECT_FALSE(node.ok());
      });
}

// Recommended way to run tests for single-threaded nodes that do not use poller
TEST(SimpleSubscriberTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<std_msgs::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<std_msgs::Header>();
    msg->seq = i;
    msg->frame_id = "Hello, world!";
    messages.push_back(msg);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .enable_operation_notifications()
      .create_node("simple_subscriber", node_info)
      .create_subscriber<std_msgs::Header>("/chatter", sub_info, node_info, false, 1)
      .accept_notification<std_msgs::Header>("/chatter", {Endpoint(DEFAULT_IP, 8001)})
      .connect_to_publisher(Endpoint(DEFAULT_IP, 8001), messages)
      .destroy_node(node_info)
      .destroy_subscriber(sub_info)
      .build<SimpleSubscriber>([](TestFixture& fixture) {
        SimpleSubscriber node(0);
        auto client = fixture.get_client_socket(0);

        EXPECT_TRUE(node.ok());
        for (int i = 0; i < 5; i++) {
          node.spin_once();
          EXPECT_TRUE(client->wait_for_operations(1, std::chrono::milliseconds(1250)));
        }
      });
}

// Recommended way to run tests for multithreaded nodes that use poller
TEST(SimpleSubscriberTest, SpinWithPollerAndOperationNotifications) {
  std::vector<std::shared_ptr<std_msgs::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<std_msgs::Header>();
    msg->seq = i;
    msg->frame_id = "Hello, world!";
    messages.push_back(msg);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .enable_poller(5)
      .enable_operation_notifications()
      .create_node("simple_subscriber", node_info)
      .create_subscriber<std_msgs::Header>("/chatter", sub_info, node_info, false, 1)
      .accept_notification<std_msgs::Header>("/chatter", {Endpoint(DEFAULT_IP, 8001)})
      .connect_to_publisher(Endpoint(DEFAULT_IP, 8001), messages)
      .destroy_node(node_info)
      .destroy_subscriber(sub_info)
      .build<SimpleSubscriber>([](TestFixture& fixture) {
        SimpleSubscriber node(0);
        auto client = fixture.get_client_socket(0);

        EXPECT_TRUE(node.ok());
        for (int i = 0; i < 5; i++) {
          node.spin_once();
          EXPECT_TRUE(client->wait_for_operations(1, std::chrono::milliseconds(1250)));
        }
      });
}
