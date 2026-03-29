#include "rix/core/node.hpp"
#include "rix/std_msgs/Time.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/sys_msgs/NodeInfo.hpp"
#include "rix/sys_msgs/SubNotify.hpp"
#include "rix/test/test_fixture.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(MessageTest, PublisherAcceptConnectionsAndPublish) {
  // Create messages to publish
  std::vector<std::shared_ptr<std_msgs::UInt32>> messages;
  for (int i = 0; i < 3; ++i) {
    auto msg = std::make_shared<std_msgs::UInt32>();
    msg->data = 42;
    messages.push_back(msg);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .enable_poller(3)
      .create_node("test_node", node_info)
      .enable_operation_notifications()
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info, false, 3)
      .accept_subscriber(messages)
      .accept_subscriber(messages)
      .accept_subscriber(messages)
      .disable_operation_notifications()
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto connection_sockets = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create publisher with specified endpoint
        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        if (!MULTITHREADED) {
          // Process connection acceptances
          node.spin_once();
          EXPECT_TRUE(pub->ok());
          EXPECT_EQ(pub->get_subscriber_count(), 1);

          node.spin_once();
          EXPECT_TRUE(pub->ok());
          EXPECT_EQ(pub->get_subscriber_count(), 2);

          node.spin_once();
          EXPECT_TRUE(pub->ok());
          EXPECT_EQ(pub->get_subscriber_count(), 3);
        } else {
          // Wait for all 3 connections to be accepted (with timeout)
          EXPECT_TRUE(server_socket->wait_for_operations(3, std::chrono::milliseconds(5000)));
          EXPECT_TRUE(pub->ok());
          EXPECT_EQ(pub->get_subscriber_count(), 3);
        }

        // Publish messages
        auto msg = std::make_shared<std_msgs::UInt32>();
        msg->data = 42;
        pub->publish(*msg);
        pub->publish(*msg);
        pub->publish(*msg);

        if (MULTITHREADED) {
          // Wait for all messages to be sent on each connection
          //  socket will notify 3 times (once per message)
          EXPECT_TRUE(fixture.wait_for_all_connections(3, std::chrono::milliseconds(5000)));
        }

        // Try publishing wrong message type (should not be sent)
        auto wrong_msg = std::make_shared<std_msgs::Time>();
        pub->publish(*wrong_msg);

        // Shutdown publisher
        pub->shutdown();
        EXPECT_FALSE(pub->ok());
        pub = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, SubscriberConnectAndReceive) {
  // Create messages to receive
  std::vector<std::shared_ptr<std_msgs::UInt32>> messages;
  messages.push_back(std::make_shared<std_msgs::UInt32>());
  messages.push_back(std::make_shared<std_msgs::UInt32>());
  messages.push_back(std::make_shared<std_msgs::UInt32>());
  messages[0]->data = 42;
  messages[1]->data = 43;
  messages[2]->data = 44;

  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .enable_poller(3)
      .create_node("test_node", node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info, false, 1)
      .accept_notification<std_msgs::UInt32>(
          "test_topic", {Endpoint("127.0.0.1", 8002), Endpoint("127.0.0.1", 8003), Endpoint("127.0.0.1", 8004)})
      .enable_operation_notifications()
      .connect_to_publisher(Endpoint("127.0.0.1", 8002), messages)
      .connect_to_publisher(Endpoint("127.0.0.1", 8003), messages)
      .connect_to_publisher(Endpoint("127.0.0.1", 8004), messages)
      .disable_operation_notifications()
      .destroy_subscriber(sub_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        auto sub_clients = fixture.get_client_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Track received messages
        std::vector<uint32_t> received_data;
        auto callback = [&received_data](const std_msgs::UInt32& msg) { received_data.push_back(msg.data); };

        // Create subscriber
        auto sub = node.create_subscriber<std_msgs::UInt32>("test_topic", callback);
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback = [](const std_msgs::Time& msg) { FAIL() << "Should not receive Time message"; };
        sub->set_callback<std_msgs::Time>(wrong_callback);
        EXPECT_TRUE(sub->ok());

        if (!MULTITHREADED) {
          // Process messages
          node.spin_once();
          EXPECT_TRUE(sub->ok());
          EXPECT_EQ(sub->get_publisher_count(), 3);
          EXPECT_EQ(received_data.size(), 3);
          EXPECT_EQ(received_data[0], 42);
          EXPECT_EQ(received_data[1], 42);
          EXPECT_EQ(received_data[2], 42);

          node.spin_once();
          EXPECT_TRUE(sub->ok());
          EXPECT_EQ(sub->get_publisher_count(), 3);
          EXPECT_EQ(received_data.size(), 6);
          EXPECT_EQ(received_data[3], 43);
          EXPECT_EQ(received_data[4], 43);
          EXPECT_EQ(received_data[5], 43);

          node.spin_once();
          EXPECT_TRUE(sub->ok());
          EXPECT_EQ(sub->get_publisher_count(), 3);
          EXPECT_EQ(received_data.size(), 9);
          EXPECT_EQ(received_data[6], 44);
          EXPECT_EQ(received_data[7], 44);
          EXPECT_EQ(received_data[8], 44);
        } else {
          // Wait for all 9 messages to be received (3 messages from 3 clients)
          // Each client socket will notify once per message
          EXPECT_TRUE(fixture.wait_for_all_clients(3, std::chrono::milliseconds(5000)));
          EXPECT_TRUE(sub->ok());
          EXPECT_EQ(sub->get_publisher_count(), 3);
          EXPECT_EQ(received_data.size(), 9);
          EXPECT_EQ(received_data[0], 42);
          EXPECT_EQ(received_data[1], 42);
          EXPECT_EQ(received_data[2], 42);
          EXPECT_EQ(received_data[3], 43);
          EXPECT_EQ(received_data[4], 43);
          EXPECT_EQ(received_data[5], 43);
          EXPECT_EQ(received_data[6], 44);
          EXPECT_EQ(received_data[7], 44);
          EXPECT_EQ(received_data[8], 44);
        }

        // Shutdown subscriber
        sub->shutdown();
        EXPECT_FALSE(sub->ok());
        sub = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, ServiceAcceptRequestAndRespond) {
  // Create request/response pairs
  std::vector<std::shared_ptr<std_msgs::UInt32>> requests;
  std::vector<std::shared_ptr<std_msgs::Time>> responses;
  for (int i = 1; i <= 3; ++i) {
    auto req = std::make_shared<std_msgs::UInt32>();
    auto res = std::make_shared<std_msgs::Time>();
    req->data = i;
    res->sec = i;
    res->nsec = i + 500;
    requests.push_back(req);
    responses.push_back(res);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .enable_operation_notifications()
      .create_service<std_msgs::UInt32, std_msgs::Time>("test_topic", srv_info, node_info, false, 3)
      .accept_service_client(requests[0], responses[0])
      .accept_service_client(requests[1], responses[1])
      .accept_service_client(requests[2], responses[2])
      .disable_operation_notifications()
      .destroy_service(srv_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto srv_connections = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto callback = [](const std_msgs::UInt32& req, std_msgs::Time& res) {
          res.sec = req.data;
          res.nsec = req.data + 500;
        };

        auto srv = node.create_service<std_msgs::UInt32, std_msgs::Time>("test_topic", callback);
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback = [](const std_msgs::Time& req, std_msgs::UInt32& res) {
          FAIL() << "Should not be called with wrong message types";
        };
        srv->set_callback<std_msgs::Time, std_msgs::UInt32>(wrong_callback);
        EXPECT_TRUE(srv->ok());

        if (!MULTITHREADED) {
          // Process requests
          node.spin_once();
          EXPECT_TRUE(srv->ok());

          node.spin_once();
          EXPECT_TRUE(srv->ok());

          node.spin_once();
          EXPECT_TRUE(srv->ok());
        } else {
          // Wait for server to accept all 3 connections first
          EXPECT_TRUE(server_socket->wait_for_operations(3, std::chrono::milliseconds(5000)));
          EXPECT_TRUE(fixture.wait_for_all_connections(1, std::chrono::milliseconds(5000)));
          EXPECT_TRUE(srv->ok());
        }

        // Shutdown service
        srv->shutdown();
        EXPECT_FALSE(srv->ok());
        srv = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, ActionAcceptGoalwithFeedbackAndResult) {
  // Create request/response pairs
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    for (int j = 1; j <= 3; ++j) {
      auto feedback = std::make_shared<std_msgs::UInt32>();
      feedback->data = i * 10 + j;
      feedbacks.back().push_back(feedback);
    }
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .enable_operation_notifications()
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
          "test_action", act_info, node_info, 0, false, 1)
      .accept_action_client(goals[0], feedbacks[0], results[0])
      .disable_operation_notifications()
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<Node>([](const TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto srv_connections = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto callback = [](const std_msgs::UInt32& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) {
          static int feedback_count = 0;
          if (feedback_count >= 3) {
            result.sec = goal.data;
            result.nsec = goal.data + 500;
            feedback_count = 0;
            return true;
          }
          feedback_count++;
          feedback.data = goal.data * 10 + feedback_count;
          return false;
        };

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", callback);
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback =
            [](const std_msgs::Time& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) -> bool {
          EXPECT_FALSE(true) << "Should not be called with wrong message types";
          return false;
        };
        act->set_callback<std_msgs::Time, std_msgs::UInt32, std_msgs::Time>(wrong_callback);
        EXPECT_TRUE(act->ok());

        std::thread thr;
        if (!MULTITHREADED) {
          thr = std::thread([&node]() { node.spin(); });
        }

        EXPECT_TRUE(server_socket->wait_for_operations(1, std::chrono::milliseconds(5000)));
        // 2 recv (opcode & goal) + 1 send (status) + 3 send (feedback) + 1 send (result) = 7 operations per connection
        EXPECT_TRUE(fixture.wait_for_all_connections(7, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(act->ok());

        if (!MULTITHREADED) {
          node.shutdown();
          if (thr.joinable()) {
            thr.join();
          }
        }
      });
}

TEST(MessageTest, ActionAcceptGoalWithCancel) {
  // Create request/response pairs
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .enable_operation_notifications()
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
          "test_action", act_info, node_info, 0, false, 1)
      .accept_action_client_with_cancel(goals[0], feedbacks[0])
      .disable_operation_notifications()
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<Node>([](const TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto srv_connections = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto callback = [](const std_msgs::UInt32& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) {
          static int feedback_count = 0;
          if (feedback_count >= 3) {
            result.sec = goal.data;
            result.nsec = goal.data + 500;
            feedback_count = 0;
            return true;
          }
          feedback_count++;
          feedback.data = goal.data * 10 + feedback_count;
          return false;
        };

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", callback);
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback =
            [](const std_msgs::Time& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) -> bool {
          EXPECT_FALSE(true) << "Should not be called with wrong message types";
          return false;
        };
        act->set_callback<std_msgs::Time, std_msgs::UInt32, std_msgs::Time>(wrong_callback);
        EXPECT_TRUE(act->ok());

        std::thread thr;
        if (!MULTITHREADED) {
          thr = std::thread([&node]() { node.spin(); });
        }
        EXPECT_TRUE(server_socket->wait_for_operations(1, std::chrono::milliseconds(5000)));
        auto conn_a = srv_connections[0]; // First connection should succeed
        EXPECT_TRUE(conn_a->wait_for_operations(
            4, std::chrono::milliseconds(5000))); // 2 recv (opcode & goal) + 1 send (status) + 1 recv (cancel)
        EXPECT_TRUE(act->ok());

        if (!MULTITHREADED) {
          node.shutdown();
          if (thr.joinable()) {
            thr.join();
          }
        }
      });
}

TEST(MessageTest, ActionAcceptGoalFailureAlreadyConnected) {
  // Create request/response pairs
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    for (int j = 1; j <= 3; ++j) {
      auto feedback = std::make_shared<std_msgs::UInt32>();
      feedback->data = i * 10 + j;
      feedbacks.back().push_back(feedback);
    }
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .enable_operation_notifications()
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
          "test_action", act_info, node_info, 0, false, 2)
      .accept_action_client(goals[0], feedbacks[0], results[0])
      .accept_action_client(goals[1], feedbacks[1], results[1], true)
      .disable_operation_notifications()
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<Node>([](const TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto srv_connections = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto callback = [](const std_msgs::UInt32& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) {
          static int feedback_count = 0;
          if (feedback_count >= 3) {
            result.sec = goal.data;
            result.nsec = goal.data + 500;
            feedback_count = 0;
            return true;
          }
          feedback_count++;
          feedback.data = goal.data * 10 + feedback_count;
          return false;
        };

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", callback);
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback =
            [](const std_msgs::Time& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) -> bool {
          EXPECT_FALSE(true) << "Should not be called with wrong message types";
          return false;
        };
        act->set_callback<std_msgs::Time, std_msgs::UInt32, std_msgs::Time>(wrong_callback);
        EXPECT_TRUE(act->ok());

        std::thread thr([&node]() { node.spin(); });

        EXPECT_TRUE(server_socket->wait_for_operations(2, std::chrono::milliseconds(5000)));
        auto conn_a = srv_connections[0]; // First connection should succeed
        // 2 recv (opcode & goal) + 1 send (status) + 3 send (feedback) + 1 send (result) = 7 operations per connection
        EXPECT_TRUE(conn_a->wait_for_operations(7, std::chrono::milliseconds(5000)));
        // 1 recv (goal) + 1 send (status) = 2 operations for failed connection
        auto conn_b = srv_connections[1]; // Second connection should fail
        EXPECT_TRUE(conn_b->wait_for_operations(2, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(act->ok());

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}

TEST(MessageTest, ActionAcceptGoalWithPreempt) {
  // Create request/response pairs
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  auto preempt_feedback = std::make_shared<std_msgs::UInt32>();
  preempt_feedback->data = 21;
  feedbacks[1].push_back(preempt_feedback);
  preempt_feedback = std::make_shared<std_msgs::UInt32>();
  preempt_feedback->data = 31;
  feedbacks[2].push_back(preempt_feedback);
  preempt_feedback = std::make_shared<std_msgs::UInt32>();
  preempt_feedback->data = 32;
  feedbacks[2].push_back(preempt_feedback);
  preempt_feedback = std::make_shared<std_msgs::UInt32>();
  preempt_feedback->data = 33;
  feedbacks[2].push_back(preempt_feedback);

  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .enable_operation_notifications()
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
          "test_action", act_info, node_info, 0, false, 1)
      .accept_action_client_with_preempt(goals, feedbacks, results[2])
      .disable_operation_notifications()
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<Node>([](const TestFixture& fixture) {
        auto server_socket = fixture.get_server_socket();
        auto srv_connections = fixture.get_connection_sockets();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto current_goal = std::make_shared<std_msgs::UInt32>();
        current_goal->data = 0;
        auto feedback_count = std::make_shared<int>(0);
        auto callback = [feedback_count, current_goal](
                            const std_msgs::UInt32& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) {
          if (goal.data != current_goal->data) {
            *current_goal = goal;
            *feedback_count = 0;
          }
          if (*feedback_count >= 3) {
            result.sec = goal.data;
            result.nsec = goal.data + 500;
            *feedback_count = 0;
            return true;
          }
          (*feedback_count)++;
          feedback.data = goal.data * 10 + *feedback_count;
          return false;
        };

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", callback);
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        // Set wrong callback (should not be called)
        auto wrong_callback =
            [](const std_msgs::Time& goal, std_msgs::UInt32& feedback, std_msgs::Time& result) -> bool {
          EXPECT_FALSE(true) << "Should not be called with wrong message types";
          return false;
        };
        act->set_callback<std_msgs::Time, std_msgs::UInt32, std_msgs::Time>(wrong_callback);
        EXPECT_TRUE(act->ok());

        std::thread thr;
        if (!MULTITHREADED) {
          thr = std::thread([&node]() { node.spin(); });
        }

        EXPECT_TRUE(server_socket->wait_for_operations(1, std::chrono::milliseconds(5000)));
        auto conn_a = srv_connections[0]; // First connection should succeed
        // 3 (2 recv (opcode & goal) + 1 send (status)) + 4 (2 recv (opcode & goal) + 1 send (status) + 1 send
        // (feedback)) + 7 (2 recv (opcode & goal) + 1 send (status) + 3 send (feedback) + 1 send (result)) = 14
        // operations
        EXPECT_TRUE(conn_a->wait_for_operations(14, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(act->ok());

        if (!MULTITHREADED) {
          node.shutdown();
          if (thr.joinable()) {
            thr.join();
          }
        }
      });
}

TEST(MessageTest, ServiceClientRequestAndReceive) {
  // Create request/response pairs
  std::vector<std::shared_ptr<std_msgs::UInt32>> requests;
  std::vector<std::shared_ptr<std_msgs::Time>> responses;
  for (int i = 1; i <= 3; ++i) {
    auto req = std::make_shared<std_msgs::UInt32>();
    auto res = std::make_shared<std_msgs::Time>();
    req->data = i;
    res->sec = i;
    res->nsec = i + 500;
    requests.push_back(req);
    responses.push_back(res);
  }

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::Time>("test_service", node_info)
      .call_service_client(requests[0], responses[0])
      .call_service_client(requests[1], responses[1])
      .call_service_client(requests[2], responses[2])
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service client
        auto srvcli = node.create_service_client<std_msgs::UInt32, std_msgs::Time>("test_service");
        EXPECT_NE(srvcli, nullptr);
        EXPECT_TRUE(srvcli->ok());

        // Make service calls
        std_msgs::UInt32 request;
        std_msgs::Time response;

        request.data = 1;
        bool call_result = srvcli->call(request, response);
        EXPECT_TRUE(call_result);
        EXPECT_EQ(response.sec, 1);
        EXPECT_EQ(response.nsec, 501);

        request.data = 2;
        call_result = srvcli->call(request, response);
        EXPECT_TRUE(call_result);
        EXPECT_EQ(response.sec, 2);
        EXPECT_EQ(response.nsec, 502);

        request.data = 3;
        call_result = srvcli->call(request, response);
        EXPECT_TRUE(call_result);
        EXPECT_EQ(response.sec, 3);
        EXPECT_EQ(response.nsec, 503);

        // Shutdown service client
        srvcli->shutdown();
        EXPECT_FALSE(srvcli->ok());
        srvcli = nullptr;
        node.spin_once();
      });
}

TEST(MessageTest, ActionClientDispatch) {
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    for (int j = 1; j <= 3; ++j) {
      auto feedback = std::make_shared<std_msgs::UInt32>();
      feedback->data = i * 10 + j;
      feedbacks.back().push_back(feedback);
    }
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", node_info)
      .send_action_goal(goals[0], feedbacks[0], results[0])
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action client
        std_msgs::UInt32 goal;
        std::vector<std_msgs::UInt32> feedback;
        std_msgs::Time result;
        auto actcli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
            "test_action",
            [&feedback](const std_msgs::UInt32& fb) { feedback.push_back(fb); },
            [&result](const std_msgs::Time& res) { result = res; });
        EXPECT_NE(actcli, nullptr);
        EXPECT_TRUE(actcli->ok());

        std::thread thr([&node]() { node.spin(); });

        // Send action goal and receive feedback/result
        goal.data = 1;
        bool send_result = actcli->dispatch(goal);
        EXPECT_TRUE(send_result);

        EXPECT_TRUE(actcli->wait_for_result(Duration(5.0))); // Wait up to 5 seconds for result

        EXPECT_EQ(feedback.size(), 3);
        EXPECT_EQ(feedback[0].data, 11);
        EXPECT_EQ(feedback[1].data, 12);
        EXPECT_EQ(feedback[2].data, 13);
        EXPECT_EQ(result.sec, 1);
        EXPECT_EQ(result.nsec, 501);

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}

TEST(MessageTest, ActionClientDispatchWithCancel) {
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    for (int j = 1; j <= 3; ++j) {
      auto feedback = std::make_shared<std_msgs::UInt32>();
      feedback->data = i * 10 + j;
      feedbacks.back().push_back(feedback);
    }
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", node_info)
      .send_action_goal_with_cancel(goals[0], feedbacks[0])
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action client
        std_msgs::UInt32 goal;
        std::vector<std_msgs::UInt32> feedback;
        std_msgs::Time result;
        std::mutex feedback_mutex;
        std::condition_variable feedback_cv;
        std::shared_ptr<ActionClient> actcli =
            node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
                "test_action",
                [&feedback, &actcli, &feedback_mutex, &feedback_cv](const std_msgs::UInt32& fb) {
                  std::lock_guard<std::mutex> lock(feedback_mutex);
                  feedback.push_back(fb);
                  if (feedback.size() == 3) {
                    actcli->cancel();
                    feedback_cv.notify_one();
                  }
                },
                [&result](const std_msgs::Time& res) { result = res; });
        EXPECT_NE(actcli, nullptr);
        EXPECT_TRUE(actcli->ok());

        std::thread thr([&node]() { node.spin(); });

        // Send action goal and receive feedback/result
        goal.data = 1;
        bool send_result = actcli->dispatch(goal);
        EXPECT_TRUE(send_result);

        {
          std::unique_lock<std::mutex> lock(feedback_mutex);
          EXPECT_TRUE(
              feedback_cv.wait_for(lock, std::chrono::seconds(5), [&feedback]() { return feedback.size() >= 3; }));
        }

        EXPECT_EQ(feedback.size(), 3);
        EXPECT_EQ(feedback[0].data, 11);
        EXPECT_EQ(feedback[1].data, 12);
        EXPECT_EQ(feedback[2].data, 13);

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}

TEST(MessageTest, ActionClientDispatchFailure) {
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 3; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    for (int j = 1; j <= 3; ++j) {
      auto feedback = std::make_shared<std_msgs::UInt32>();
      feedback->data = i * 10 + j;
      feedbacks.back().push_back(feedback);
    }
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", node_info)
      .send_action_goal(goals[0], feedbacks[0], results[0], true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action client
        std_msgs::UInt32 goal;
        std::vector<std_msgs::UInt32> feedback;
        std_msgs::Time result;
        auto actcli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
            "test_action",
            [&feedback](const std_msgs::UInt32& fb) { feedback.push_back(fb); },
            [&result](const std_msgs::Time& res) { result = res; });
        EXPECT_NE(actcli, nullptr);
        EXPECT_TRUE(actcli->ok());

        std::thread thr([&node]() { node.spin(); });

        // Send action goal and receive feedback/result
        goal.data = 1;
        bool send_result = actcli->dispatch(goal);
        EXPECT_FALSE(send_result);

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}

TEST(MessageTest, ActionClientDispatchPreempt) {
  std::vector<std::shared_ptr<std_msgs::UInt32>> goals;
  std::vector<std::vector<std::shared_ptr<std_msgs::UInt32>>> feedbacks;
  std::vector<std::shared_ptr<std_msgs::Time>> results;
  for (int i = 1; i <= 2; ++i) {
    auto goal = std::make_shared<std_msgs::UInt32>();
    goal->data = i;
    goals.push_back(goal);
    feedbacks.push_back(std::vector<std::shared_ptr<std_msgs::UInt32>>());
    for (int j = 1; j <= 3; ++j) {
      auto feedback = std::make_shared<std_msgs::UInt32>();
      feedback->data = i * 10 + j;
      feedbacks.back().push_back(feedback);
    }
    auto result = std::make_shared<std_msgs::Time>();
    result->sec = i;
    result->nsec = i + 500;
    results.push_back(result);
  }

  feedbacks[0].resize(1);

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>("test_action", node_info)
      .send_action_goal_with_preempt(goals, feedbacks, results[1])
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action client
        std_msgs::UInt32 goal;
        std::vector<std_msgs::UInt32> feedback;
        std_msgs::Time result;
        std::shared_ptr<ActionClient> actcli =
            node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::Time>(
                "test_action",
                [&feedback, &actcli](const std_msgs::UInt32& fb) {
                  static bool first_feedback = true;
                  feedback.push_back(fb);
                  // Preempt after first feedback
                  if (first_feedback) {
                    first_feedback = false;
                    std_msgs::UInt32 preempt_goal;
                    preempt_goal.data = 2;
                    bool send_result = actcli->dispatch(preempt_goal);
                    EXPECT_TRUE(send_result);
                  }
                },
                [&result](const std_msgs::Time& res) { result = res; });
        EXPECT_NE(actcli, nullptr);
        EXPECT_TRUE(actcli->ok());

        std::thread thr([&node]() { node.spin(); });

        // Send action goal and receive feedback/result
        goal.data = 1;
        bool send_result = actcli->dispatch(goal);
        EXPECT_TRUE(send_result);

        EXPECT_TRUE(actcli->wait_for_result(Duration(5.0))); // Wait up to 5 seconds for result
        EXPECT_EQ(feedback.size(), 4);
        EXPECT_EQ(feedback[0].data, 11); // From first goal
        EXPECT_EQ(feedback[1].data, 21); // From preempted goal
        EXPECT_EQ(feedback[2].data, 22);
        EXPECT_EQ(feedback[3].data, 23);
        EXPECT_EQ(result.sec, 2);
        EXPECT_EQ(result.nsec, 502);

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}