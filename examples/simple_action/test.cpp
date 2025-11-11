#include "rix/test/test_fixture.hpp"
#include "simple_action.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleActionTest, Create) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("simple_action", node_info)
      .create_action<std_msgs::Double, std_msgs::Float, std_msgs::Double>(
          "/exponent", act_info, node_info)
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<SimpleAction>([](TestFixture& fixture) {
        SimpleAction node(10, 0);
        EXPECT_TRUE(node.ok());
      });
}

TEST(SimpleActionTest, CreateNodeRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_action", node_info, true) // Simulate failure
      .build<SimpleAction>([](TestFixture& fixture) {
        SimpleAction node(10, 0);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleActionTest, CreateActionFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("simple_action", node_info)
      .create_action<std_msgs::Double, std_msgs::Float, std_msgs::Double>(
          "/exponent", act_info, node_info, 0, true) // Simulate failure
      .destroy_node(node_info)
      .build<SimpleAction>([](TestFixture& fixture) {
        SimpleAction node(10, 0);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleActionTests, SpinWithOperationNotifications) {
  auto goal = std::make_shared<std_msgs::Double>();
  goal->data = 3.0;
  auto feedbacks = std::vector<std::shared_ptr<std_msgs::Float>>();
  double result_data = 0.0;
  for (int i = 0; i < 10; ++i) {
    auto feedback = std::make_shared<std_msgs::Float>();
    feedback->data = static_cast<float>(i) / 10.0f * 100.0f;
    feedbacks.push_back(feedback);
    result_data += pow(goal->data, i) / tgamma(static_cast<double>(i + 1));
  }
  auto result = std::make_shared<std_msgs::Double>();
  result->data = result_data;

  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("simple_action", node_info)
      .enable_operation_notifications()
      .create_action<std_msgs::Double, std_msgs::Float, std_msgs::Double>(
          "/exponent", act_info, node_info, 0, false, 1)
      .accept_action_client(goal, feedbacks, result)
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<SimpleAction>([feedbacks](TestFixture& fixture) {
        SimpleAction node(10, 0);
        EXPECT_TRUE(node.ok());

        std::thread thr([&node]() { node.spin(); });

        auto conn = fixture.get_connection_socket(0);
        EXPECT_TRUE(conn->wait_for_operations(3 + feedbacks.size() + 1, std::chrono::milliseconds(5000)));

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}