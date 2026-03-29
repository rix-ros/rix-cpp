#include "rix/test/node_test_harness.hpp"
#include "simple_action_client.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

TEST(SimpleActionClientTest, Create) {
  NodeTestHarness harness;
  harness.expect_action_client<rix::std_msgs::Double, rix::std_msgs::Float, rix::std_msgs::Double>("/exponent");
  auto& node = harness.create<SimpleActionClient>(1.0);
  EXPECT_TRUE(node.ok());
}

TEST(SimpleActionClientTest, SendMessages) {
  NodeTestHarness harness;
  auto& capture = harness.expect_action_client<rix::std_msgs::Double, rix::std_msgs::Float, rix::std_msgs::Double>("/exponent");
  auto& node = harness.create<SimpleActionClient>(1.0);
  EXPECT_TRUE(node.ok());

  harness.advance_time(1.0);
  harness.spin(1);

  EXPECT_EQ(capture.dispatch_count(), 1);
  EXPECT_EQ(capture.goal(0).data, 0.0);

  rix::std_msgs::Float feedback;
  feedback.data = 1.0f;
  capture.inject_feedback(feedback);
  feedback.data = 5.0f;
  capture.inject_feedback(feedback);
  feedback.data = 10.0f;
  capture.inject_feedback(feedback);

  rix::std_msgs::Double result;
  result.data = 100.0;
  capture.inject_result(result);
  harness.advance_time(1.0);
  harness.spin(1);
}

TEST(SimpleActionClientTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<SimpleActionClient>(1.0);
  EXPECT_FALSE(node.ok());
}