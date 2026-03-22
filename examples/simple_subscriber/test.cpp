#include "rix/test/node_test_harness.hpp"
#include "simple_subscriber.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

TEST(SimpleSubscriberTest, Create) {
  NodeTestHarness harness;
  harness.expect_subscriber<rix::std_msgs::Header>("/chatter");
  auto& node = harness.create<SimpleSubscriber>(1.0);
  EXPECT_TRUE(node.ok());
}

TEST(SimpleSubscriberTest, ReceiveMessages) {
  NodeTestHarness harness;
  auto& capture = harness.expect_subscriber<rix::std_msgs::Header>("/chatter");
  auto& node = harness.create<SimpleSubscriber>(1.0);
  EXPECT_TRUE(node.ok());

  std_msgs::Header msg;
  for (int i = 0; i < 3; i++) {
    msg.seq = i + 1;
    msg.frame_id = "Hello!";
    capture.inject(msg);
  }

  for (int i = 0; i < 3; i++) {
    harness.advance_time(1.0);
    harness.spin(1);
  }
}

TEST(SimpleSubscriberTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<SimpleSubscriber>(1.0);
  EXPECT_FALSE(node.ok());
}