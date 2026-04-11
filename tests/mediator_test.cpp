#include <gtest/gtest.h>

#include "rix/std_msgs/String.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/test/mediator_test_fixture.hpp"

using namespace rix;

TEST(MediatorTest, Ping) {
  // Clear, self-documenting test

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 1)
                     .ping();
  const auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process ping
}

TEST(MediatorTest, RegisterAndDeregisterNode) {
  // Clear, self-documenting test
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 2)
                     .register_node("test_node", 1234)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process registration
  EXPECT_EQ(med->get_node_count(), 1);
  med->spin_once(); // Process deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, RegisterNodeFailureDuplicateID) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 2)
                     .register_node("test_node", 1234, false)
                     .register_node("other_node", 1234, true);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once();                    // Process duplicate registration
  EXPECT_EQ(med->get_node_count(), 1); // Still only one node
}

TEST(MediatorTest, DeregisterUnregisteredNode) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 3)
                     .register_node("test_node", 1234, false)
                     .register_node("other_node", 4321, false)
                     .deregister_node("unknown_node", 9999);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process second registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once();                    // Process deregistration of unknown node
  EXPECT_EQ(med->get_node_count(), 2); // Still two nodes
}

TEST(MediatorTest, RegisterAndDeregisterPublisher) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
                     .register_node("test_node", 1234)
                     .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_publisher_count(), 0);
}

TEST(MediatorTest, RegisterPublisherFailureDuplicateID) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), true)
                     .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once();                         // Process duplicate publisher registration
  EXPECT_EQ(med->get_node_count(), 1);      // Still one node
  EXPECT_EQ(med->get_publisher_count(), 1); // Still one publisher

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_publisher_count(), 0);
}

TEST(MediatorTest, RegisterPublisherFailureInvalidMessageHash) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .register_publisher(8765, 1234, "test_topic", std_msgs::Time().hash(), true)
                     .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once();                         // Process invalid message hash registration
  EXPECT_EQ(med->get_node_count(), 1);      // Still one node
  EXPECT_EQ(med->get_publisher_count(), 1); // Still one publisher

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_publisher_count(), 0);
}

TEST(MediatorTest, RegisterPublisherFailureInvalidNodeID) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .register_publisher(8765, 4321, "test_topic", std_msgs::UInt32().hash(), true)
                     .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once();                         // Process invalid node ID publisher registration
  EXPECT_EQ(med->get_node_count(), 1);      // Still one node
  EXPECT_EQ(med->get_publisher_count(), 1); // Still one publisher

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_publisher_count(), 0);
}

TEST(MediatorTest, DeregisterUnregisteredPublisher) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_publisher(9999, 1234, "other_topic", std_msgs::UInt32().hash())
                     .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once();                         // Process deregistration of unknown publisher
  EXPECT_EQ(med->get_node_count(), 1);      // Still one node
  EXPECT_EQ(med->get_publisher_count(), 1); // Still one publisher

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_publisher_count(), 0);
}

