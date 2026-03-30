#include "rix/test/node_test_harness.hpp"
#include "simple_service.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

TEST(SimpleServiceTest, Create) {
  NodeTestHarness harness;
  harness.expect_service<rix::std_msgs::UInt32, rix::std_msgs::String>("/alphabet");
  auto& node = harness.create<SimpleService>(1.0);
  EXPECT_TRUE(node.ok());
}

TEST(SimpleServiceTest, RequestResponse) {
  NodeTestHarness harness;
  auto& capture = harness.expect_service<rix::std_msgs::UInt32, rix::std_msgs::String>("/alphabet");
  auto& node = harness.create<SimpleService>(1.0);
  EXPECT_TRUE(node.ok());

  rix::std_msgs::UInt32 req;
  req.data = 17;
  EXPECT_TRUE(capture.call(req));
  EXPECT_EQ(capture.response(0).data, "r");

  req.data = 14;
  EXPECT_TRUE(capture.call(req));
  EXPECT_EQ(capture.response(1).data, "o");

  req.data = 1;
  EXPECT_TRUE(capture.call(req));
  EXPECT_EQ(capture.response(2).data, "b");
}

TEST(SimpleServiceTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<SimpleService>(1.0);
  EXPECT_FALSE(node.ok());
}