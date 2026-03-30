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
#include "rix/std_msgs/Time.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/sys_msgs/SubNotify.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/test/test_fixture.hpp"
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>

using namespace rix;

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

// ---------------------------------------------------------------------------
// Tests ported from message_test.cpp
// ---------------------------------------------------------------------------

TEST(NodeTestNew, PublisherAcceptConnectionsAndPublish) {
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
        auto acceptors = fixture.get_acceptors();
        auto connection_streams = fixture.get_connection_streams();

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
          EXPECT_TRUE(acceptors[0]->wait_for_operations(3, std::chrono::milliseconds(5000)));
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

TEST(NodeTestNew, SubscriberConnectAndReceive) {
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
  std::shared_ptr<MockStream> client1, client2, client3;
  TestFixture()
      .enable_poller(3)
      .create_node("test_node", node_info)
      .create_subscriber<std_msgs::UInt32>("test_topic", sub_info, node_info, false, 1)
      .accept_notification<std_msgs::UInt32>(
          "test_topic", {Endpoint("127.0.0.1", 8002), Endpoint("127.0.0.1", 8003), Endpoint("127.0.0.1", 8004)})
      .enable_operation_notifications()
      .connect_to_publisher(Endpoint("127.0.0.1", 8002), messages, client1)
      .connect_to_publisher(Endpoint("127.0.0.1", 8003), messages, client2)
      .connect_to_publisher(Endpoint("127.0.0.1", 8004), messages, client3)
      .disable_operation_notifications()
      .destroy_subscriber(sub_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
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

TEST(NodeTestNew, ServiceAcceptRequestAndRespond) {
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
      .create_service<std_msgs::UInt32, std_msgs::Time>("test_service", srv_info, node_info, false, 3)
      .accept_service_client(requests[0], responses[0])
      .accept_service_client(requests[1], responses[1])
      .accept_service_client(requests[2], responses[2])
      .disable_operation_notifications()
      .destroy_service(srv_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto acceptors = fixture.get_acceptors();
        auto srv_connections = fixture.get_connection_streams();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create service with callback
        auto callback = [](const std_msgs::UInt32& req, std_msgs::Time& res) {
          res.sec = req.data;
          res.nsec = req.data + 500;
        };

        auto srv = node.create_service<std_msgs::UInt32, std_msgs::Time>("test_service", callback);
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
          EXPECT_TRUE(acceptors[0]->wait_for_operations(3, std::chrono::milliseconds(5000)));
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

TEST(NodeTestNew, ActionAcceptGoalWithFeedbackAndResult) {
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
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto acceptors = fixture.get_acceptors();
        auto connections = fixture.get_connection_streams();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action with callback
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

        EXPECT_TRUE(acceptors[0]->wait_for_operations(1, std::chrono::milliseconds(5000)));
        // 2 recv (opcode & goal) + 1 send (status) + 3 send (feedback) + 1 send (result) = 7 operations
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

TEST(NodeTestNew, ActionAcceptGoalWithCancel) {
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
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto acceptors = fixture.get_acceptors();
        auto connections = fixture.get_connection_streams();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action with callback
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
        EXPECT_TRUE(acceptors[0]->wait_for_operations(1, std::chrono::milliseconds(5000)));
        auto conn_a = connections[0];
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

TEST(NodeTestNew, ActionAcceptGoalFailureAlreadyConnected) {
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
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto acceptors = fixture.get_acceptors();
        auto connections = fixture.get_connection_streams();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action with callback
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

        EXPECT_TRUE(acceptors[0]->wait_for_operations(2, std::chrono::milliseconds(5000)));
        auto conn_a = connections[0]; // First connection should succeed
        // 2 recv (opcode & goal) + 1 send (status) + 3 send (feedback) + 1 send (result) = 7 operations
        EXPECT_TRUE(conn_a->wait_for_operations(7, std::chrono::milliseconds(5000)));
        // 1 recv (goal) + 1 send (status) = 2 operations for failed connection
        auto conn_b = connections[1]; // Second connection should fail
        EXPECT_TRUE(conn_b->wait_for_operations(2, std::chrono::milliseconds(5000)));
        EXPECT_TRUE(act->ok());

        node.shutdown();
        if (thr.joinable()) {
          thr.join();
        }
      });
}

TEST(NodeTestNew, ActionAcceptGoalWithPreempt) {
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
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](const TestFixture& fixture) {
        auto acceptors = fixture.get_acceptors();
        auto connections = fixture.get_connection_streams();

        Node node("test_node");
        EXPECT_TRUE(node.ok());

        // Create action with callback
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

        EXPECT_TRUE(acceptors[0]->wait_for_operations(1, std::chrono::milliseconds(5000)));
        auto conn_a = connections[0];
        // 3 (2 recv (opcode & goal) + 1 send (status)) + 4 (2 recv (opcode & goal) + 1 send (status) + 1 send
        // (feedback)) + 7 (2 recv (opcode & goal) + 1 send (status) + 3 send (feedback) + 1 send (result)) = 14
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

TEST(NodeTestNew, ServiceClientRequestAndReceive) {
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
      .build<Node>([](const TestFixture& fixture) {
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

TEST(NodeTestNew, ActionClientDispatch) {
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
      .build<Node>([](const TestFixture& fixture) {
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

        EXPECT_TRUE(actcli->wait_for_result(Duration(5.0)));

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

TEST(NodeTestNew, ActionClientDispatchWithCancel) {
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
      .build<Node>([](const TestFixture& fixture) {
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

TEST(NodeTestNew, ActionClientDispatchFailure) {
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
      .build<Node>([](const TestFixture& fixture) {
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

TEST(NodeTestNew, ActionClientDispatchPreempt) {
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
      .build<Node>([](const TestFixture& fixture) {
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

        EXPECT_TRUE(actcli->wait_for_result(Duration(5.0)));
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