TEST(MediatorTest, RegisterAndDeregisterSubscriber) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
                     .register_node("test_node", 1234)
                     .register_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, RegisterSubscriberFailureDuplicateID) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .register_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash(), true)
                     .deregister_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once();                          // Process duplicate subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);       // Still one node
  EXPECT_EQ(med->get_subscriber_count(), 1); // Still one subscriber

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, RegisterSubscriberFailureInvalidMessageHash) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .register_subscriber(8765, 1234, "test_topic", std_msgs::Time().hash(), true)
                     .deregister_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once();                          // Process invalid message hash registration
  EXPECT_EQ(med->get_node_count(), 1);       // Still one node
  EXPECT_EQ(med->get_subscriber_count(), 1); // Still one subscriber

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, RegisterSubscriberFailureInvalidNodeID) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .register_subscriber(8765, 4321, "test_topic", std_msgs::UInt32().hash(), true)
                     .deregister_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once();                          // Process invalid node ID subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);       // Still one node
  EXPECT_EQ(med->get_subscriber_count(), 1); // Still one subscriber

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, NotifySubscribersFromSubRegister) {
  sys_msgs::SubNotify notify;
  notify.id = 5678;
  notify.publishers.resize(3);
  notify.publishers[0].id = 123;
  notify.publishers[0].node_id = 1234;
  notify.publishers[0].topic_info.name = "test_topic";
  notify.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify.publishers[0].endpoint.address = "127.0.0.1";
  notify.publishers[0].endpoint.port = 123;

  notify.publishers[1].id = 234;
  notify.publishers[1].node_id = 1234;
  notify.publishers[1].topic_info.name = "test_topic";
  notify.publishers[1].topic_info.message_hash = std_msgs::UInt32().hash();
  notify.publishers[1].endpoint.address = "127.0.0.1";
  notify.publishers[1].endpoint.port = 234;

  notify.publishers[2].id = 456;
  notify.publishers[2].node_id = 1234;
  notify.publishers[2].topic_info.name = "test_topic";
  notify.publishers[2].topic_info.message_hash = std_msgs::UInt32().hash();
  notify.publishers[2].endpoint.address = "127.0.0.1";
  notify.publishers[2].endpoint.port = 456;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 8)
          .register_node("test_node", 1234)
          .register_publisher(123, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 123))
          .register_publisher(234, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 234))
          .register_publisher(456, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 456))
          .register_node("other_node", 4321)
          .register_subscriber(5678, 4321, "test_topic", std_msgs::UInt32().hash())
          .notify_subscriber(5678, Endpoint("127.0.0.1", 8001), "test_topic", std_msgs::UInt32().hash(), notify)
          .deregister_subscriber(5678, 4321, "test_topic", std_msgs::UInt32().hash())
          .deregister_node("other_node", 4321);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process publisher registration 1
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process publisher registration 2
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 2);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process publisher registration 3
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process second node registration
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, NotifySubscribersFromPubRegister) {
  sys_msgs::SubNotify notify1;
  notify1.id = 5678;
  notify1.publishers.resize(1);
  notify1.publishers[0].id = 123;
  notify1.publishers[0].node_id = 1234;
  notify1.publishers[0].topic_info.name = "test_topic";
  notify1.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify1.publishers[0].endpoint.address = "127.0.0.1";
  notify1.publishers[0].endpoint.port = 123;

  sys_msgs::SubNotify notify2;
  notify2.id = 5678;
  notify2.publishers.resize(1);
  notify2.publishers[0].id = 234;
  notify2.publishers[0].node_id = 1234;
  notify2.publishers[0].topic_info.name = "test_topic";
  notify2.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify2.publishers[0].endpoint.address = "127.0.0.1";
  notify2.publishers[0].endpoint.port = 234;

  sys_msgs::SubNotify notify3;
  notify3.id = 5678;
  notify3.publishers.resize(1);
  notify3.publishers[0].id = 456;
  notify3.publishers[0].node_id = 1234;
  notify3.publishers[0].topic_info.name = "test_topic";
  notify3.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify3.publishers[0].endpoint.address = "127.0.0.1";
  notify3.publishers[0].endpoint.port = 456;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 8)
          .register_node("other_node", 4321)
          .register_subscriber(5678, 4321, "test_topic", std_msgs::UInt32().hash())
          .register_node("test_node", 1234)
          .register_publisher(123, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 123))
          .notify_subscriber(5678, Endpoint("127.0.0.1", 8001), "test_topic", std_msgs::UInt32().hash(), notify1)
          .register_publisher(234, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 234))
          .notify_subscriber(5678, Endpoint("127.0.0.1", 8001), "test_topic", std_msgs::UInt32().hash(), notify2)
          .register_publisher(456, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 456))
          .notify_subscriber(5678, Endpoint("127.0.0.1", 8001), "test_topic", std_msgs::UInt32().hash(), notify3)
          .deregister_subscriber(5678, 4321, "test_topic", std_msgs::UInt32().hash())
          .deregister_node("other_node", 4321);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process second node registration
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process publisher registration 1
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process publisher registration 2
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 2);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process publisher registration 3
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 3);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, DeregisterUnregisteredSubscriber) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
                     .register_node("test_node", 1234)
                     .register_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_subscriber(9999, 1234, "other_topic", std_msgs::UInt32().hash())
                     .deregister_subscriber(5678, 1234, "test_topic", std_msgs::UInt32().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once();                          // Process deregistration of unknown subscriber
  EXPECT_EQ(med->get_node_count(), 1);       // Still one node
  EXPECT_EQ(med->get_subscriber_count(), 1); // Still one subscriber

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_subscriber_count(), 0);
}

