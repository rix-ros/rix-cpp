/**
 * @file e2e_test.cpp
 * @brief End-to-end tests exercising real Node, Publisher, and Subscriber over
 *        a live Mediator.
 *
 * These tests spin a real Mediator on an ephemeral port, connect real Nodes to
 * it, and exchange messages.  No mock transport is involved.  Both TCP and SHM
 * protocols are tested via a parameterized fixture.
 *
 * Synchronization pattern:
 *   - Mediator runs in its own thread.
 *   - Publisher and subscriber nodes each run in their own threads via spin().
 *   - A promise/future pair is used to detect receipt of the expected message.
 *   - Tests time-out if the message is not received within a short deadline.
 *
 * Thread cleanup: threads are shut down and joined in TearDown via
 * node_threads_, which ensures cleanup runs even when an assertion fails.
 */

#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

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

/** Timeout used for all blocking waits in these tests. */
static constexpr auto kTimeout = std::chrono::seconds(5);

/**
 * @brief RAII guard that shuts down a Node and joins its spin thread.
 */
struct NodeThread {
    Node& node;
    std::thread thread;

    NodeThread(Node& n, std::function<void()> spin_fn)
        : node(n), thread(std::move(spin_fn)) {}

    ~NodeThread() {
        node.shutdown();
        if (thread.joinable()) thread.join();
    }

    // Non-copyable, non-movable
    NodeThread(const NodeThread&) = delete;
    NodeThread& operator=(const NodeThread&) = delete;
};

} // namespace

// ---------------------------------------------------------------------------
// Parameterized fixture
// ---------------------------------------------------------------------------

struct E2EParams {
    std::string name;
    TransportOptions options;
};

class E2ETest : public ::testing::TestWithParam<E2EParams> {
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
// Tests
// ---------------------------------------------------------------------------

/**
 * @brief Publisher and subscriber exchange a single string message.
 *
 * The subscriber callback fulfills a promise; the test waits on the future
 * with a timeout to avoid hanging forever if delivery fails.
 */
TEST_P(E2ETest, SingleMessageDelivery) {
    const TransportOptions opts = GetParam().options;
    const std::string topic = "e2e_test_topic";
    const std::string payload = "hello_e2e";

    std::promise<std::string> received_promise;
    auto received_future = received_promise.get_future();

    // --- subscriber node ---
    Node sub_node("sub_node", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(sub_node.ok()) << "subscriber node failed to register";

    auto sub = sub_node.create_subscriber<std_msgs::String>(
        topic,
        [&received_promise](const std_msgs::String& msg) {
            received_promise.set_value(msg.data);
        },
        Endpoint("127.0.0.1", 0), opts);
    ASSERT_NE(sub, nullptr) << "create_subscriber returned nullptr";

    NodeThread sub_guard(sub_node, [&sub_node]() { sub_node.spin(); });

    // --- publisher node ---
    Node pub_node("pub_node", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(pub_node.ok()) << "publisher node failed to register";

    auto pub = pub_node.create_publisher<std_msgs::String>(
        topic, Endpoint("127.0.0.1", 0), opts);
    ASSERT_NE(pub, nullptr) << "create_publisher returned nullptr";

    NodeThread pub_guard(pub_node, [&pub_node]() { pub_node.spin(); });

    // Allow time for the mediator to process registrations and notify the
    // subscriber of the new publisher.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Publish until the subscriber receives or the deadline expires.
    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (received_future.wait_for(std::chrono::milliseconds(0)) !=
               std::future_status::ready &&
           std::chrono::steady_clock::now() < deadline) {
        std_msgs::String msg;
        msg.data = payload;
        pub->publish(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // NodeThread guards will shut down and join threads when going out of
    // scope, so we can safely assert here.
    EXPECT_EQ(received_future.wait_for(std::chrono::milliseconds(0)),
              std::future_status::ready)
        << "message was not received within timeout";

    if (received_future.wait_for(std::chrono::milliseconds(0)) ==
        std::future_status::ready) {
        EXPECT_EQ(received_future.get(), payload);
    }
}

/**
 * @brief Multiple messages are published and all must be received in order.
 */
TEST_P(E2ETest, MultipleMessagesDelivery) {
    const TransportOptions opts = GetParam().options;
    const std::string topic = "e2e_multi_topic";
    constexpr int kCount = 5;

    std::vector<std::string> received;
    std::mutex received_mutex;
    std::condition_variable received_cv;

    Node sub_node("sub_node_multi", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(sub_node.ok());

    auto sub = sub_node.create_subscriber<std_msgs::String>(
        topic,
        [&](const std_msgs::String& msg) {
            std::lock_guard<std::mutex> lock(received_mutex);
            received.push_back(msg.data);
            received_cv.notify_one();
        },
        Endpoint("127.0.0.1", 0), opts);
    ASSERT_NE(sub, nullptr);

    NodeThread sub_guard(sub_node, [&sub_node]() { sub_node.spin(); });

    Node pub_node("pub_node_multi", Endpoint("127.0.0.1", 0), mediator_ep_);
    ASSERT_TRUE(pub_node.ok());

    auto pub = pub_node.create_publisher<std_msgs::String>(
        topic, Endpoint("127.0.0.1", 0), opts);
    ASSERT_NE(pub, nullptr);

    NodeThread pub_guard(pub_node, [&pub_node]() { pub_node.spin(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Publish kCount messages, retrying each until acknowledgement.
    for (int i = 0; i < kCount; ++i) {
        std_msgs::String msg;
        msg.data = "msg_" + std::to_string(i);

        auto deadline = std::chrono::steady_clock::now() + kTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            {
                std::lock_guard<std::mutex> lock(received_mutex);
                if (static_cast<int>(received.size()) > i) break;
            }
            pub->publish(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Wait for all messages.
    bool all_received;
    {
        std::unique_lock<std::mutex> lock(received_mutex);
        all_received = received_cv.wait_for(lock, kTimeout,
                                            [&]() { return static_cast<int>(received.size()) >= kCount; });
    }

    EXPECT_TRUE(all_received) << "did not receive all messages in time";

    std::lock_guard<std::mutex> lock(received_mutex);
    for (int i = 0; i < std::min(kCount, static_cast<int>(received.size())); ++i) {
        EXPECT_EQ(received[i], "msg_" + std::to_string(i));
    }
}

/**
 * @brief Node reports ok() after successfully connecting to the mediator.
 */
TEST_P(E2ETest, NodeRegistration) {
    Node node("registration_node", Endpoint("127.0.0.1", 0), mediator_ep_);
    EXPECT_TRUE(node.ok());

    // Give the mediator a moment to process the registration.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(mediator_.get_node_count(), 1u);
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    TCP, E2ETest,
    ::testing::Values(E2EParams{"TCP", TransportOptions::tcp()}),
    [](const ::testing::TestParamInfo<E2EParams>& info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(
    SHM, E2ETest,
    ::testing::Values(E2EParams{"SHM", TransportOptions::shm(4 * 1024 * 1024)}),
    [](const ::testing::TestParamInfo<E2EParams>& info) { return info.param.name; });
