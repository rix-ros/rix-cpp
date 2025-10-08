#include "rix/msg/standard/String.hpp"
#include "rix/test/node_test_fixture.hpp"
#include <gtest/gtest.h>

using namespace rix;

TEST(NodeRegistrationV2, RegisterAndDeregisterNode) {
  // Clear, self-documenting test
  auto fixture = NodeTestFixture().register_node().deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());
  } // Node destructor triggers deregistration
}

TEST(NodeRegistrationV2, RegisterNodeFailure) {
  auto fixture = NodeTestFixture().register_node(true);

  auto node = fixture.build();
  EXPECT_FALSE(node->ok());
}

TEST(NodeRegistrationV2, RegisterComponentsAfterNodeShutdown) {
  auto fixture = NodeTestFixture().register_node(true);

  {
    auto node = fixture.build();
    EXPECT_FALSE(node->ok());

    auto pub = node->create_publisher<msg::standard::UInt32>("test_topic",
                                                             Endpoint("127.0.0.1", 0));
    EXPECT_EQ(pub, nullptr);

    auto sub = node->create_subscriber<msg::standard::UInt32>(
        "test_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));
    EXPECT_EQ(sub, nullptr);

    auto srv = node->create_service<msg::standard::UInt32, msg::standard::Time>(
        "test_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));
    EXPECT_EQ(srv, nullptr);

    auto srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "test_service");
    EXPECT_EQ(srv_cli, nullptr);
  }
}

TEST(NodeRegistrationV2, RegisterComponentsAfterManualNodeShutdown) {
  auto fixture = NodeTestFixture().register_node().deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    node->shutdown();
    EXPECT_FALSE(node->ok());

    auto pub = node->create_publisher<msg::standard::UInt32>("test_topic",
                                                             Endpoint("127.0.0.1", 0));
    EXPECT_EQ(pub, nullptr);

    auto sub = node->create_subscriber<msg::standard::UInt32>(
        "test_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));
    EXPECT_EQ(sub, nullptr);

    auto srv = node->create_service<msg::standard::UInt32, msg::standard::Time>(
        "test_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));
    EXPECT_EQ(srv, nullptr);

    auto srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "test_service");
    EXPECT_EQ(srv_cli, nullptr);
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterPublisher) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_publisher("test_topic", msg::standard::UInt32().hash())
                     .deregister_node()
                     .deregister_publisher("test_topic", msg::standard::UInt32().hash());

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto pub = node->create_publisher<msg::standard::UInt32>("test_topic",
                                                             Endpoint("127.0.0.1", 0));

    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());
  }
}

TEST(NodeRegistrationV2, RegisterPublisherFailure) {
  auto fixture =
      NodeTestFixture()
          .register_node()
          .create_server()
          .register_publisher("test_topic", msg::standard::UInt32().hash(), true)
          .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto pub = node->create_publisher<msg::standard::UInt32>("test_topic",
                                                             Endpoint("127.0.0.1", 0));

    EXPECT_NE(pub, nullptr);
    EXPECT_FALSE(pub->ok());
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterPublisherFromShutdown) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_publisher("test_topic", msg::standard::UInt32().hash())
                     .deregister_publisher("test_topic", msg::standard::UInt32().hash())
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto pub = node->create_publisher<msg::standard::UInt32>("test_topic",
                                                             Endpoint("127.0.0.1", 0));

    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());

    pub->shutdown();
    EXPECT_FALSE(pub->ok());

    node->spin_once();
    EXPECT_TRUE(node->ok());
  }
}