TEST(MediatorTest, RegisterAndDeregisterService) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
                     .register_node("test_node", 1234)
                     .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
                     .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_service_count(), 0);
}

TEST(MediatorTest, RegisterServiceFailureDuplicateID) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash(), true)
          .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once();                       // Process duplicate service registration
  EXPECT_EQ(med->get_node_count(), 1);    // Still one node
  EXPECT_EQ(med->get_service_count(), 1); // Still one service

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_service_count(), 0);
}

TEST(MediatorTest, RegisterServiceFailureInvalidRequestHash) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .register_service(8765, 1234, "test_service", std_msgs::String().hash(), std_msgs::Time().hash(), true)
          .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once();                       // Process invalid request hash registration
  EXPECT_EQ(med->get_node_count(), 1);    // Still one node
  EXPECT_EQ(med->get_service_count(), 1); // Still one service

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_service_count(), 0);
}

TEST(MediatorTest, RegisterServiceFailureInvalidResponseHash) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .register_service(8765, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::String().hash(), true)
          .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once();                       // Process invalid response hash registration
  EXPECT_EQ(med->get_node_count(), 1);    // Still one node
  EXPECT_EQ(med->get_service_count(), 1); // Still one service

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_service_count(), 0);
}

TEST(MediatorTest, RegisterServiceFailureInvalidNodeID) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .register_service(8765, 4321, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash(), true)
          .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once();                       // Process invalid node ID service registration
  EXPECT_EQ(med->get_node_count(), 1);    // Still one node
  EXPECT_EQ(med->get_service_count(), 1); // Still one service

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_service_count(), 0);
}

TEST(MediatorTest, DeregisterUnregisteredService) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_service(9999, 1234, "other_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once();                       // Process deregistration of unknown service
  EXPECT_EQ(med->get_node_count(), 1);    // Still one node
  EXPECT_EQ(med->get_service_count(), 1); // Still one service

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_service_count(), 0);
}

TEST(MediatorTest, RequestServiceClient) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .request_service_client(1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash(), 5678)
          .deregister_service(5678, 1234, "test_service", std_msgs::UInt32().hash(), std_msgs::Time().hash())
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once(); // Process service client request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process service deregistration
  EXPECT_EQ(med->get_service_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, RequestServiceClientFailure) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 3)
                     .register_node("test_node", 1234)
                     .request_service_client(
                         1234, "nonexistent_service", std_msgs::UInt32().hash(), std_msgs::Time().hash(), 0, true)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process service client request for nonexistent service
  EXPECT_TRUE(med->ok());
  EXPECT_EQ(med->get_service_count(), 0); // No service should be registered

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, RegisterAndDeregisterAction) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
          .register_node("test_node", 1234)
          .register_action(
              5678, 1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash())
          .deregister_action(5678, 1234, "test_action")
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process action registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 1);

  med->spin_once(); // Process action deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_action_count(), 0);
}

TEST(MediatorTest, RegisterActionFailureDuplicateID) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_action(
              5678, 1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash())
          .register_action(5678,
                           1234,
                           "test_action",
                           std_msgs::UInt32().hash(),
                           std_msgs::String().hash(),
                           std_msgs::Time().hash(),
                           true)
          .deregister_action(5678, 1234, "test_action")
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process action registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 1);

  med->spin_once();                      // Process duplicate action registration
  EXPECT_EQ(med->get_node_count(), 1);   // Still one node
  EXPECT_EQ(med->get_action_count(), 1); // Still one action

  med->spin_once(); // Process action deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_action_count(), 0);
}

TEST(MediatorTest, RegisterActionFailureDuplicateName) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_action(
              5678, 1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash())
          .register_action(8765,
                           1234,
                           "test_action",
                           std_msgs::UInt32().hash(),
                           std_msgs::String().hash(),
                           std_msgs::Time().hash(),
                           true)
          .deregister_action(5678, 1234, "test_action")
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process action registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 1);

  med->spin_once();                      // Process duplicate name action registration
  EXPECT_EQ(med->get_node_count(), 1);   // Still one node
  EXPECT_EQ(med->get_action_count(), 1); // Still one action

  med->spin_once(); // Process action deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_action_count(), 0);
}

