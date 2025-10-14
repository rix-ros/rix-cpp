#include "rix/test/node_test_fixture.hpp"
#include "simple_publisher.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimplePublisherTest, Create) {
  NodeTestFixture()
      .register_node()
      .register_publisher<msg::standard::Header>("/chatter")
      .deregister_node()
      .deregister_publisher<msg::standard::Header>("/chatter")
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) { EXPECT_TRUE(node->ok()); }, 1.0, 0);
}

TEST(SimplePublisherTest, CreateNodeRegisterFailure) {
  NodeTestFixture()
      .register_node(true) // Simulate failure
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) { EXPECT_FALSE(node->ok()); }, 1.0, 0);
}

TEST(SimplePublisherTest, CreatePublisherRegisterFailure) {
  NodeTestFixture()
      .register_node()
      .register_publisher<msg::standard::Header>("/chatter", true) // Simulate failure
      .deregister_node()
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) { EXPECT_FALSE(node->ok()); }, 1.0, 0);
}

TEST(SimplePublisherTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<msg::standard::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<msg::standard::Header>();
    msg->seq = i;
    msg->frame_id = "Hello, world!";
    messages.push_back(msg);
  }

  NodeTestFixture()
      .enable_debug_clock()
      .register_node()
      .register_publisher<msg::standard::Header>("/chatter", false, 1)
      .enable_operation_notifications()
      .create_pub_connection(messages)
      .disable_operation_notifications()
      .deregister_node()
      .deregister_publisher<msg::standard::Header>("/chatter")
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) {
            auto conn = fixture.get_connection_socket(0);

            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              MockClock::current_time += Duration(1.0);
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
    msg->seq = i;
    msg->frame_id = "Hello, world!";
    messages.push_back(msg);
  }

  NodeTestFixture()
      .enable_debug_clock()
      .enable_poller(5)
      .enable_operation_notifications()
      .register_node()
      .register_publisher<msg::standard::Header>("/chatter", false, 1)
      .create_pub_connection(messages)
      .deregister_node()
      .deregister_publisher<msg::standard::Header>("/chatter")
      .build<SimplePublisher>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimplePublisher> node) {
            auto conn = fixture.get_connection_socket(0);

            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              MockClock::current_time += Duration(1.0);
              node->spin_once();
              EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          1,
          0);
}