TEST(NodeRegistrationV2, RegisterSubscriberFailure) {
  auto fixture =
      NodeTestFixture()
          .register_node()
          .create_server()
          .register_subscriber("test_topic", msg::standard::UInt32().hash(), true)
          .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto sub = node->create_subscriber(
        "test_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));

    EXPECT_NE(sub, nullptr);
    EXPECT_FALSE(sub->ok());
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterSubscriber) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_subscriber("test_topic", msg::standard::UInt32().hash())
                     .deregister_node()
                     .deregister_subscriber("test_topic", msg::standard::UInt32().hash());

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto sub = node->create_subscriber(
        "test_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));

    EXPECT_NE(sub, nullptr);
    EXPECT_TRUE(sub->ok());
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterSubscriberFromShutdown) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_subscriber("test_topic", msg::standard::UInt32().hash())
                     .deregister_subscriber("test_topic", msg::standard::UInt32().hash())
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto sub = node->create_subscriber(
        "test_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));

    EXPECT_NE(sub, nullptr);
    EXPECT_TRUE(sub->ok());

    sub->shutdown();
    EXPECT_FALSE(sub->ok());
    node->spin_once();
    EXPECT_TRUE(node->ok());
  }
}

TEST(NodeRegistrationV2, RegisterServiceFailure) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_service("test_service",
                                       msg::standard::UInt32().hash(),
                                       msg::standard::Time().hash(),
                                       true)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto srv = node->create_service(
        "test_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));

    EXPECT_NE(srv, nullptr);
    EXPECT_FALSE(srv->ok());
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterService) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_service("test_service",
                                       msg::standard::UInt32().hash(),
                                       msg::standard::Time().hash())
                     .deregister_node()
                     .deregister_service("test_service",
                                         msg::standard::UInt32().hash(),
                                         msg::standard::Time().hash());

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto srv = node->create_service(
        "test_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));

    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterServiceFromShutdown) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_service("test_service",
                                       msg::standard::UInt32().hash(),
                                       msg::standard::Time().hash())
                     .deregister_service("test_service",
                                         msg::standard::UInt32().hash(),
                                         msg::standard::Time().hash())
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto srv = node->create_service(
        "test_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));

    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());

    srv->shutdown();
    EXPECT_FALSE(srv->ok());
    node->spin_once();
    EXPECT_TRUE(node->ok());
  }
}

TEST(NodeRegistrationV2, RequestServiceClientFailure) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_service_client("test_service",
                                             msg::standard::UInt32().hash(),
                                             msg::standard::Time().hash(),
                                             true)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "test_service");

    EXPECT_NE(srv_cli, nullptr);
    EXPECT_FALSE(srv_cli->ok());
  }
}

TEST(NodeRegistrationV2, RequestServiceClient) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_service_client("test_service",
                                             msg::standard::UInt32().hash(),
                                             msg::standard::Time().hash())
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "test_service");

    EXPECT_NE(srv_cli, nullptr);
    EXPECT_TRUE(srv_cli->ok());
  }
}

TEST(NodeRegistrationV2, RequestServiceClientShutdown) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_service_client("test_service",
                                             msg::standard::UInt32().hash(),
                                             msg::standard::Time().hash())
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "test_service");

    EXPECT_NE(srv_cli, nullptr);
    EXPECT_TRUE(srv_cli->ok());

    srv_cli->shutdown();
    EXPECT_FALSE(srv_cli->ok());
    node->spin_once();
    EXPECT_TRUE(node->ok());
  }
}