TEST(MediatorTest, RegisterActionFailureInvalidNodeID) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_action(
              5678, 1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash())
          .register_action(8765,
                           4321,
                           "other_action",
                           std_msgs::UInt32().hash(),
                           std_msgs::String().hash(),
                           std_msgs::Time().hash(),
                           true)
          .deregister_action(5678, 1234, "test_action")
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process action registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 1);

  med->spin_once();                      // Process invalid node ID action registration
  EXPECT_EQ(med->get_node_count(), 1);   // Still one node
  EXPECT_EQ(med->get_action_count(), 1); // Still one action

  med->spin_once(); // Process action deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_action_count(), 0);
}

TEST(MediatorTest, DeregisterUnregisteredAction) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_action(
              5678, 1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash())
          .deregister_action(9999, 1234, "other_action")
          .deregister_action(5678, 1234, "test_action")
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process action registration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 1);

  med->spin_once();                      // Process deregistration of unknown action
  EXPECT_EQ(med->get_node_count(), 1);   // Still one node
  EXPECT_EQ(med->get_action_count(), 1); // Still one action

  med->spin_once(); // Process action deregistration
  EXPECT_EQ(med->get_node_count(), 1);
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
  EXPECT_EQ(med->get_action_count(), 0);
}

TEST(MediatorTest, RequestActionClient) {
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 5)
          .register_node("test_node", 1234)
          .register_action(
              5678, 1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash())
          .request_action_client(
              1234, "test_action", std_msgs::UInt32().hash(), std_msgs::String().hash(), std_msgs::Time().hash(), 5678)
          .deregister_action(5678, 1234, "test_action")
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process action registration
  EXPECT_EQ(med->get_action_count(), 1);

  med->spin_once(); // Process action client request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process action deregistration
  EXPECT_EQ(med->get_action_count(), 0);

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, RequestActionClientFailure) {
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 3)
                     .register_node("test_node", 1234)
                     .request_action_client(1234,
                                            "nonexistent_action",
                                            std_msgs::UInt32().hash(),
                                            std_msgs::String().hash(),
                                            std_msgs::Time().hash(),
                                            0,
                                            true)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process action client request for nonexistent action
  EXPECT_TRUE(med->ok());
  EXPECT_EQ(med->get_action_count(), 0); // No action should be registered

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, ParameterSetRequest) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 3)
                     .register_node("test_node", 1234)
                     .request_parameter_set(1234, "test_param", param)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process parameter set request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, ParameterSetRequestFailureInvalidMessageHash) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  auto wrong_param = std::make_shared<std_msgs::Time>();
  wrong_param->sec = 123456789;
  wrong_param->nsec = 987654321;

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
                     .register_node("test_node", 1234)
                     .request_parameter_set(1234, "test_param", param)
                     .request_parameter_set(1234, "test_param", wrong_param, true)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process parameter set request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process invalid parameter set request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, ParameterSetRequestFailureInvalidNodeID) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  auto wrong_param = std::make_shared<std_msgs::Time>();
  wrong_param->sec = 123456789;
  wrong_param->nsec = 987654321;

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 1)
                     .request_parameter_set(1234, "test_param", param, true);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process parameter set request with invalid node ID
  EXPECT_TRUE(med->ok());
}

TEST(MediatorTest, ParameterGetRequest) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
                     .register_node("test_node", 1234)
                     .request_parameter_set(1234, "test_param", param)
                     .request_parameter_get(1234, "test_param", param)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process parameter set request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process parameter get request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, ParameterGetRequestFailureNonexistent) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 3)
                     .register_node("test_node", 1234)
                     .request_parameter_get(1234, "test_param", param, true)
                     .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process parameter get request with failure
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, ParameterGetRequestFailureInvalidNodeID) {
  auto param = std::make_shared<std_msgs::String>();
  param->data = "test_value";

  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 1)
                     .request_parameter_get(1234, "test_param", param, true);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process parameter get request with failure
  EXPECT_TRUE(med->ok());
}

