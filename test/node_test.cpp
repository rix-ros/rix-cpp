#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/test/test_fixture.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(NodeTest, RegisterAndDeregisterNode) {
  // Clear, self-documenting test
  msg::mediator::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info).destroy_node(node_info).build<Node>([](TestFixture& fixture) {
    Node node("test_node");
    EXPECT_TRUE(node.ok()); // Node destructor triggers deregistration
  });
}

TEST(NodeTest, PingNode) {
  // Clear, self-documenting test
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info, false, 1)
      .accept_ping()
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        node.spin_once(); // Handle ping
      });
}

TEST(NodeTest, RegisterNodeFailure) {
  msg::mediator::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info, true).build<Node>([](TestFixture& fixture) {
    Node node("test_node");
    EXPECT_FALSE(node.ok());
  });
}

TEST(NodeTest, RegisterComponentsAfterNodeShutdown) {
  msg::mediator::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info, true).build<Node>([](TestFixture& fixture) {
    Node node("test_node");
    EXPECT_FALSE(node.ok());

    auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");
    EXPECT_EQ(pub, nullptr);

    auto sub = node.create_subscriber<msg::standard::UInt32>("test_topic", [](const msg::standard::UInt32&) {});
    EXPECT_EQ(sub, nullptr);

    auto srv = node.create_service<msg::standard::UInt32, msg::standard::Time>(
        "test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
    EXPECT_EQ(srv, nullptr);

    auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");
    EXPECT_EQ(srv_cli, nullptr);

    auto act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
        "test_action",
        [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
    EXPECT_EQ(act, nullptr);

    auto act_cli =
        node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");
    EXPECT_EQ(act_cli, nullptr);
  });
}

TEST(NodeTest, RegisterComponentsAfterManualNodeShutdown) {
  msg::mediator::NodeInfo node_info;
  TestFixture().create_node("test_node", node_info).destroy_node(node_info).build<Node>([](TestFixture& fixture) {
    Node node("test_node");
    EXPECT_TRUE(node.ok());

    node.shutdown();
    EXPECT_FALSE(node.ok());

    auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");
    EXPECT_EQ(pub, nullptr);

    auto sub = node.create_subscriber<msg::standard::UInt32>("test_topic", [](const msg::standard::UInt32&) {});
    EXPECT_EQ(sub, nullptr);

    auto srv = node.create_service<msg::standard::UInt32, msg::standard::Time>(
        "test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
    EXPECT_EQ(srv, nullptr);

    auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");
    EXPECT_EQ(srv_cli, nullptr);

    auto act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
        "test_action",
        [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
    EXPECT_EQ(act, nullptr);

    auto act_cli =
        node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");
    EXPECT_EQ(act_cli, nullptr);
  });
}

TEST(NodeTest, RegisterAndDeregisterPublisher) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info)
      .destroy_node(node_info)
      .destroy_publisher(pub_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");

        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
      });
}

TEST(NodeTest, RegisterPublisherFailure) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");

        EXPECT_NE(pub, nullptr);
        EXPECT_FALSE(pub->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterPublisherFromShutdown) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info)
      .destroy_publisher(pub_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");

        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        pub->shutdown();
        EXPECT_FALSE(pub->ok());

        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

TEST(NodeTest, RegisterSubscriberFailure) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const msg::standard::UInt32&) {});

        EXPECT_NE(sub, nullptr);
        EXPECT_FALSE(sub->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterSubscriber) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info)
      .destroy_node(node_info)
      .destroy_subscriber(sub_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const msg::standard::UInt32&) {});

        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterSubscriberFromShutdown) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::SubInfo sub_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info)
      .destroy_subscriber(sub_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto sub = node.create_subscriber("test_topic", [](const msg::standard::UInt32&) {});

        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        sub->shutdown();
        EXPECT_FALSE(sub->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

TEST(NodeTest, RegisterServiceFailure) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_service", srv_info, node_info, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});

        EXPECT_NE(srv, nullptr);
        EXPECT_FALSE(srv->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterService) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_service", srv_info, node_info)
      .destroy_node(node_info)
      .destroy_service(srv_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});

        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterServiceFromShutdown) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::SrvInfo srv_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_service", srv_info, node_info)
      .destroy_service(srv_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv = node.create_service("test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});

        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        srv->shutdown();
        EXPECT_FALSE(srv->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

TEST(NodeTest, RequestServiceClientFailure) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");

        EXPECT_NE(srv_cli, nullptr);
        EXPECT_FALSE(srv_cli->ok());
      });
}

