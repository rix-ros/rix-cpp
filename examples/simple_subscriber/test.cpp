#include "rix/test/node_test_fixture.hpp"
#include "simple_subscriber.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleSubscriberTest, Create) {
  NodeTestFixture()
      .register_node()
      .register_subscriber<msg::standard::Header>("/chatter")
      .deregister_node()
      .deregister_subscriber<msg::standard::Header>("/chatter")
      .build<SimpleSubscriber>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleSubscriber> node) { EXPECT_TRUE(node->ok()); }, 0);
}

TEST(SimpleSubscriberTest, CreateNodeRegisterFailure) {
  NodeTestFixture()
      .register_node(true) // Simulate failure
      .build<SimpleSubscriber>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleSubscriber> node) { EXPECT_FALSE(node->ok()); }, 0);
}

TEST(SimpleSubscriberTest, CreatePublisherRegisterFailure) {
  NodeTestFixture()
      .register_node()
      .register_subscriber<msg::standard::Header>("/chatter", true) // Simulate failure
      .deregister_node()
      .build<SimpleSubscriber>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleSubscriber> node) { EXPECT_FALSE(node->ok()); }, 0);
}

// Recommended way to run tests for single-threaded nodes that do not use poller
TEST(SimpleSubscriberTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<msg::standard::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<msg::standard::Header>();
    msg->seq = i;
    msg->frame_id = "Hello, world!";
    messages.push_back(msg);
  }

  NodeTestFixture()
      .enable_operation_notifications()
      .register_node()
      .register_subscriber<msg::standard::Header>("/chatter", false, 1)
      .create_sub_connection<msg::standard::Header>("/chatter", {Endpoint(DEFAULT_IP, 8001)})
      .create_sub_client(Endpoint(DEFAULT_IP, 8001), messages)
      .deregister_node()
      .deregister_subscriber<msg::standard::Header>("/chatter")
      .build<SimpleSubscriber>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleSubscriber> node) {
            auto client = fixture.get_client_socket(0);

            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              node->spin_once();
              EXPECT_TRUE(client->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          0);
}

// Recommended way to run tests for multithreaded nodes that use poller
TEST(SimpleSubscriberTest, SpinWithPollerAndOperationNotifications) {
  std::vector<std::shared_ptr<msg::standard::Header>> messages;
  for (int i = 0; i < 5; i++) {
    auto msg = std::make_shared<msg::standard::Header>();
    msg->seq = i;
    msg->frame_id = "Hello, world!";
    messages.push_back(msg);
  }

  NodeTestFixture()
      .enable_poller(5)
      .enable_operation_notifications()
      .register_node()
      .register_subscriber<msg::standard::Header>("/chatter", false, 1)
      .create_sub_connection<msg::standard::Header>("/chatter", {Endpoint(DEFAULT_IP, 8001)})
      .create_sub_client(Endpoint(DEFAULT_IP, 8001), messages)
      .deregister_node()
      .deregister_subscriber<msg::standard::Header>("/chatter")
      .build<SimpleSubscriber>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleSubscriber> node) {
            auto client = fixture.get_client_socket(0);

            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              node->spin_once();
              EXPECT_TRUE(client->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          0);
}