TEST(MediatorTest, SystemInfoGetRequest) {
  sys_msgs::SystemInfo sys_info;
  sys_info.nodes.resize(1);
  sys_info.nodes[0].name = "test_node";
  sys_info.nodes[0].id = 1234;
  sys_info.publishers.resize(1);
  sys_info.publishers[0].id = 5678;
  sys_info.publishers[0].node_id = 1234;
  sys_info.publishers[0].topic_info.name = "test_topic";
  sys_info.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  sys_info.publishers[0].endpoint.address = "127.0.0.1";
  sys_info.publishers[0].endpoint.port = 1234;
  sys_info.subscribers.resize(1);
  sys_info.subscribers[0].id = 9012;
  sys_info.subscribers[0].node_id = 1234;
  sys_info.subscribers[0].topic_info.name = "other_topic";
  sys_info.subscribers[0].topic_info.message_hash = std_msgs::Time().hash();
  sys_info.subscribers[0].endpoint.address = "127.0.0.1";
  sys_info.subscribers[0].endpoint.port = 5678;
  sys_info.services.resize(1);
  sys_info.services[0].id = 3456;
  sys_info.services[0].node_id = 1234;
  sys_info.services[0].name = "test_service";
  sys_info.services[0].request_hash = std_msgs::UInt32().hash();
  sys_info.services[0].response_hash = std_msgs::Time().hash();
  sys_info.services[0].endpoint.address = "127.0.0.1";
  sys_info.services[0].endpoint.port = 9012;
  sys_info.topics.resize(2);
  // Topics are listed in alphabetical order
  sys_info.topics[0].name = "other_topic";
  sys_info.topics[0].message_hash = std_msgs::Time().hash();
  sys_info.topics[1].name = "test_topic";
  sys_info.topics[1].message_hash = std_msgs::UInt32().hash();

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 6)
          .register_node("test_node", 1234)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 1234))
          .register_subscriber(9012, 1234, "other_topic", std_msgs::Time().hash(), false, Endpoint("127.0.0.1", 5678))
          .register_service(3456,
                            1234,
                            "test_service",
                            std_msgs::UInt32().hash(),
                            std_msgs::Time().hash(),
                            false,
                            Endpoint("127.0.0.1", 9012))
          .request_system_info(1234, sys_info)
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once(); // Process system info request
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, SystemInfoGetRequestFailureInvalidNodeID) {
  sys_msgs::SystemInfo sys_info;
  sys_info.nodes.resize(1);
  sys_info.nodes[0].name = "test_node";
  sys_info.nodes[0].id = 1234;
  sys_info.publishers.resize(1);
  sys_info.publishers[0].id = 5678;
  sys_info.publishers[0].node_id = 1234;
  sys_info.publishers[0].topic_info.name = "test_topic";
  sys_info.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  sys_info.publishers[0].endpoint.address = "127.0.0.1";
  sys_info.publishers[0].endpoint.port = 1234;
  sys_info.subscribers.resize(1);
  sys_info.subscribers[0].id = 9012;
  sys_info.subscribers[0].node_id = 1234;
  sys_info.subscribers[0].topic_info.name = "other_topic";
  sys_info.subscribers[0].topic_info.message_hash = std_msgs::Time().hash();
  sys_info.subscribers[0].endpoint.address = "127.0.0.1";
  sys_info.subscribers[0].endpoint.port = 5678;
  sys_info.services.resize(1);
  sys_info.services[0].id = 3456;
  sys_info.services[0].node_id = 1234;
  sys_info.services[0].name = "test_service";
  sys_info.services[0].request_hash = std_msgs::UInt32().hash();
  sys_info.services[0].response_hash = std_msgs::Time().hash();
  sys_info.services[0].endpoint.address = "127.0.0.1";
  sys_info.services[0].endpoint.port = 9012;
  sys_info.topics.resize(2);
  sys_info.topics[0].name = "test_topic";
  sys_info.topics[0].message_hash = std_msgs::UInt32().hash();
  sys_info.topics[1].name = "other_topic";
  sys_info.topics[1].message_hash = std_msgs::Time().hash();

  sys_msgs::SystemInfo empty_sys_info;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 6)
          .register_node("test_node", 1234)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 1234))
          .register_subscriber(9012, 1234, "other_topic", std_msgs::Time().hash(), false, Endpoint("127.0.0.1", 5678))
          .register_service(3456,
                            1234,
                            "test_service",
                            std_msgs::UInt32().hash(),
                            std_msgs::Time().hash(),
                            false,
                            Endpoint("127.0.0.1", 9012))
          .request_system_info(4321, empty_sys_info) // Expect failure (empty info)
          .deregister_node("test_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process service registration
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once(); // Process system info request failure
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, SubscriberRegisteredAfterPublisherDeregistered) {
  // Test that subscriber does NOT receive notification for deregistered publisher
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 8)
          .register_node("pub_node", 1234)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
          .register_node("sub_node", 4321)
          .register_subscriber(9012, 4321, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8002))
          .deregister_subscriber(9012, 4321, "test_topic", std_msgs::UInt32().hash())
          .deregister_node("sub_node", 4321)
          .deregister_node("pub_node", 1234);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process pub_node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process publisher registration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process sub_node registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process subscriber registration (no pubs to notify about)
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process sub_node deregistration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process pub_node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, PublisherRegisteredAfterSubscriberReceivesNotification) {
  // Test that existing subscriber IS notified when new publisher registers
  sys_msgs::SubNotify notify;
  notify.id = 9012;
  notify.publishers.resize(1);
  notify.publishers[0].id = 5678;
  notify.publishers[0].node_id = 1234;
  notify.publishers[0].topic_info.name = "test_topic";
  notify.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify.publishers[0].endpoint.address = "127.0.0.1";
  notify.publishers[0].endpoint.port = 8001;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 8)
          .register_node("sub_node", 4321)
          .register_subscriber(9012, 4321, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8002))
          .register_node("pub_node", 1234)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .notify_subscriber(9012, Endpoint("127.0.0.1", 8002), "test_topic", std_msgs::UInt32().hash(), notify)
          .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
          .deregister_subscriber(9012, 4321, "test_topic", std_msgs::UInt32().hash())
          .deregister_node("pub_node", 1234)
          .deregister_node("sub_node", 4321);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process sub_node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process subscriber registration
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process pub_node registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process publisher registration (should notify subscriber)
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher deregistration
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process subscriber deregistration
  EXPECT_EQ(med->get_subscriber_count(), 0);

  med->spin_once(); // Process pub_node deregistration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process sub_node deregistration
  EXPECT_EQ(med->get_node_count(), 0);
}