TEST(NodeTest, RequestServiceClient) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");

        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
      });
}

TEST(NodeTest, RequestServiceClientShutdown) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");

        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());

        srv_cli->shutdown();
        EXPECT_FALSE(srv_cli->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

TEST(NodeTest, RegisterActionFailure) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", act_info, node_info, 0, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action(
            "test_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });

        EXPECT_NE(act, nullptr);
        EXPECT_FALSE(act->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterAction) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", act_info, node_info)
      .destroy_node(node_info)
      .destroy_action(act_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action(
            "test_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return false; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterActionFromShutdown) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::ActInfo act_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", act_info, node_info)
      .destroy_action(act_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act = node.create_action(
            "test_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return false; });

        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        act->shutdown();
        EXPECT_FALSE(act->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

TEST(NodeTest, RequestActionClientFailure) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", node_info, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");

        EXPECT_NE(act_cli, nullptr);
        EXPECT_FALSE(act_cli->ok());
      });
}

TEST(NodeTest, RequestActionClient) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action", node_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");

        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());
      });
}

TEST(NodeTest, RequestActionClientShutdown) {
  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action", node_info)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");

        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());

        act_cli->shutdown();
        EXPECT_FALSE(act_cli->ok());
        node.spin_once();
        EXPECT_TRUE(node.ok());
      });
}

