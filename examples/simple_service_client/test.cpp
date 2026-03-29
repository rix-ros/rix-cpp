#include "rix/test/node_test_harness.hpp"
#include "simple_service_client.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

TEST(SimpleServiceClientTest, Create) {
  NodeTestHarness harness;
  harness.expect_service_client<rix::std_msgs::UInt32, rix::std_msgs::String>("/alphabet");
  auto& node = harness.create<SimpleServiceClient>(1.0);
  EXPECT_TRUE(node.ok());
}

TEST(SimpleServiceClientTest, SendMessages) {
  NodeTestHarness harness;
  auto& capture = harness.expect_service_client<rix::std_msgs::UInt32, rix::std_msgs::String>("/alphabet");
  auto& node = harness.create<SimpleServiceClient>(1.0);
  EXPECT_TRUE(node.ok());

  rix::std_msgs::String res;
  res.data = "Hello,";
  capture.queue_response(res);
  res.data = "World!";
  capture.queue_response(res);
  res.data = "Test.";
  capture.queue_response(res);
  for (int i = 0; i < 3; i++) {
    harness.advance_time(1.0);
    harness.spin(1);
  }

  EXPECT_EQ(capture.call_count(), 3);
  EXPECT_EQ(capture.request(0).data, 0);
  EXPECT_EQ(capture.request(1).data, 1);
  EXPECT_EQ(capture.request(2).data, 2);
}

TEST(SimpleServiceClientTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<SimpleServiceClient>(1.0);
  EXPECT_FALSE(node.ok());
}