TEST(MediatorTest, MultipleNodesOnSameTopic) {
  // Test multiple publishers and subscribers from different nodes on same topic
  sys_msgs::SubNotify notify1;
  notify1.id = 9012;
  notify1.publishers.resize(2);
  notify1.publishers[0].id = 5678;
  notify1.publishers[0].node_id = 1234;
  notify1.publishers[0].topic_info.name = "shared_topic";
  notify1.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify1.publishers[0].endpoint.address = "127.0.0.1";
  notify1.publishers[0].endpoint.port = 8001;
  notify1.publishers[1].id = 5679;
  notify1.publishers[1].node_id = 4321;
  notify1.publishers[1].topic_info.name = "shared_topic";
  notify1.publishers[1].topic_info.message_hash = std_msgs::UInt32().hash();
  notify1.publishers[1].endpoint.address = "127.0.0.1";
  notify1.publishers[1].endpoint.port = 8003;

  sys_msgs::SubNotify notify2;
  notify2.id = 9013;
  notify2.publishers.resize(2);
  notify2.publishers[0].id = 5678;
  notify2.publishers[0].node_id = 1234;
  notify2.publishers[0].topic_info.name = "shared_topic";
  notify2.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify2.publishers[0].endpoint.address = "127.0.0.1";
  notify2.publishers[0].endpoint.port = 8001;
  notify2.publishers[1].id = 5679;
  notify2.publishers[1].node_id = 4321;
  notify2.publishers[1].topic_info.name = "shared_topic";
  notify2.publishers[1].topic_info.message_hash = std_msgs::UInt32().hash();
  notify2.publishers[1].endpoint.address = "127.0.0.1";
  notify2.publishers[1].endpoint.port = 8003;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 7)
          .register_node("node1", 1234)
          .register_node("node2", 4321)
          .register_node("node3", 5555)
          .register_publisher(5678, 1234, "shared_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .register_publisher(5679, 4321, "shared_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8003))
          .register_subscriber(
              9012, 5555, "shared_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8002))
          .notify_subscriber(9012, Endpoint("127.0.0.1", 8002), "shared_topic", std_msgs::UInt32().hash(), notify1)
          .register_subscriber(
              9013, 5555, "shared_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8004))
          .notify_subscriber(9013, Endpoint("127.0.0.1", 8004), "shared_topic", std_msgs::UInt32().hash(), notify2);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node1 registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process node2 registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process node3 registration
  EXPECT_EQ(med->get_node_count(), 3);

  med->spin_once(); // Process publisher 1 registration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher 2 registration
  EXPECT_EQ(med->get_publisher_count(), 2);

  med->spin_once(); // Process subscriber 1 registration (notified of 2 pubs)
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process subscriber 2 registration (notified of 2 pubs)
  EXPECT_EQ(med->get_subscriber_count(), 2);
}

TEST(MediatorTest, MultipleNodesOnDifferentTopics) {
  // Test multiple publishers and subscribers from different nodes on different topics
  sys_msgs::SubNotify notify1;
  notify1.id = 9012;
  notify1.publishers.resize(1);
  notify1.publishers[0].id = 5679;
  notify1.publishers[0].node_id = 4321;
  notify1.publishers[0].topic_info.name = "topic_b";
  notify1.publishers[0].topic_info.message_hash = std_msgs::String().hash();
  notify1.publishers[0].endpoint.address = "127.0.0.1";
  notify1.publishers[0].endpoint.port = 8003;

  sys_msgs::SubNotify notify2;
  notify2.id = 9013;
  notify2.publishers.resize(1);
  notify2.publishers[0].id = 5678;
  notify2.publishers[0].node_id = 1234;
  notify2.publishers[0].topic_info.name = "topic_a";
  notify2.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify2.publishers[0].endpoint.address = "127.0.0.1";
  notify2.publishers[0].endpoint.port = 8001;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 6)
          .register_node("node1", 1234)
          .register_node("node2", 4321)
          .register_publisher(5678, 1234, "topic_a", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .register_publisher(5679, 4321, "topic_b", std_msgs::String().hash(), false, Endpoint("127.0.0.1", 8003))
          .register_subscriber(9012, 1234, "topic_b", std_msgs::String().hash(), false, Endpoint("127.0.0.1", 8002))
          .notify_subscriber(9012, Endpoint("127.0.0.1", 8002), "topic_b", std_msgs::String().hash(), notify1)
          .register_subscriber(9013, 4321, "topic_a", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8004))
          .notify_subscriber(9013, Endpoint("127.0.0.1", 8004), "topic_a", std_msgs::UInt32().hash(), notify2);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node1 registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process node2 registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process publisher on topic_a
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher on topic_b
  EXPECT_EQ(med->get_publisher_count(), 2);

  med->spin_once(); // Process subscriber on topic_b (matches pub on topic_b)
  EXPECT_EQ(med->get_subscriber_count(), 1);

  med->spin_once(); // Process subscriber on topic_a (matches pub on topic_a)
  EXPECT_EQ(med->get_subscriber_count(), 2);

  // Verify counts remain stable
  EXPECT_EQ(med->get_node_count(), 2);
  EXPECT_EQ(med->get_publisher_count(), 2);
  EXPECT_EQ(med->get_subscriber_count(), 2);
}