TEST(NodeTest, RegisterAndDeregisterMultipleOfAll) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info, pub_info2;
  msg::mediator::SubInfo sub_info, sub_info2;
  msg::mediator::SrvInfo srv_info, srv_info2;
  msg::mediator::ActInfo act_info, act_info2;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_service", srv_info, node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", act_info, node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action", node_info)
      .create_publisher<msg::standard::UInt32>("other_topic", pub_info2, node_info)
      .create_subscriber<msg::standard::UInt32>("other_topic", sub_info2, node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("other_service", srv_info2, node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("other_service", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "other_action", act_info2, node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("other_action",
                                                                                               node_info)
      .destroy_publisher(pub_info)
      .destroy_subscriber(sub_info)
      .destroy_service(srv_info)
      .destroy_action(act_info)
      .destroy_publisher(pub_info2)
      .destroy_subscriber(sub_info2)
      .destroy_service(srv_info2)
      .destroy_action(act_info2)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());

        auto sub = node.create_subscriber("test_topic", [](const msg::standard::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());

        auto srv = node.create_service("test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());

        auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());

        auto act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
            "test_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());

        auto act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());

        auto other_pub = node.create_publisher<msg::standard::UInt32>("other_topic");
        EXPECT_NE(other_pub, nullptr);
        EXPECT_TRUE(other_pub->ok());

        auto other_sub = node.create_subscriber("other_topic", [](const msg::standard::UInt32&) {});
        EXPECT_NE(other_sub, nullptr);
        EXPECT_TRUE(other_sub->ok());

        auto other_srv =
            node.create_service("other_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
        EXPECT_NE(other_srv, nullptr);
        EXPECT_TRUE(other_srv->ok());

        auto other_srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("other_service");
        EXPECT_NE(other_srv_cli, nullptr);
        EXPECT_TRUE(other_srv_cli->ok());

        auto other_act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
            "other_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
        EXPECT_NE(other_act, nullptr);
        EXPECT_TRUE(other_act->ok());

        auto other_act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
                "other_action");
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

TEST(NodeTest, PreserveComponentOrderOnDeregistration) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info, pub_info2;
  msg::mediator::SubInfo sub_info, sub_info2;
  msg::mediator::SrvInfo srv_info, srv_info2;
  msg::mediator::ActInfo act_info, act_info2;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_service", srv_info, node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", act_info, node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action", node_info)
      .create_publisher<msg::standard::UInt32>("other_topic", pub_info2, node_info)
      .create_subscriber<msg::standard::UInt32>("other_topic", sub_info2, node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("other_service", srv_info2, node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("other_service", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "other_action", act_info2, node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("other_action",
                                                                                               node_info)
      .destroy_node(node_info)
      .destroy_action(act_info2)
      .destroy_service(srv_info2)
      .destroy_subscriber(sub_info2)
      .destroy_publisher(pub_info2)
      .destroy_action(act_info)
      .destroy_service(srv_info)
      .destroy_subscriber(sub_info)
      .destroy_publisher(pub_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
        pub = nullptr;

        auto sub = node.create_subscriber("test_topic", [](const msg::standard::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
        sub = nullptr;

        auto srv = node.create_service("test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
        srv = nullptr;

        auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
        srv_cli = nullptr;

        auto act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
            "test_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
        act = nullptr;

        auto act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());
        act_cli = nullptr;

        auto other_pub = node.create_publisher<msg::standard::UInt32>("other_topic");
        EXPECT_NE(other_pub, nullptr);
        EXPECT_TRUE(other_pub->ok());
        other_pub = nullptr;

        auto other_sub = node.create_subscriber("other_topic", [](const msg::standard::UInt32&) {});
        EXPECT_NE(other_sub, nullptr);
        EXPECT_TRUE(other_sub->ok());
        other_sub = nullptr;

        auto other_srv =
            node.create_service("other_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
        EXPECT_NE(other_srv, nullptr);
        EXPECT_TRUE(other_srv->ok());
        other_srv = nullptr;

        auto other_srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("other_service");
        EXPECT_NE(other_srv_cli, nullptr);
        EXPECT_TRUE(other_srv_cli->ok());
        other_srv_cli = nullptr;

        auto other_act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
            "other_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
        EXPECT_NE(other_act, nullptr);
        EXPECT_TRUE(other_act->ok());
        other_act = nullptr;

        auto other_act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
                "other_action");
        EXPECT_NE(other_act_cli, nullptr);
        EXPECT_TRUE(other_act_cli->ok());
        other_act_cli = nullptr;
      });
}

TEST(NodeTest, ShutdownFromSignal) {
  msg::mediator::NodeInfo node_info;
  msg::mediator::PubInfo pub_info, pub_info2;
  msg::mediator::SubInfo sub_info, sub_info2;
  msg::mediator::SrvInfo srv_info, srv_info2;
  msg::mediator::ActInfo act_info, act_info2;
  TestFixture()
      .create_node("test_node", node_info)
      .create_publisher<msg::standard::UInt32>("test_topic", pub_info, node_info)
      .create_subscriber<msg::standard::UInt32>("test_topic", sub_info, node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("test_service", srv_info, node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "test_action", act_info, node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action", node_info)
      .create_publisher<msg::standard::UInt32>("other_topic", pub_info2, node_info)
      .create_subscriber<msg::standard::UInt32>("other_topic", sub_info2, node_info)
      .create_service<msg::standard::UInt32, msg::standard::Time>("other_service", srv_info2, node_info)
      .create_service_client<msg::standard::UInt32, msg::standard::Time>("other_service", node_info)
      .create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
          "other_action", act_info2, node_info)
      .create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("other_action",
                                                                                               node_info)
      .destroy_publisher(pub_info)
      .destroy_subscriber(sub_info)
      .destroy_service(srv_info)
      .destroy_action(act_info)
      .destroy_publisher(pub_info2)
      .destroy_subscriber(sub_info2)
      .destroy_service(srv_info2)
      .destroy_action(act_info2)
      .destroy_node(
          node_info) // Node destroyed last because other components will be destroyed during spin_once after signal
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        auto pub = node.create_publisher<msg::standard::UInt32>("test_topic");
        EXPECT_NE(pub, nullptr);
        EXPECT_TRUE(pub->ok());
        pub = nullptr;

        auto sub = node.create_subscriber("test_topic", [](const msg::standard::UInt32&) {});
        EXPECT_NE(sub, nullptr);
        EXPECT_TRUE(sub->ok());
        sub = nullptr;

        auto srv = node.create_service("test_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
        EXPECT_NE(srv, nullptr);
        EXPECT_TRUE(srv->ok());
        srv = nullptr;

        auto srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("test_service");
        EXPECT_NE(srv_cli, nullptr);
        EXPECT_TRUE(srv_cli->ok());
        srv_cli = nullptr;

        auto act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
            "test_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
        EXPECT_NE(act, nullptr);
        EXPECT_TRUE(act->ok());
        act = nullptr;

        auto act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>("test_action");
        EXPECT_NE(act_cli, nullptr);
        EXPECT_TRUE(act_cli->ok());
        act_cli = nullptr;

        auto other_pub = node.create_publisher<msg::standard::UInt32>("other_topic");
        EXPECT_NE(other_pub, nullptr);
        EXPECT_TRUE(other_pub->ok());
        other_pub = nullptr;

        auto other_sub = node.create_subscriber("other_topic", [](const msg::standard::UInt32&) {});
        EXPECT_NE(other_sub, nullptr);
        EXPECT_TRUE(other_sub->ok());
        other_sub = nullptr;

        auto other_srv =
            node.create_service("other_service", [](const msg::standard::UInt32&, msg::standard::Time&) {});
        EXPECT_NE(other_srv, nullptr);
        EXPECT_TRUE(other_srv->ok());
        other_srv = nullptr;

        auto other_srv_cli = node.create_service_client<msg::standard::UInt32, msg::standard::Time>("other_service");
        EXPECT_NE(other_srv_cli, nullptr);
        EXPECT_TRUE(other_srv_cli->ok());
        other_srv_cli = nullptr;

        auto other_act = node.create_action<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
            "other_action",
            [](const msg::standard::UInt32&, msg::standard::UInt32&, msg::standard::Time&) -> bool { return true; });
        EXPECT_NE(other_act, nullptr);
        EXPECT_TRUE(other_act->ok());
        other_act = nullptr;

        auto other_act_cli =
            node.create_action_client<msg::standard::UInt32, msg::standard::UInt32, msg::standard::Time>(
                "other_action");
        EXPECT_NE(other_act_cli, nullptr);
        EXPECT_TRUE(other_act_cli->ok());
        other_act_cli = nullptr;

        auto sig = Spinner::get_shutdown_signal();
        EXPECT_TRUE(sig->raise());
        // (TODO: Enable some synchronization mechanism to avoid this sleep)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        node.spin_once(); // Register the signal
        node.spin_once(); // Shutdown all components
        EXPECT_FALSE(node.ok());
      });
}

TEST(NodeTest, ParameterSetRequest) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .set_parameter("test_param", node_info, param)
      .destroy_node(node_info)
      .build<Node>([param](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        bool status = node.set_parameter("test_param", *param);
        EXPECT_TRUE(status);
      });
}

TEST(NodeTest, ParameterSetRequestFailure) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .set_parameter("test_param", node_info, param, true)
      .destroy_node(node_info)
      .build<Node>([param](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        bool status = node.set_parameter("test_param", *param);
        EXPECT_FALSE(status);
      });
}

TEST(NodeTest, ParameterGetRequest) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .get_parameter("test_param", node_info, param)
      .destroy_node(node_info)
      .build<Node>([param](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        msg::standard::String param_received;
        bool status = node.get_parameter("test_param", param_received);
        EXPECT_TRUE(status);
        EXPECT_EQ(param_received.data, param->data);
      });
}

TEST(NodeTest, ParameterGetRequestFailure) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .get_parameter("test_param", node_info, param, true)
      .destroy_node(node_info)
      .build<Node>([](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        msg::standard::String param_received;
        bool status = node.get_parameter("test_param", param_received);
        EXPECT_FALSE(status);
      });
}

