#include "rix/test/test_fixture.hpp"
#include "simple_action_client.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleActionClientTest, Create) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_action_client", node_info)
      .create_action_client<std_msgs::Double, std_msgs::Float, std_msgs::Double>("/exponent", node_info)
      .destroy_node(node_info)
      .build<SimpleActionClient>([](TestFixture& fixture) {
        SimpleActionClient node(1);
        EXPECT_TRUE(node.ok());
      });
}

TEST(SimpleActionClientTest, CreateNodeRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_action_client", node_info, true) // Simulate failure
      .build<SimpleActionClient>([](TestFixture& fixture) {
        SimpleActionClient node(1);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleActionClientTest, CreateActionClientFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_action_client", node_info)
      .create_action_client<std_msgs::Double, std_msgs::Float, std_msgs::Double>(
          "/exponent", node_info, true) // Simulate failure
      .destroy_node(node_info)
      .build<SimpleActionClient>([](TestFixture& fixture) {
        SimpleActionClient node(1);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleServiceClientTest, Spin) {
  auto goal = std::make_shared<std_msgs::Double>();
  goal->data = 0.0;
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
  TestFixture()
      .create_node("simple_action_client", node_info)
      .create_action_client<std_msgs::Double, std_msgs::Float, std_msgs::Double>("/exponent", node_info)
      .enable_operation_notifications()
      .send_action_goal(goal, feedbacks, result)
      .disable_operation_notifications()
      .destroy_node(node_info)
      .build<SimpleActionClient>([&feedbacks](TestFixture& fixture) {
        SimpleActionClient node(1);
        EXPECT_TRUE(node.ok());

        std::thread thr([&node]() { node.spin(); });

        auto cli = fixture.get_client_socket(0);
        EXPECT_TRUE(cli->wait_for_operations(2 + feedbacks.size() + 1, std::chrono::milliseconds(5000)));

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}