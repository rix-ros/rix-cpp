/**
 * @file node_test_new.cpp
 * @brief Example tests using Fixture 1 (TestFixture from test_fixture_new.hpp).
 *
 * These tests exercise Node's IPC lifecycle through the updated Acceptor/Stream
 * mock infrastructure. The API mirrors the old TestFixture but uses MockAcceptor
 * and MockStream instead of the deleted GenericSocket/MockSocket.
 */

#include "rix/core/spinner.hpp"
#include "rix/std_msgs/Header.hpp"
#include "rix/std_msgs/String.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/test/test_fixture.hpp"
#include <gtest/gtest.h>
#include <thread>

using namespace rix;

// =============================================================================
// Basic Node registration / deregistration
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterNode) {
  sys_msgs::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info).destroy_node(node_info).build<Node>([](const TestFixture&) {
    Node node("test_node");
    EXPECT_TRUE(node.ok());
  });
}

TEST(NodeTestNew, RegisterNodeFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info, true).build<Node>([](const TestFixture&) {
    Node node("test_node");
    EXPECT_FALSE(node.ok());
  });
}

// =============================================================================
// Publisher lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterPublisher) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
      });
}

TEST(NodeTestNew, RegisterPublisherFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_FALSE(pub->ok());
      });
}

// =============================================================================
// Subscriber lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterSubscriber) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .destroy_subscriber(sub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
      });
}

// =============================================================================
// Service lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterService) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .destroy_service(srv_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
      });
}

// =============================================================================
// Service client
// =============================================================================

TEST(NodeTestNew, RequestServiceClient) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
      });
}

// =============================================================================
// Action lifecycle
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterAction) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
      });
}

// =============================================================================
// Publisher with messages (using clock + operation notifications)
// =============================================================================

TEST(NodeTestNew, PublisherSendsMessages) {
  // Prepare expected messages
  std::vector<std::shared_ptr<std_msgs::Header>> messages;
  for (int i = 0; i < 3; i++) {
    auto msg = std::make_shared<std_msgs::Header>();
    msg->seq = i + 1;
    msg->frame_id = "test_frame";
    msg->stamp.sec = i + 1;
    msg->stamp.nsec = 0;
    messages.push_back(msg);
  }

  std::shared_ptr<MockClock> clock;
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  std::shared_ptr<MockStream> conn;
  TestFixture()
      .enable_clock(clock)
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::Header>("test_topic", pub_info, node_info, false, 1)
      .enable_operation_notifications()
      .accept_subscriber(messages, conn)
      .disable_operation_notifications()
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([&clock, &conn](const TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::Header>("test_topic");
        EXPECT_TRUE(pub->ok());

        // Accept the subscriber connection (Publisher::on_spin)
        node.spin_once();

        // Simulate timed message publication
        std_msgs::Header msg;
        msg.frame_id = "test_frame";
        for (int i = 0; i < 3; i++) {
          clock->sleep_for(Duration(1.0));
          msg.seq = i + 1;
          msg.stamp.sec = i + 1;
          msg.stamp.nsec = 0;
          pub->publish(msg);
          EXPECT_TRUE(conn->wait_for_operations(1, std::chrono::milliseconds(1000)));
        }
      });
}

// =============================================================================
// Multiple components
// =============================================================================

TEST(NodeTestNew, RegisterMultipleComponents) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  sys_msgs::SubInfo sub_info;
  sys_msgs::SrvInfo srv_info;
  sys_msgs::ActInfo act_info;

  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .destroy_publisher(pub_info)
      .destroy_subscriber(sub_info)
      .destroy_service(srv_info)
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        // Shutdown components manually
        pub->shutdown();
        node.spin_once();
        pub = nullptr;

        sub->shutdown();
        node.spin_once();
        sub = nullptr;

        srv->shutdown();
        node.spin_once();
        srv = nullptr;

        act->shutdown();
        node.spin_once();
        act = nullptr;

        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Ping handling
// =============================================================================

TEST(NodeTestNew, PingNode) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info, false, 1)
      .accept_ping(node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());
        node.spin_once();
      });
}

// =============================================================================
// Component registration after node shutdown
// =============================================================================

