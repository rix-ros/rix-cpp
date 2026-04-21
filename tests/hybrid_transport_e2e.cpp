/**
 * @file hybrid_test.cpp
 * @brief Tests exercising mixed TCP and SHM transports within the same mediator
 *        session.
 *
 * Each test spins a real Mediator on an ephemeral port and creates Nodes whose
 * components use different protocols simultaneously.  The goal is to verify
 * that TCP and SHM transports coexist without interfering with one another.
 *
 * Scenarios covered:
 *
 *   MixedTopicsOnOneNode
 *     A single publisher-node hosts one TCP publisher and one SHM publisher on
 *     separate topics.  A single subscriber-node hosts matching TCP and SHM
 *     subscribers.  Both messages must arrive.
 *
 *   ConcurrentPipelines
 *     Two independent pub→sub pipelines (one TCP, one SHM) run in parallel on
 *     the same mediator.  Each pipeline delivers its message independently;
 *     neither transport should block the other.
 *
 *   BidirectionalMixed
 *     Node A sends over TCP to Node B; Node B replies over SHM to Node A.
 *     Tests that a single node can simultaneously hold a TCP subscriber and an
 *     SHM publisher (or vice versa) and that both channels deliver correctly.
 */

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "rix/core/mediator.hpp"
#include "rix/core/node.hpp"
#include "rix/ipc/transport_factory.hpp"
#include "rix/std_msgs/String.hpp"
#include "rix/std_msgs/UInt32.hpp"

using namespace rix;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

static constexpr auto kTimeout = std::chrono::seconds(5);

/** RAII guard: shuts down a Node and joins its spin thread on destruction. */
struct NodeThread {
    Node& node;
    std::thread thread;

    NodeThread(Node& n, std::function<void()> spin_fn)
        : node(n), thread(std::move(spin_fn)) {}

    ~NodeThread() {
        node.shutdown();
        if (thread.joinable()) thread.join();
    }

    NodeThread(const NodeThread&) = delete;
    NodeThread& operator=(const NodeThread&) = delete;
};

/**
 * @brief Publish @p msg on @p pub every 10 ms until @p fut becomes ready or
 *        the deadline expires.  Returns true if the future is ready.
 */
