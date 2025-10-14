#include "rix/test/node_test_fixture.hpp"
#include "simple_service.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(SimpleServiceTest, Create) {
  NodeTestFixture()
      .register_node()
      .register_service<msg::standard::UInt32, msg::standard::String>("/alphabet")
      .deregister_node()
      .deregister_service<msg::standard::UInt32, msg::standard::String>("/alphabet")
      .build<SimpleService>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleService> node) { EXPECT_TRUE(node->ok()); }, 0);
}

TEST(SimpleServiceTest, CreateNodeRegisterFailure) {
  NodeTestFixture()
      .register_node(true) // Simulate failure
      .build<SimpleService>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleService> node) { EXPECT_FALSE(node->ok()); }, 0);
}

TEST(SimpleServiceTest, CreatePublisherRegisterFailure) {
  NodeTestFixture()
      .register_node()
      .register_service<msg::standard::UInt32, msg::standard::String>("/alphabet", true) // Simulate failure
      .deregister_node()
      .build<SimpleService>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleService> node) { EXPECT_FALSE(node->ok()); }, 0);
}

// Recommended way to run tests for single-threaded nodes that do not use poller
TEST(SimpleServiceTest, SpinWithOperationNotifications) {
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

  NodeTestFixture()
      .register_node()
      .enable_operation_notifications()
      .register_service<msg::standard::UInt32, msg::standard::String>("/alphabet", false, 5)
      .create_srv_connection(requests[0], responses[0])
      .create_srv_connection(requests[1], responses[1])
      .create_srv_connection(requests[2], responses[2])
      .create_srv_connection(requests[3], responses[3])
      .create_srv_connection(requests[4], responses[4])
      .deregister_node()
      .deregister_service<msg::standard::UInt32, msg::standard::String>("/alphabet")
      .build<SimpleService>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleService> node) {
            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              node->spin_once();
              auto conn = fixture.get_connection_socket(i);
              EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          0);
}

// Recommended way to run tests for multithreaded nodes that use poller
TEST(SimpleServiceTest, SpinWithPollerAndOperationNotifications) {
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

  NodeTestFixture()
      .register_node()
      .enable_operation_notifications()
      .register_service<msg::standard::UInt32, msg::standard::String>("/alphabet", false, 5)
      .create_srv_connection(requests[0], responses[0])
      .create_srv_connection(requests[1], responses[1])
      .create_srv_connection(requests[2], responses[2])
      .create_srv_connection(requests[3], responses[3])
      .create_srv_connection(requests[4], responses[4])
      .deregister_node()
      .deregister_service<msg::standard::UInt32, msg::standard::String>("/alphabet")
      .build<SimpleService>(
          [](NodeTestFixture& fixture, std::unique_ptr<SimpleService> node) {
            EXPECT_TRUE(node->ok());
            for (int i = 0; i < 5; i++) {
              node->spin_once();
              auto conn = fixture.get_connection_socket(i);
              EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1250)));
            }
          },
          0);
}