TEST(MediatorTest, SameTopicDifferentMessageHashesIsolated) {
  // Test that same topic name with different message hashes are rejected
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
          .register_node("node1", 1234)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .register_publisher(
              5679, 1234, "test_topic", std_msgs::String().hash(), true, Endpoint("127.0.0.1", 8002)) // Should fail
          .register_subscriber(
              9012, 1234, "test_topic", std_msgs::Time().hash(), true, Endpoint("127.0.0.1", 8003)); // Should fail

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process first publisher (UInt32 hash)
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once();                         // Process second publisher (String hash) - should fail
  EXPECT_EQ(med->get_publisher_count(), 1); // Still only 1

  med->spin_once();                          // Process subscriber (Time hash) - should fail
  EXPECT_EQ(med->get_subscriber_count(), 0); // No subscribers registered
}

TEST(MediatorTest, MultiplePublishersDeregisteredBeforeSubscriber) {
  // Test that subscriber gets NO notifications when all pubs are deregistered first
  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 8)
          .register_node("pub_node1", 1234)
          .register_node("pub_node2", 4321)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .register_publisher(5679, 4321, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8003))
          .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
          .deregister_publisher(5679, 4321, "test_topic", std_msgs::UInt32().hash())
          .register_node("sub_node", 5555)
          .register_subscriber(9012, 5555, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8002));

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process pub_node1 registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process pub_node2 registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process publisher 1 registration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher 2 registration
  EXPECT_EQ(med->get_publisher_count(), 2);

  med->spin_once(); // Process publisher 1 deregistration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher 2 deregistration
  EXPECT_EQ(med->get_publisher_count(), 0);

  med->spin_once(); // Process sub_node registration
  EXPECT_EQ(med->get_node_count(), 3);

  med->spin_once(); // Process subscriber registration (no pubs available)
  EXPECT_EQ(med->get_subscriber_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 0); // No publishers
}