TEST(NodeTestNew, RegisterComponentsAfterNodeShutdown) {
  sys_msgs::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info, true).build<Node>([](const TestFixture&) {
    Node node("test_node");
    EXPECT_FALSE(node.ok());

    auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
    EXPECT_EQ(pub, nullptr);

    auto sub = node.create_subscriber<std_msgs::UInt32>("test_topic", [](const std_msgs::UInt32&) {});
    EXPECT_EQ(sub, nullptr);

    auto srv = node.create_service<std_msgs::UInt32, std_msgs::String>(
        "test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
    EXPECT_EQ(srv, nullptr);

    auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
    EXPECT_EQ(srv_cli, nullptr);

    auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
        "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
    EXPECT_EQ(act, nullptr);

    auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
        "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
    EXPECT_EQ(act_cli, nullptr);
  });
}

TEST(NodeTestNew, RegisterComponentsAfterManualNodeShutdown) {
  sys_msgs::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info).destroy_node(node_info).build<Node>([](const TestFixture&) {
    Node node("test_node");
    EXPECT_TRUE(node.ok());

    node.shutdown();
    EXPECT_FALSE(node.ok());

    auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
    EXPECT_EQ(pub, nullptr);

    auto sub = node.create_subscriber<std_msgs::UInt32>("test_topic", [](const std_msgs::UInt32&) {});
    EXPECT_EQ(sub, nullptr);

    auto srv = node.create_service<std_msgs::UInt32, std_msgs::String>(
        "test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
    EXPECT_EQ(srv, nullptr);

    auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
    EXPECT_EQ(srv_cli, nullptr);

    auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
        "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
    EXPECT_EQ(act, nullptr);

    auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
        "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
    EXPECT_EQ(act_cli, nullptr);
  });
}

// =============================================================================
// Publisher shutdown from component
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterPublisherFromShutdown) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        pub->shutdown();
        EXPECT_FALSE(pub->ok());

        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Subscriber failure and shutdown
// =============================================================================

TEST(NodeTestNew, RegisterSubscriberFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_FALSE(sub->ok());
      });
}

TEST(NodeTestNew, RegisterAndDeregisterSubscriberFromShutdown) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .destroy_subscriber(sub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        sub->shutdown();
        EXPECT_FALSE(sub->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Service failure and shutdown
// =============================================================================

TEST(NodeTestNew, RegisterServiceFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_FALSE(srv->ok());
      });
}

TEST(NodeTestNew, RegisterAndDeregisterServiceFromShutdown) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .destroy_service(srv_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        srv->shutdown();
        EXPECT_FALSE(srv->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Service client failure and shutdown
// =============================================================================

TEST(NodeTestNew, RequestServiceClientFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_FALSE(srv_cli->ok());
      });
}

TEST(NodeTestNew, RequestServiceClientShutdown) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());

        srv_cli->shutdown();
        EXPECT_FALSE(srv_cli->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Action failure and shutdown
// =============================================================================

TEST(NodeTestNew, RegisterActionFailure) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info, 0, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_FALSE(act->ok());
      });
}

TEST(NodeTestNew, RegisterAndDeregisterActionFromShutdown) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return false; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        act->shutdown();
        EXPECT_FALSE(act->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Action client lifecycle
// =============================================================================

TEST(NodeTestNew, RequestActionClientFailure) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", node_info, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(act_cli, nullptr);
        EXPECT_FALSE(act_cli->ok());
      });
}

TEST(NodeTestNew, RequestActionClient) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());
      });
}

