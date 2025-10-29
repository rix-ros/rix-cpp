#include "rix/test/test_fixture.hpp"
#include "simple_service.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleServiceTest, Create) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("simple_service", node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("/alphabet", srv_info, node_info)
      .destroy_node(node_info)
      .destroy_service(srv_info)
      .build<SimpleService>([](TestFixture& fixture) {
        SimpleService node(0);
        EXPECT_TRUE(node.ok());
      });
}

TEST(SimpleServiceTest, CreateNodeRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("simple_service", node_info, true) // Simulate failure
      .build<SimpleService>([](TestFixture& fixture) {
        SimpleService node(0);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleServiceTest, CreatePublisherRegisterFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("simple_service", node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>(
          "/alphabet", srv_info, node_info, true) // Simulate failure
      .destroy_node(node_info)
      .build<SimpleService>([](TestFixture& fixture) {
        SimpleService node(0);
        EXPECT_FALSE(node.ok());
      });
}

// Recommended way to run tests for single-threaded nodes that do not use poller
TEST(SimpleServiceTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<std_msgs::UInt32>> requests;
  std::vector<std::shared_ptr<std_msgs::String>> responses;
  for (int i = 0; i < 5; i++) {
    auto req = std::make_shared<std_msgs::UInt32>();
    req->data = i;
    requests.push_back(req);
    auto res = std::make_shared<std_msgs::String>();
    res->data = std::string(1, 'a' + (i % 26));
    responses.push_back(res);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("simple_service", node_info)
      .enable_operation_notifications()
      .create_service<std_msgs::UInt32, std_msgs::String>("/alphabet", srv_info, node_info, false, 5)
      .accept_service_client(requests[0], responses[0])
      .accept_service_client(requests[1], responses[1])
      .accept_service_client(requests[2], responses[2])
      .accept_service_client(requests[3], responses[3])
      .accept_service_client(requests[4], responses[4])
      .destroy_node(node_info)
      .destroy_service(srv_info)
      .build<SimpleService>([](TestFixture& fixture) {
        SimpleService node(0);
        EXPECT_TRUE(node.ok());
        for (int i = 0; i < 5; i++) {
          node.spin_once();
          auto conn = fixture.get_connection_socket(i);
          EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
        }
      });
}