TEST(MediatorTest, PartialPublisherDeregistrationBeforeSubscriber) {
  // Test subscriber notified only of remaining publishers
  sys_msgs::SubNotify notify;
  notify.id = 9012;
  notify.publishers.resize(1);
  notify.publishers[0].id = 5679;
  notify.publishers[0].node_id = 4321;
  notify.publishers[0].topic_info.name = "test_topic";
  notify.publishers[0].topic_info.message_hash = std_msgs::UInt32().hash();
  notify.publishers[0].endpoint.address = "127.0.0.1";
  notify.publishers[0].endpoint.port = 8003;

  auto fixture =
      MediatorTestFixture(Endpoint("127.0.0.1", 0))
          .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 7)
          .register_node("pub_node1", 1234)
          .register_node("pub_node2", 4321)
          .register_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8001))
          .register_publisher(5679, 4321, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8003))
          .deregister_publisher(5678, 1234, "test_topic", std_msgs::UInt32().hash())
          .register_node("sub_node", 5555)
          .register_subscriber(9012, 5555, "test_topic", std_msgs::UInt32().hash(), false, Endpoint("127.0.0.1", 8002))
          .notify_subscriber(9012, Endpoint("127.0.0.1", 8002), "test_topic", std_msgs::UInt32().hash(), notify);

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process pub_node1 registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process pub_node2 registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process publisher 1 registration
  EXPECT_EQ(med->get_publisher_count(), 1);

  med->spin_once(); // Process publisher 2 registration
  EXPECT_EQ(med->get_publisher_count(), 2);

  med->spin_once();                         // Process publisher 1 deregistration
  EXPECT_EQ(med->get_publisher_count(), 1); // Only pub 2 remains

  med->spin_once(); // Process sub_node registration
  EXPECT_EQ(med->get_node_count(), 3);

  med->spin_once(); // Process subscriber registration (notified only of pub 2)
  EXPECT_EQ(med->get_subscriber_count(), 1);
  EXPECT_EQ(med->get_publisher_count(), 1); // Still 1 publisher
}

TEST(MediatorTest, CrossNodeServiceIsolation) {
  // Test that services with same name but different hashes are rejected
  auto fixture = MediatorTestFixture(Endpoint("127.0.0.1", 0))
                     .create_server(Endpoint("127.0.0.1", 0), Endpoint("127.0.0.1", 48104), 4)
                     .register_node("node1", 1234)
                     .register_node("node2", 4321)
                     .register_service(5678,
                                       1234,
                                       "test_service",
                                       std_msgs::UInt32().hash(),
                                       std_msgs::Time().hash(),
                                       false,
                                       Endpoint("127.0.0.1", 8001))
                     .register_service(5679,
                                       4321,
                                       "test_service",
                                       std_msgs::String().hash(),
                                       std_msgs::Time().hash(),
                                       true,
                                       Endpoint("127.0.0.1", 8002)); // Should fail - name exists

  auto med = fixture.build();
  EXPECT_TRUE(med->ok());

  med->spin_once(); // Process node1 registration
  EXPECT_EQ(med->get_node_count(), 1);

  med->spin_once(); // Process node2 registration
  EXPECT_EQ(med->get_node_count(), 2);

  med->spin_once(); // Process service 1 registration
  EXPECT_EQ(med->get_service_count(), 1);

  med->spin_once();                       // Process service 2 registration - should fail
  EXPECT_EQ(med->get_service_count(), 1); // Still only 1
}