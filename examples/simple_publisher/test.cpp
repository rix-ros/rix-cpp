#include "rix/test/node_test_harness.hpp"
#include "simple_publisher.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

TEST(SimplePublisherTest, Create) {
  NodeTestHarness harness;
  harness.expect_publisher<rix::std_msgs::Header>("/chatter", 0);
  auto& node = harness.create<SimplePublisher>(1.0);
  EXPECT_TRUE(node.ok());
}

TEST(SimplePublisherTest, SendMessages) {
  NodeTestHarness harness;
  auto& capture = harness.expect_publisher<rix::std_msgs::Header>("/chatter", 1);
  auto& node = harness.create<SimplePublisher>(1.0);
  EXPECT_TRUE(node.ok());

  for (int i = 0; i < 3; i++) {
    harness.advance_time(1.0);
    harness.spin(1);
  }

  EXPECT_EQ(capture.message_count(), 3u);
  for (size_t i = 0; i < capture.message_count(); i++) {
    EXPECT_EQ(capture.message(i).seq, static_cast<uint32_t>(i + 1));
    EXPECT_EQ(capture.message(i).frame_id, "Hello!");
  }
}

TEST(SimplePublisherTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<SimplePublisher>(1.0);
  EXPECT_FALSE(node.ok());
}