TEST(NodeTestNew, RequestActionClientShutdown) {
  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", node_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());

        act_cli->shutdown();
        EXPECT_FALSE(act_cli->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Register and deregister multiple of all component types
// =============================================================================

TEST(NodeTestNew, RegisterAndDeregisterMultipleOfAll) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info, pub_info2;
  sys_msgs::SubInfo sub_info, sub_info2;
  sys_msgs::SrvInfo srv_info, srv_info2;
  sys_msgs::ActInfo act_info, act_info2;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", node_info)
      .create_publisher<std_msgs::UInt32>("other_topic", pub_info2, node_info)
      .create_subscriber<std_msgs::UInt32>("other_topic", sub_info2, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("other_service", srv_info2, node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("other_service", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("other_action", act_info2, node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("other_action", node_info)
      .destroy_publisher(pub_info)
      .destroy_subscriber(sub_info)
      .destroy_service(srv_info)
      .destroy_action(act_info)
      .destroy_publisher(pub_info2)
      .destroy_subscriber(sub_info2)
      .destroy_service(srv_info2)
      .destroy_action(act_info2)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());

        auto other_pub = node.create_publisher<std_msgs::UInt32>("other_topic");
        EXPECT_NE(other_pub, nullptr);
        EXPECT_TRUE(other_pub->ok());

        auto other_sub = node.create_subscriber("other_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(other_sub, nullptr);
        EXPECT_TRUE(other_sub->ok());

        auto other_srv = node.create_service("other_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(other_srv, nullptr);
        EXPECT_TRUE(other_srv->ok());

        auto other_srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("other_service");
        EXPECT_NE(other_srv_cli, nullptr);
        EXPECT_TRUE(other_srv_cli->ok());

        auto other_act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "other_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(other_act, nullptr);
        EXPECT_TRUE(other_act->ok());

        auto other_act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "other_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(other_act_cli, nullptr);
        EXPECT_TRUE(other_act_cli->ok());

        pub->shutdown();
        EXPECT_FALSE(pub->ok());
        pub = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        sub->shutdown();
        EXPECT_FALSE(sub->ok());
        sub = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        srv->shutdown();
        EXPECT_FALSE(srv->ok());
        srv = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        srv_cli->shutdown();
        EXPECT_FALSE(srv_cli->ok());
        srv_cli = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        act->shutdown();
        EXPECT_FALSE(act->ok());
        act = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        act_cli->shutdown();
        EXPECT_FALSE(act_cli->ok());
        act_cli = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        other_pub->shutdown();
        EXPECT_FALSE(other_pub->ok());
        other_pub = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        other_sub->shutdown();
        EXPECT_FALSE(other_sub->ok());
        other_sub = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        other_srv->shutdown();
        EXPECT_FALSE(other_srv->ok());
        other_srv = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        other_srv_cli->shutdown();
        EXPECT_FALSE(other_srv_cli->ok());
        other_srv_cli = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        other_act->shutdown();
        EXPECT_FALSE(other_act->ok());
        other_act = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());

        other_act_cli->shutdown();
        EXPECT_FALSE(other_act_cli->ok());
        other_act_cli = nullptr;
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

// =============================================================================
// Preserve component order on deregistration
// =============================================================================

TEST(NodeTestNew, PreserveComponentOrderOnDeregistration) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info, pub_info2;
  sys_msgs::SubInfo sub_info, sub_info2;
  sys_msgs::SrvInfo srv_info, srv_info2;
  sys_msgs::ActInfo act_info, act_info2;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", node_info)
      .create_publisher<std_msgs::UInt32>("other_topic", pub_info2, node_info)
      .create_subscriber<std_msgs::UInt32>("other_topic", sub_info2, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("other_service", srv_info2, node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("other_service", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("other_action", act_info2, node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("other_action", node_info)
      .destroy_action(act_info2)
      .destroy_service(srv_info2)
      .destroy_subscriber(sub_info2)
      .destroy_publisher(pub_info2)
      .destroy_action(act_info)
      .destroy_service(srv_info)
      .destroy_subscriber(sub_info)
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
        pub = nullptr;

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
        sub = nullptr;

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
        srv = nullptr;

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
        srv_cli = nullptr;

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
        act = nullptr;

        auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());
        act_cli = nullptr;

        auto other_pub = node.create_publisher<std_msgs::UInt32>("other_topic");
        EXPECT_NE(other_pub, nullptr);
        EXPECT_TRUE(other_pub->ok());
        other_pub = nullptr;

        auto other_sub = node.create_subscriber("other_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(other_sub, nullptr);
        EXPECT_TRUE(other_sub->ok());
        other_sub = nullptr;

        auto other_srv = node.create_service("other_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(other_srv, nullptr);
        EXPECT_TRUE(other_srv->ok());
        other_srv = nullptr;

        auto other_srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("other_service");
        EXPECT_NE(other_srv_cli, nullptr);
        EXPECT_TRUE(other_srv_cli->ok());
        other_srv_cli = nullptr;

        auto other_act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "other_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(other_act, nullptr);
        EXPECT_TRUE(other_act->ok());
        other_act = nullptr;

        auto other_act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "other_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(other_act_cli, nullptr);
        EXPECT_TRUE(other_act_cli->ok());
        other_act_cli = nullptr;
      });
}

// =============================================================================
// Shutdown from signal
// =============================================================================

TEST(NodeTestNew, ShutdownFromSignal) {
  sys_msgs::NodeInfo node_info;
  sys_msgs::PubInfo pub_info, pub_info2;
  sys_msgs::SubInfo sub_info, sub_info2;
  sys_msgs::SrvInfo srv_info, srv_info2;
  sys_msgs::ActInfo act_info, act_info2;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<std_msgs::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("test_service", srv_info, node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("test_service", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", act_info, node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("test_action", node_info)
      .create_publisher<std_msgs::UInt32>("other_topic", pub_info2, node_info)
      .create_subscriber<std_msgs::UInt32>("other_topic", sub_info2, node_info)
      .create_service<std_msgs::UInt32, std_msgs::String>("other_service", srv_info2, node_info)
      .create_service_client<std_msgs::UInt32, std_msgs::String>("other_service", node_info)
      .create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("other_action", act_info2, node_info)
      .create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>("other_action", node_info)
      .destroy_publisher(pub_info)
      .destroy_subscriber(sub_info)
      .destroy_service(srv_info)
      .destroy_action(act_info)
      .destroy_publisher(pub_info2)
      .destroy_subscriber(sub_info2)
      .destroy_service(srv_info2)
      .destroy_action(act_info2)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<std_msgs::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
        pub = nullptr;

        auto sub = node.create_subscriber("test_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
        sub = nullptr;

        auto srv = node.create_service("test_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
        srv = nullptr;

        auto srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
        srv_cli = nullptr;

        auto act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
        act = nullptr;

        auto act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "test_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());
        act_cli = nullptr;

        auto other_pub = node.create_publisher<std_msgs::UInt32>("other_topic");
        EXPECT_NE(other_pub, nullptr);
        EXPECT_TRUE(other_pub->ok());
        other_pub = nullptr;

        auto other_sub = node.create_subscriber("other_topic", [](const std_msgs::UInt32&) {});
        EXPECT_NE(other_sub, nullptr);
        EXPECT_TRUE(other_sub->ok());
        other_sub = nullptr;

        auto other_srv = node.create_service("other_service", [](const std_msgs::UInt32&, std_msgs::String&) {});
        EXPECT_NE(other_srv, nullptr);
        EXPECT_TRUE(other_srv->ok());
        other_srv = nullptr;

        auto other_srv_cli = node.create_service_client<std_msgs::UInt32, std_msgs::String>("other_service");
        EXPECT_NE(other_srv_cli, nullptr);
        EXPECT_TRUE(other_srv_cli->ok());
        other_srv_cli = nullptr;

        auto other_act = node.create_action<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "other_action", [](const std_msgs::UInt32&, std_msgs::UInt32&, std_msgs::String&) -> bool { return true; });
        EXPECT_NE(other_act, nullptr);
        EXPECT_TRUE(other_act->ok());
        other_act = nullptr;

        auto other_act_cli = node.create_action_client<std_msgs::UInt32, std_msgs::UInt32, std_msgs::String>(
            "other_action", [](const std_msgs::UInt32&) {}, [](const std_msgs::String&) {});
        EXPECT_NE(other_act_cli, nullptr);
        EXPECT_TRUE(other_act_cli->ok());
        other_act_cli = nullptr;

        auto sig = Spinner::get_shutdown_signal();
        EXPECT_TRUE(sig->raise());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        node.spin_once();
        node.spin_once();
        EXPECT_FALSE(node.ok());

        Spinner::set_shutdown_signal(nullptr);
        Spinner::reset_signal_received();
      });
}

// =============================================================================
// Parameter server
// =============================================================================

TEST(NodeTestNew, ParameterSetRequest) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .set_parameter("test_param", node_info, param)
      .destroy_node(node_info)
      .build<Node>([param](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        bool status = node.set_parameter("test_param", *param);
        EXPECT_TRUE(status);
      });
}

TEST(NodeTestNew, ParameterSetRequestFailure) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .set_parameter("test_param", node_info, param, true)
      .destroy_node(node_info)
      .build<Node>([param](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        bool status = node.set_parameter("test_param", *param);
        EXPECT_FALSE(status);
      });
}

TEST(NodeTestNew, ParameterGetRequest) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .get_parameter("test_param", node_info, param)
      .destroy_node(node_info)
      .build<Node>([param](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        std_msgs::String param_received;
        bool status = node.get_parameter("test_param", param_received);
        EXPECT_TRUE(status);
        EXPECT_EQ(param_received.data, param->data);
      });
}

TEST(NodeTestNew, ParameterGetRequestFailure) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .get_parameter("test_param", node_info, param, true)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        std_msgs::String param_received;
        bool status = node.get_parameter("test_param", param_received);
        EXPECT_FALSE(status);
      });
}

// =============================================================================
// System info
// =============================================================================

TEST(NodeTestNew, SystemInfoGetRequest) {
  sys_msgs::SystemInfo sys_info;
  sys_info.nodes.resize(1);
  sys_info.nodes[0].name = "test_node";
  sys_info.nodes[0].id = 1;
  sys_info.publishers.resize(1);
  sys_info.publishers[0].topic_info.name = "test_topic";
  sys_info.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  sys_info.publishers[0].endpoint.address = "127.0.0.1";
  sys_info.publishers[0].endpoint.port = 1234;
  sys_info.subscribers.resize(1);
  sys_info.subscribers[0].topic_info.name = "test_topic";
  sys_info.subscribers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  sys_info.subscribers[0].endpoint.address = "127.0.0.1";
  sys_info.subscribers[0].endpoint.port = 5678;
  sys_info.services.resize(1);
  sys_info.services[0].name = "test_service";
  sys_info.services[0].request_hash = std_msgs::UInt32().hash();
  sys_info.services[0].response_hash = std_msgs::String().hash();
  sys_info.services[0].endpoint.address = "127.0.0.1";
  sys_info.services[0].endpoint.port = 9012;
  sys_info.topics.resize(1);
  sys_info.topics[0].name = "test_topic";
  sys_info.topics[0].message_hash = std_msgs::UInt32().hash();

  sys_msgs::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .get_system_info(sys_info, node_info)
      .destroy_node(node_info)
      .build<Node>([sys_info](const TestFixture&) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        sys_msgs::SystemInfo sys_info_received;
        bool status = node.get_system_info(sys_info_received);
        EXPECT_TRUE(status);
        EXPECT_EQ(sys_info_received.nodes.size(), sys_info.nodes.size());
        EXPECT_EQ(sys_info_received.nodes[0].name, sys_info.nodes[0].name);
        EXPECT_EQ(sys_info_received.nodes[0].id, sys_info.nodes[0].id);
        EXPECT_EQ(sys_info_received.publishers.size(), sys_info.publishers.size());
        EXPECT_EQ(sys_info_received.publishers[0].topic_info.name, sys_info.publishers[0].topic_info.name);
        EXPECT_EQ(sys_info_received.publishers[0].topic_info.message_hash,
                  sys_info.publishers[0].topic_info.message_hash);
        EXPECT_EQ(sys_info_received.publishers[0].endpoint.address, sys_info.publishers[0].endpoint.address);
        EXPECT_EQ(sys_info_received.publishers[0].endpoint.port, sys_info.publishers[0].endpoint.port);
        EXPECT_EQ(sys_info_received.subscribers.size(), sys_info.subscribers.size());
        EXPECT_EQ(sys_info_received.subscribers[0].topic_info.name, sys_info.subscribers[0].topic_info.name);
        EXPECT_EQ(sys_info_received.subscribers[0].topic_info.message_hash,
                  sys_info.subscribers[0].topic_info.message_hash);
        EXPECT_EQ(sys_info_received.subscribers[0].endpoint.address, sys_info.subscribers[0].endpoint.address);
        EXPECT_EQ(sys_info_received.subscribers[0].endpoint.port, sys_info.subscribers[0].endpoint.port);
        EXPECT_EQ(sys_info_received.services.size(), sys_info.services.size());
        EXPECT_EQ(sys_info_received.services[0].name, sys_info.services[0].name);
        EXPECT_EQ(sys_info_received.services[0].request_hash, sys_info.services[0].request_hash);
        EXPECT_EQ(sys_info_received.services[0].response_hash, sys_info.services[0].response_hash);
        EXPECT_EQ(sys_info_received.services[0].endpoint.address, sys_info.services[0].endpoint.address);
        EXPECT_EQ(sys_info_received.services[0].endpoint.port, sys_info.services[0].endpoint.port);
        EXPECT_EQ(sys_info_received.topics.size(), sys_info.topics.size());
        EXPECT_EQ(sys_info_received.topics[0].name, sys_info.topics[0].name);
        EXPECT_EQ(sys_info_received.topics[0].message_hash, sys_info.topics[0].message_hash);
      });
}
