#include "rix/test/node_test_harness.hpp"
#include "parameter_server.hpp"
#include <gtest/gtest.h>

using namespace rix;
using namespace rix::test;

TEST(ParameterServerTest, Create) {
  NodeTestHarness harness;
  auto& node = harness.create<ParameterServer>(0);
  EXPECT_TRUE(node.ok());
}

TEST(ParameterServerTest, RequestResponse) {
  NodeTestHarness harness;
  auto& node = harness.create<ParameterServer>(0);
  EXPECT_TRUE(node.ok());

  rix::std_msgs::Header param;
  node.get_parameter("test_param", param);
  EXPECT_EQ(param.frame_id, "test");
  EXPECT_EQ(param.seq, 1234);
}

TEST(ParameterServerTest, CreateFail) {
  NodeTestHarness harness;
  harness.node_registration_fails();
  auto& node = harness.create<ParameterServer>(0);
  EXPECT_FALSE(node.ok());
}