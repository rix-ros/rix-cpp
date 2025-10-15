#include "rix/test/node_test_fixture.hpp"
#include "simple_service_client.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleServiceClientTest, Create) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("simple_service_client", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::String>("/alphabet", node_info)
      .destroy_node(node_info)
      .build<SimpleServiceClient>([](TestFixture& fixture) {
        SimpleServiceClient node(1);
        EXPECT_TRUE(node.ok());
      });
}

TEST(SimpleServiceClientTest, CreateNodeRegisterFailure) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("simple_service_client", node_info, true) // Simulate failure
      .build<SimpleServiceClient>([](TestFixture& fixture) {
        SimpleServiceClient node(1);
        EXPECT_FALSE(node.ok());
      });
}

TEST(SimpleServiceClientTest, CreateServiceClientFailure) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("simple_service_client", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::String>("/alphabet", node_info, true) // Simulate failure
      .destroy_node(node_info)
      .build<SimpleServiceClient>([](TestFixture& fixture) {
        SimpleServiceClient node(1);
        EXPECT_FALSE(node.ok());
      });
}

// Recommended way to run tests for single-threaded nodes that do not use poller
TEST(SimpleServiceClientTest, SpinWithOperationNotifications) {
  std::vector<std::shared_ptr<msg::standard::UInt32>> requests;
  std::vector<std::shared_ptr<msg::standard::String>> responses;
  for (int i = 0; i < 5; i++) {
    auto req = std::make_shared<msg::standard::UInt32>();
    req->data = i;
    requests.push_back(req);
    auto res = std::make_shared<msg::standard::String>();
    res->data = std::string(1, 'a' + (i % 26));
    responses.push_back(res);
  }

  std::shared_ptr<MockClock> clock;
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .enable_clock(clock)
      .create_node("simple_service_client", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::String>("/alphabet", node_info, false)
      .enable_operation_notifications()
      .call_service_client(requests[0], responses[0])
      .call_service_client(requests[1], responses[1])
      .call_service_client(requests[2], responses[2])
      .call_service_client(requests[3], responses[3])
      .call_service_client(requests[4], responses[4])
      .disable_operation_notifications()
      .destroy_node(node_info)
      .build<SimpleServiceClient>([clock](TestFixture& fixture) {
        SimpleServiceClient node(1);
        EXPECT_TRUE(node.ok());
        for (int i = 0; i < 5; i++) {
          clock->sleep_for(Duration(1.0)); // Increment 1.0 second
          auto client = fixture.get_client_socket(i);
          node.spin_once();
          EXPECT_TRUE(client->wait_for_operations(1, std::chrono::milliseconds(1250)));
        }
      });
}