TEST(NodeRegistrationV2, RegisterAndDeregisterMultipleOfAll) {
  auto fixture = NodeTestFixture()
                     .register_node()
                     .create_server()
                     .register_publisher("test_topic", msg::standard::UInt32().hash())
                     .create_server()
                     .register_subscriber("test_topic", msg::standard::UInt32().hash())
                     .create_server()
                     .register_service("test_service",
                                       msg::standard::UInt32().hash(),
                                       msg::standard::Time().hash())
                     .request_service_client("test_service",
                                             msg::standard::UInt32().hash(),
                                             msg::standard::Time().hash())
                     .create_server()
                     .register_publisher("other_topic", msg::standard::UInt32().hash())
                     .create_server()
                     .register_subscriber("other_topic", msg::standard::UInt32().hash())
                     .create_server()
                     .register_service("other_service",
                                       msg::standard::UInt32().hash(),
                                       msg::standard::Time().hash())
                     .request_service_client("other_service",
                                             msg::standard::UInt32().hash(),
                                             msg::standard::Time().hash())
                     .deregister_publisher("test_topic", msg::standard::UInt32().hash())
                     .deregister_subscriber("test_topic", msg::standard::UInt32().hash())
                     .deregister_service("test_service",
                                         msg::standard::UInt32().hash(),
                                         msg::standard::Time().hash())
                     .deregister_publisher("other_topic", msg::standard::UInt32().hash())
                     .deregister_subscriber("other_topic", msg::standard::UInt32().hash())
                     .deregister_service("other_service",
                                         msg::standard::UInt32().hash(),
                                         msg::standard::Time().hash())
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    auto pub = node->create_publisher<msg::standard::UInt32>("test_topic",
                                                             Endpoint("127.0.0.1", 0));
    EXPECT_NE(pub, nullptr);
    EXPECT_TRUE(pub->ok());

    auto sub = node->create_subscriber(
        "test_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));
    EXPECT_NE(sub, nullptr);
    EXPECT_TRUE(sub->ok());

    auto srv = node->create_service(
        "test_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));
    EXPECT_NE(srv, nullptr);
    EXPECT_TRUE(srv->ok());

    auto srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "test_service");
    EXPECT_NE(srv_cli, nullptr);
    EXPECT_TRUE(srv_cli->ok());

    auto other_pub = node->create_publisher<msg::standard::UInt32>(
        "other_topic", Endpoint("127.0.0.1", 0));
    EXPECT_NE(other_pub, nullptr);
    EXPECT_TRUE(other_pub->ok());

    auto other_sub = node->create_subscriber(
        "other_topic", [](const msg::standard::UInt32&) {}, Endpoint("127.0.0.1", 0));
    EXPECT_NE(other_sub, nullptr);
    EXPECT_TRUE(other_sub->ok());

    auto other_srv = node->create_service(
        "other_service",
        [](const msg::standard::UInt32&, msg::standard::Time&) {},
        Endpoint("127.0.0.1", 0));
    EXPECT_NE(other_srv, nullptr);
    EXPECT_TRUE(other_srv->ok());

    auto other_srv_cli =
        node->create_service_client<msg::standard::UInt32, msg::standard::Time>(
            "other_service");
    EXPECT_NE(other_srv_cli, nullptr);
    EXPECT_TRUE(other_srv_cli->ok());

    pub->shutdown();
    EXPECT_FALSE(pub->ok());
    pub = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    sub->shutdown();
    EXPECT_FALSE(sub->ok());
    sub = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    srv->shutdown();
    EXPECT_FALSE(srv->ok());
    srv = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    srv_cli->shutdown();
    EXPECT_FALSE(srv_cli->ok());
    srv_cli = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    other_pub->shutdown();
    EXPECT_FALSE(other_pub->ok());
    other_pub = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    other_sub->shutdown();
    EXPECT_FALSE(other_sub->ok());
    other_sub = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    other_srv->shutdown();
    EXPECT_FALSE(other_srv->ok());
    other_srv = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());

    other_srv_cli->shutdown();
    EXPECT_FALSE(other_srv_cli->ok());
    other_srv_cli = nullptr;
    node->spin_once();
    EXPECT_TRUE(node->ok());
  }
}

TEST(NodeRegistrationV2, ParameterSetRequest) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_parameter_set("test_param", param)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    bool status = node->set_parameter("test_param", *param);
    EXPECT_TRUE(status);
  }
}

TEST(NodeRegistrationV2, ParameterSetRequestFailure) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_parameter_set("test_param", param, true)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    bool status = node->set_parameter("test_param", *param);
    EXPECT_FALSE(status);
  }
}

TEST(NodeRegistrationV2, ParameterGetRequest) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_parameter_get("test_param", param)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    msg::standard::String param_received;
    bool status = node->get_parameter("test_param", param_received);
    EXPECT_TRUE(status);
    EXPECT_EQ(param_received.data, param->data);
  }
}