TEST(NodeTest, SystemInfoGetRequest) {
  msg::mediator::SystemInfo sys_info;
  sys_info.nodes.resize(1);
  sys_info.nodes[0].name = "test_node";
  sys_info.nodes[0].id = 1;
  sys_info.publishers.resize(1);
  sys_info.publishers[0].topic_info.name = "test_topic";
  sys_info.publishers[0].topic_info.message_hash = msg::standard::UInt32().hash();
  sys_info.publishers[0].endpoint.address = "127.0.0.1";
  sys_info.publishers[0].endpoint.port = 1234;
  sys_info.subscribers.resize(1);
  sys_info.subscribers[0].topic_info.name = "test_topic";
  sys_info.subscribers[0].topic_info.message_hash = msg::standard::UInt32().hash();
  sys_info.subscribers[0].endpoint.address = "127.0.0.1";
  sys_info.subscribers[0].endpoint.port = 5678;
  sys_info.services.resize(1);
  sys_info.services[0].name = "test_service";
  sys_info.services[0].request_hash = msg::standard::UInt32().hash();
  sys_info.services[0].response_hash = msg::standard::Time().hash();
  sys_info.services[0].endpoint.address = "127.0.0.1";
  sys_info.services[0].endpoint.port = 9012;
  sys_info.topics.resize(1);
  sys_info.topics[0].name = "test_topic";
  sys_info.topics[0].message_hash = msg::standard::UInt32().hash();
  sys_info.services.resize(1);

  msg::mediator::NodeInfo node_info;
  TestFixture()
      .create_node("test_node", node_info)
      .get_system_info(sys_info, node_info)
      .destroy_node(node_info)
      .build<Node>([sys_info](TestFixture& fixture) {
        Node node("test_node");
        EXPECT_TRUE(node.ok());

        msg::mediator::SystemInfo sys_info_received;
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