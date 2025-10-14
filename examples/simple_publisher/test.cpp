#include "rix/test/node_test_fixture.hpp"
#include "simple_publisher.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimplePublisherTest, Create) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  NodeTestFixture()
      .register_node("simple_publisher", node_info)
      .register_publisher<msg::standard::Header>("/chatter", pub_info)
      .deregister_node(node_info)
      .deregister_publisher(pub_info)
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) { EXPECT_TRUE(node->ok()); }, 1.0, 0);
}

TEST(SimplePublisherTest, CreateNodeRegisterFailure) {
  msg::mediator::NodeInfo node_info;
  NodeTestFixture()
      .register_node("simple_publisher", node_info, true) // Simulate failure
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) { EXPECT_FALSE(node->ok()); }, 1.0, 0);
}

TEST(SimplePublisherTest, CreatePublisherRegisterFailure) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  NodeTestFixture()
      .register_node("simple_publisher", node_info)
      .register_publisher<msg::standard::Header>("/chatter", pub_info, true) // Simulate failure
      .deregister_node(node_info)
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) { EXPECT_FALSE(node->ok()); }, 1.0, 0);
}

TEST(SimplePublisherTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<msg::standard::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<msg::standard::Header>();
    msg->seq = i + 1;
    msg->frame_id = "Hello, world!";
    msg->stamp.sec = i + 1;
    msg->stamp.nsec = 0;
    messages.push_back(msg);
  }

  std::shared_ptr<MockClock> clock;
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  NodeTestFixture()
      .enable_debug_clock(clock)
      .register_node("simple_publisher", node_info)
      .register_publisher<msg::standard::Header>("/chatter", pub_info, false, 1)
      .enable_operation_notifications()
      .create_pub_connection(messages)
      .disable_operation_notifications()
      .deregister_node(node_info)
      .deregister_publisher(pub_info)
      .build<SimplePublisher>(
          [clock](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) {
            auto conn = fixture.get_connection_socket(0);

            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              clock->current_time += Duration(1.0);
              node->spin_once();
              EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          1,
          0);
}

// Recommended way to run tests for multithreaded nodes that use poller
TEST(SimplePublisherTest, SpinWithPollerAndOperationNotifications) {
  std::vector<std::shared_ptr<msg::standard::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<msg::standard::Header>();
    msg->seq = i + 1;
    msg->frame_id = "Hello, world!";
    msg->stamp.sec = i + 1;
    msg->stamp.nsec = 0;
    messages.push_back(msg);
  }

  std::shared_ptr<MockClock> clock;
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  NodeTestFixture()
      .enable_debug_clock(clock)
      .enable_poller(5)
      .enable_operation_notifications()
      .register_node("simple_publisher", node_info)
      .register_publisher<msg::standard::Header>("/chatter", pub_info, false, 1)
      .create_pub_connection(messages)
      .deregister_node(node_info)
      .deregister_publisher(pub_info)
      .build<SimplePublisher>(
          [clock](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) {
            auto conn = fixture.get_connection_socket(0);

            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              clock->current_time += Duration(1.0);
              node->spin_once();
              EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          1,
          0);
}