TEST(NodeRegistrationV2, ParameterGetRequestFailure) {
  auto param = std::make_shared<msg::standard::String>();
  param->data = "test_value";

  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_parameter_get("test_param", param, true)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    msg::standard::String param_received;
    bool status = node->get_parameter("test_param", param_received);
    EXPECT_FALSE(status);
  }
}

TEST(NodeRegistrationV2, SystemInfoGetRequest) {
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

  auto fixture =
      NodeTestFixture().register_node().request_system_info(sys_info).deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    msg::mediator::SystemInfo sys_info_received;
    bool status = node->get_system_info(sys_info_received);
    EXPECT_TRUE(status);
    EXPECT_EQ(sys_info_received.nodes.size(), sys_info.nodes.size());
    EXPECT_EQ(sys_info_received.nodes[0].name, sys_info.nodes[0].name);
    EXPECT_EQ(sys_info_received.nodes[0].id, sys_info.nodes[0].id);
    EXPECT_EQ(sys_info_received.publishers.size(), sys_info.publishers.size());
    EXPECT_EQ(sys_info_received.publishers[0].topic_info.name,
              sys_info.publishers[0].topic_info.name);
    EXPECT_EQ(sys_info_received.publishers[0].topic_info.message_hash,
              sys_info.publishers[0].topic_info.message_hash);
    EXPECT_EQ(sys_info_received.publishers[0].endpoint.address,
              sys_info.publishers[0].endpoint.address);
    EXPECT_EQ(sys_info_received.publishers[0].endpoint.port,
              sys_info.publishers[0].endpoint.port);
    EXPECT_EQ(sys_info_received.subscribers.size(), sys_info.subscribers.size());
    EXPECT_EQ(sys_info_received.subscribers[0].topic_info.name,
              sys_info.subscribers[0].topic_info.name);
    EXPECT_EQ(sys_info_received.subscribers[0].topic_info.message_hash,
              sys_info.subscribers[0].topic_info.message_hash);
    EXPECT_EQ(sys_info_received.subscribers[0].endpoint.address,
              sys_info.subscribers[0].endpoint.address);
    EXPECT_EQ(sys_info_received.subscribers[0].endpoint.port,
              sys_info.subscribers[0].endpoint.port);
    EXPECT_EQ(sys_info_received.services.size(), sys_info.services.size());
    EXPECT_EQ(sys_info_received.services[0].name, sys_info.services[0].name);
    EXPECT_EQ(sys_info_received.services[0].request_hash,
              sys_info.services[0].request_hash);
    EXPECT_EQ(sys_info_received.services[0].response_hash,
              sys_info.services[0].response_hash);
    EXPECT_EQ(sys_info_received.services[0].endpoint.address,
              sys_info.services[0].endpoint.address);
    EXPECT_EQ(sys_info_received.services[0].endpoint.port,
              sys_info.services[0].endpoint.port);
    EXPECT_EQ(sys_info_received.topics.size(), sys_info.topics.size());
    EXPECT_EQ(sys_info_received.topics[0].name, sys_info.topics[0].name);
    EXPECT_EQ(sys_info_received.topics[0].message_hash, sys_info.topics[0].message_hash);
  }
}

TEST(NodeRegistrationV2, SystemInfoGetRequestFailure) {
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

  auto fixture = NodeTestFixture()
                     .register_node()
                     .request_system_info(sys_info, true)
                     .deregister_node();

  {
    auto node = fixture.build();
    EXPECT_TRUE(node->ok());

    msg::mediator::SystemInfo sys_info_received;
    bool status = node->get_system_info(sys_info_received);
    // Status will still be true on failure
    EXPECT_TRUE(status);
    // Response will be empty on failure
    EXPECT_EQ(sys_info_received.nodes.size(), 0);
    EXPECT_EQ(sys_info_received.publishers.size(), 0);
    EXPECT_EQ(sys_info_received.subscribers.size(), 0);
    EXPECT_EQ(sys_info_received.services.size(), 0);
    EXPECT_EQ(sys_info_received.topics.size(), 0);
  }
}