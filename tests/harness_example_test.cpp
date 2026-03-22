/**
 * @file simple_publisher_harness_test.cpp
 * @brief Example tests using Fixture 2 (NodeTestHarness from node_test_harness.hpp).
 *
 * Demonstrates the educator-facing API for testing custom RIX Nodes.
 * Compare with examples/simple_publisher/test.cpp to see the dramatic
 * reduction in boilerplate — no NodeInfo, PubInfo, OPCODE, or mock socket
 * management needed.
 */

// Include the harness and the node under test
#include "rix/std_msgs/Header.hpp"
#include "rix/std_msgs/String.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/test/node_test_harness.hpp"
#include <gtest/gtest.h>

/**
 * A minimal publisher node used for demonstration.
 * Publishes a Header message on /chatter every 1.0s.
 */
class DemoPublisher final : public rix::Node {
public:
  DemoPublisher(double rate) : rix::Node("demo_publisher") {
    if (!ok())
      return;
    pub_ = create_publisher<rix::std_msgs::Header>("/chatter");
    if (!pub_ || !pub_->ok()) {
      shutdown();
      return;
    }
    msg_.frame_id = "Hello!";
    msg_.seq = 0;
    timer_ = this->create_timer(rix::Duration(1.0 / rate), &DemoPublisher::on_timer, this);
  }

private:
  void on_timer(const rix::TimerCallback::Event&) {
    msg_.seq += 1;
    msg_.stamp = rix::Time::now().to_msg();
    pub_->publish(msg_);
  }
  std::shared_ptr<rix::Publisher> pub_;
  std::shared_ptr<rix::TimerCallback> timer_;
  rix::std_msgs::Header msg_;
};

/**
 * A minimal service node for demonstration.
 * Converts UInt32 → String (alphabet mapping).
 */
class DemoService final : public rix::Node {
public:
  DemoService() : rix::Node("demo_service") {
    if (!ok())
      return;
    auto srv = create_service("/alphabet", &DemoService::callback, this);
    if (!srv || !srv->ok()) {
      shutdown();
      return;
    }
  }

private:
  void callback(const rix::std_msgs::UInt32& req, rix::std_msgs::String& res) {
    res.data = std::string(1, 'a' + (req.data % 26));
  }
};

using namespace rix::test;

// =============================================================================
// Publisher Tests — The "3 lines of setup" pattern
// =============================================================================

TEST(HarnessTest, PublisherNodeCreatesSuccessfully) {
  // 1. Declare topology
  NodeTestHarness harness;
  harness.expect_publisher<rix::std_msgs::Header>("/chatter", 0);

  // 2. Create node
  auto& node = harness.create<DemoPublisher>(1.0);

  // 3. Assert
  EXPECT_TRUE(node.ok());
}

TEST(HarnessTest, PublisherSendsMessagesOnSpin) {
  NodeTestHarness harness;
  auto& capture = harness.expect_publisher<rix::std_msgs::Header>("/chatter", 1);

  auto& node = harness.create<DemoPublisher>(1.0);
  EXPECT_TRUE(node.ok());

  // Drive 3 publish cycles: advance clock, then spin
  for (int i = 0; i < 3; i++) {
    harness.advance_time(1.0);
    harness.spin(1);
  }

  // Verify the publisher sent 3 messages with incrementing seq numbers
  EXPECT_EQ(capture.message_count(), 3u);
  for (size_t i = 0; i < capture.message_count(); i++) {
    EXPECT_EQ(capture.message(i).seq, static_cast<uint32_t>(i + 1));
    EXPECT_EQ(capture.message(i).frame_id, "Hello!");
  }
}

TEST(HarnessTest, PublisherNodeRegistrationFails) {
  NodeTestHarness harness;
  harness.node_registration_fails();

  auto& node = harness.create<DemoPublisher>(1.0);
  EXPECT_FALSE(node.ok());
}

// =============================================================================
// Service Tests
// =============================================================================

TEST(HarnessTest, ServiceNodeCreatesSuccessfully) {
  NodeTestHarness harness;
  harness.expect_service<rix::std_msgs::UInt32, rix::std_msgs::String>("/alphabet");

  auto& node = harness.create<DemoService>();
  EXPECT_TRUE(node.ok());
}