template <typename T>
bool publish_until_ready(std::shared_ptr<Publisher> pub, const T& msg,
                         std::future<std::string>& fut) {
    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (fut.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready &&
           std::chrono::steady_clock::now() < deadline) {
        pub->publish(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

} // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class HybridTest : public ::testing::Test {
protected:
    void SetUp() override {
        mediator_ep_ = mediator_.local_endpoint();
        mediator_thread_ = std::thread([this]() { mediator_.spin(); });
    }

    void TearDown() override {
        mediator_.shutdown();
        if (mediator_thread_.joinable()) mediator_thread_.join();
    }

    Mediator mediator_{Endpoint("127.0.0.1", 0)};
    std::thread mediator_thread_;
    Endpoint mediator_ep_{};
};

// ---------------------------------------------------------------------------
// Test 1: Mixed-protocol topics on the same nodes
// ---------------------------------------------------------------------------

/**
 * @brief One publisher-node hosts a TCP pub and an SHM pub on different topics.
 *        One subscriber-node hosts matching subs.  Both messages must arrive.
 */
TEST_F(HybridTest, MixedTopicsOnOneNode) {
    const std::string tcp_topic = "hybrid_tcp_topic";
    const std::string shm_topic = "hybrid_shm_topic";
    const std::string tcp_payload = "tcp_hello";
    const std::string shm_payload = "shm_hello";

    std::promise<std::string> tcp_promise, shm_promise;
    auto tcp_future = tcp_promise.get_future();
    auto shm_future = shm_promise.get_future();

    // --- subscriber node with one TCP sub and one SHM sub ---
    Node sub_node("hybrid_sub", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(sub_node.ok());

    auto tcp_sub = sub_node.create_subscriber<std_msgs::String>(
        tcp_topic,
        [&tcp_promise](const std_msgs::String& msg) {
            tcp_promise.set_value(msg.data);
        },
        Endpoint("127.0.0.1", 0), TransportOptions::tcp());
    ASSERT_NE(tcp_sub, nullptr);

    auto shm_sub = sub_node.create_subscriber<std_msgs::String>(
        shm_topic,
        [&shm_promise](const std_msgs::String& msg) {
            shm_promise.set_value(msg.data);
        },
        Endpoint("127.0.0.1", 0), TransportOptions::shm());
    ASSERT_NE(shm_sub, nullptr);

    NodeThread sub_guard(sub_node, [&sub_node]() { sub_node.spin(); });

    // --- publisher node with one TCP pub and one SHM pub ---
    Node pub_node("hybrid_pub", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(pub_node.ok());

    auto tcp_pub = pub_node.create_publisher<std_msgs::String>(
        tcp_topic, Endpoint("127.0.0.1", 0), TransportOptions::tcp());
    ASSERT_NE(tcp_pub, nullptr);

    auto shm_pub = pub_node.create_publisher<std_msgs::String>(
        shm_topic, Endpoint("127.0.0.1", 0), TransportOptions::shm());
    ASSERT_NE(shm_pub, nullptr);

    NodeThread pub_guard(pub_node, [&pub_node]() { pub_node.spin(); });

    // Allow mediator to complete registration and notify subscribers.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std_msgs::String tcp_msg, shm_msg;
    tcp_msg.data = tcp_payload;
    shm_msg.data = shm_payload;

    // Publish on both channels concurrently until both futures are ready.
    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while ((tcp_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready ||
            shm_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) &&
           std::chrono::steady_clock::now() < deadline) {
        if (tcp_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            tcp_pub->publish(tcp_msg);
        if (shm_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            shm_pub->publish(shm_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(tcp_future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready)
        << "TCP message was not received within timeout";
    EXPECT_EQ(shm_future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready)
        << "SHM message was not received within timeout";

    if (tcp_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        EXPECT_EQ(tcp_future.get(), tcp_payload);
    if (shm_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        EXPECT_EQ(shm_future.get(), shm_payload);
}

// ---------------------------------------------------------------------------
// Test 2: Concurrent independent pipelines
// ---------------------------------------------------------------------------

/**
 * @brief A TCP pipeline and an SHM pipeline run in parallel on the same
 *        mediator.  Each has its own pub-node and sub-node.  Both must deliver
 *        their message within the timeout.
 */
TEST_F(HybridTest, ConcurrentPipelines) {
    const std::string tcp_topic = "concurrent_tcp";
    const std::string shm_topic = "concurrent_shm";

    std::promise<std::string> tcp_promise, shm_promise;
    auto tcp_future = tcp_promise.get_future();
    auto shm_future = shm_promise.get_future();

    // TCP pipeline nodes
    Node tcp_sub_node("concurrent_tcp_sub", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(tcp_sub_node.ok());
    auto tcp_sub = tcp_sub_node.create_subscriber<std_msgs::String>(
        tcp_topic,
        [&tcp_promise](const std_msgs::String& msg) { tcp_promise.set_value(msg.data); },
        Endpoint("127.0.0.1", 0), TransportOptions::tcp());
    ASSERT_NE(tcp_sub, nullptr);
    NodeThread tcp_sub_guard(tcp_sub_node, [&tcp_sub_node]() { tcp_sub_node.spin(); });

    Node tcp_pub_node("concurrent_tcp_pub", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(tcp_pub_node.ok());
    auto tcp_pub = tcp_pub_node.create_publisher<std_msgs::String>(
        tcp_topic, Endpoint("127.0.0.1", 0), TransportOptions::tcp());
    ASSERT_NE(tcp_pub, nullptr);
    NodeThread tcp_pub_guard(tcp_pub_node, [&tcp_pub_node]() { tcp_pub_node.spin(); });

    // SHM pipeline nodes
    Node shm_sub_node("concurrent_shm_sub", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(shm_sub_node.ok());
    auto shm_sub = shm_sub_node.create_subscriber<std_msgs::String>(
        shm_topic,
        [&shm_promise](const std_msgs::String& msg) { shm_promise.set_value(msg.data); },
        Endpoint("127.0.0.1", 0), TransportOptions::shm());
    ASSERT_NE(shm_sub, nullptr);
    NodeThread shm_sub_guard(shm_sub_node, [&shm_sub_node]() { shm_sub_node.spin(); });

    Node shm_pub_node("concurrent_shm_pub", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(shm_pub_node.ok());
    auto shm_pub = shm_pub_node.create_publisher<std_msgs::String>(
        shm_topic, Endpoint("127.0.0.1", 0), TransportOptions::shm());
    ASSERT_NE(shm_pub, nullptr);
    NodeThread shm_pub_guard(shm_pub_node, [&shm_pub_node]() { shm_pub_node.spin(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std_msgs::String tcp_msg, shm_msg;
    tcp_msg.data = "concurrent_tcp_payload";
    shm_msg.data = "concurrent_shm_payload";

    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while ((tcp_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready ||
            shm_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) &&
           std::chrono::steady_clock::now() < deadline) {
        if (tcp_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            tcp_pub->publish(tcp_msg);
        if (shm_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            shm_pub->publish(shm_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(tcp_future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready)
        << "TCP pipeline did not deliver within timeout";
    EXPECT_EQ(shm_future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready)
        << "SHM pipeline did not deliver within timeout";

    if (tcp_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        EXPECT_EQ(tcp_future.get(), "concurrent_tcp_payload");
    if (shm_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        EXPECT_EQ(shm_future.get(), "concurrent_shm_payload");
}

// ---------------------------------------------------------------------------
// Test 3: Bidirectional mixed-protocol channels
// ---------------------------------------------------------------------------

/**
 * @brief Node A sends over TCP to Node B; Node B sends over SHM back to Node A.
 *
 * Each node simultaneously holds a publisher on one protocol and a subscriber
 * on the other, verifying that mixed-protocol co-hosting works correctly.
 */
TEST_F(HybridTest, BidirectionalMixed) {
    // A→B channel uses TCP; B→A channel uses SHM.
    const std::string a_to_b_topic = "bidir_a_to_b";
    const std::string b_to_a_topic = "bidir_b_to_a";

    std::promise<std::string> b_received_promise;  // B receives from A via TCP
    std::promise<std::string> a_received_promise;  // A receives from B via SHM
    auto b_future = b_received_promise.get_future();
    auto a_future = a_received_promise.get_future();

    // Node A: TCP publisher (→B) + SHM subscriber (←B)
    Node node_a("bidir_node_a", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(node_a.ok());

    auto a_pub = node_a.create_publisher<std_msgs::String>(
        a_to_b_topic, Endpoint("127.0.0.1", 0), TransportOptions::tcp());
    ASSERT_NE(a_pub, nullptr);

    auto a_sub = node_a.create_subscriber<std_msgs::String>(
        b_to_a_topic,
        [&a_received_promise](const std_msgs::String& msg) {
            a_received_promise.set_value(msg.data);
        },
        Endpoint("127.0.0.1", 0), TransportOptions::shm());
    ASSERT_NE(a_sub, nullptr);

    NodeThread node_a_guard(node_a, [&node_a]() { node_a.spin(); });

    // Node B: SHM publisher (→A) + TCP subscriber (←A)
    Node node_b("bidir_node_b", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(node_b.ok());

    auto b_pub = node_b.create_publisher<std_msgs::String>(
        b_to_a_topic, Endpoint("127.0.0.1", 0), TransportOptions::shm());
    ASSERT_NE(b_pub, nullptr);

    auto b_sub = node_b.create_subscriber<std_msgs::String>(
        a_to_b_topic,
        [&b_received_promise](const std_msgs::String& msg) {
            b_received_promise.set_value(msg.data);
        },
        Endpoint("127.0.0.1", 0), TransportOptions::tcp());
    ASSERT_NE(b_sub, nullptr);

    NodeThread node_b_guard(node_b, [&node_b]() { node_b.spin(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std_msgs::String a_msg, b_msg;
    a_msg.data = "from_a_via_tcp";
    b_msg.data = "from_b_via_shm";

    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while ((b_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready ||
            a_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) &&
           std::chrono::steady_clock::now() < deadline) {
        if (b_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            a_pub->publish(a_msg);
        if (a_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            b_pub->publish(b_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(b_future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready)
        << "Node B did not receive TCP message from Node A within timeout";
    EXPECT_EQ(a_future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready)
        << "Node A did not receive SHM message from Node B within timeout";

    if (b_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        EXPECT_EQ(b_future.get(), "from_a_via_tcp");
    if (a_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        EXPECT_EQ(a_future.get(), "from_b_via_shm");
}
