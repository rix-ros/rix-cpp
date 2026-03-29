#include "rix/test/node_test_harness.hpp"
#include "simple_action.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

const int MAX_ITERS = 16;

TEST(SimpleActionTest, Create) {
  NodeTestHarness harness;
  harness.expect_action<rix::std_msgs::Double, rix::std_msgs::Float, rix::std_msgs::Double>("/exponent");
  auto& node = harness.create<SimpleAction>(MAX_ITERS, 1.0);
  EXPECT_TRUE(node.ok());
}

TEST(SimpleActionTest, RequestResponse) {
  NodeTestHarness harness;
  auto& capture =
      harness.expect_action<rix::std_msgs::Double, rix::std_msgs::Float, rix::std_msgs::Double>("/exponent");
  auto& node = harness.create<SimpleAction>(MAX_ITERS, 1.0);
  EXPECT_TRUE(node.ok());

  rix::std_msgs::Double req;
  req.data = 0;
  EXPECT_TRUE(capture.set_goal(req));
  for (int i = 0; i < MAX_ITERS - 1; ++i) {
    EXPECT_FALSE(capture.call());
    EXPECT_EQ(capture.feedback(i).data, (static_cast<float>(i) / static_cast<float>(MAX_ITERS)) * 100.0f);
  }
  EXPECT_TRUE(capture.call());
  EXPECT_NEAR(capture.result(0).data, exp(0), 1e-4);

  req.data = 1;
  capture.clear();
  EXPECT_TRUE(capture.set_goal(req));
  for (int i = 0; i < MAX_ITERS - 1; ++i) {
    EXPECT_FALSE(capture.call());
    EXPECT_EQ(capture.feedback(i).data, (static_cast<float>(i) / static_cast<float>(MAX_ITERS)) * 100.0f);
  }
  EXPECT_TRUE(capture.call());
  EXPECT_NEAR(capture.result(0).data, exp(1), 1e-4);

  req.data = 2;
  capture.clear();
  EXPECT_TRUE(capture.set_goal(req));
  for (int i = 0; i < MAX_ITERS - 1; ++i) {
    EXPECT_FALSE(capture.call());
    EXPECT_EQ(capture.feedback(i).data, (static_cast<float>(i) / static_cast<float>(MAX_ITERS)) * 100.0f);
  }
  EXPECT_TRUE(capture.call());
  EXPECT_NEAR(capture.result(0).data, exp(2), 1e-4);
}

TEST(SimpleActionTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<SimpleAction>(MAX_ITERS, 1.0);
  EXPECT_FALSE(node.ok());
}