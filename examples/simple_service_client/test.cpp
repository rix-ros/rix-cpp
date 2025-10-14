#include "rix/test/node_test_fixture.hpp"
#include "simple_service_client.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleServiceClientTest, Create) {
  msg::mediator::NodeInfo node_info;
  NodeTestFixture()
      .register_node("simple_service_client", node_info)
      .request_service_client<msg::standard::UInt32, msg::standard::String>("/alphabet")
      .deregister_node(node_info)
      .build<SimpleServiceClient>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleServiceClient> node) { EXPECT_TRUE(node->ok()); }, 10);
}

TEST(SimpleServiceClientTest, CreateNodeRegisterFailure) {
  msg::mediator::NodeInfo node_info;
  NodeTestFixture()
      .register_node("simple_service_client", node_info, true) // Simulate failure
      .build<SimpleServiceClient>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleServiceClient> node) { EXPECT_FALSE(node->ok()); }, 10);
}

TEST(SimpleServiceClientTest, CreateServiceClientFailure) {
  msg::mediator::NodeInfo node_info;
  NodeTestFixture()
      .register_node("simple_service_client", node_info)
      .request_service_client<msg::standard::UInt32, msg::standard::String>("/alphabet", true) // Simulate failure
      .deregister_node(node_info)
      .build<SimpleServiceClient>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleServiceClient> node) { EXPECT_FALSE(node->ok()); }, 10);
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
  NodeTestFixture()
      .enable_debug_clock(clock)
      .register_node("simple_service_client", node_info)
      .request_service_client<msg::standard::UInt32, msg::standard::String>("/alphabet", false)
      .enable_operation_notifications()
      .create_srv_cli_client(requests[0], responses[0])
      .create_srv_cli_client(requests[1], responses[1])
      .create_srv_cli_client(requests[2], responses[2])
      .create_srv_cli_client(requests[3], responses[3])
      .create_srv_cli_client(requests[4], responses[4])
      .disable_operation_notifications()
      .deregister_node(node_info)
      .build<SimpleServiceClient>(
          [clock](NodeTestFixture& fixture, std::unique_ptr<SimpleServiceClient> node) {
            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              clock->current_time += Duration(0.1); // Increment 0.1 second
              auto client = fixture.get_client_socket(i);
              node->spin_once();
              EXPECT_TRUE(client->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          10);
}