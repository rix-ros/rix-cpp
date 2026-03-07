#pragma once

#include "rix/core/node.hpp"
#include "rix/sys_msgs/ActResponse.hpp"
#include "rix/sys_msgs/SrvResponse.hpp"
#include "rix/sys_msgs/SubNotify.hpp"
#include "rix/test/acceptor_builder.hpp"
#include "rix/test/mock_clock.hpp"
#include "rix/test/mock_poller.hpp"
#include "rix/test/stream_builder.hpp"
#include "rix/test/transport_manager.hpp"
#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rix::test {

// ============================================================================
// Handles — lightweight objects for injecting/capturing messages in tests
// ============================================================================

/**
 * @brief Handle returned by expect_publisher().
 *        Captures all messages the publisher sends to its subscribers.
 */
template <typename TMsg> class PublisherCapture {
public:
  size_t message_count() const { return messages_.size(); }
  const TMsg& message(size_t i) const { return messages_.at(i); }
  const TMsg& last_message() const { return messages_.back(); }
  const std::vector<TMsg>& messages() const { return messages_; }

private:
  friend class NodeTestHarness;
  std::vector<TMsg> messages_;
};

/**
 * @brief Handle returned by expect_subscriber().
 *        Provides inject() to feed messages into the subscriber.
 */
template <typename TMsg> class SubscriberInjector {
public:
  /**
   * @brief Queue messages to be delivered to the subscriber on the next spin cycle(s).
   */
  void inject(const std::vector<TMsg>& msgs) {
    for (auto& m : msgs)
      pending_.push_back(m);
  }
  void inject(const TMsg& msg) { pending_.push_back(msg); }

private:
  friend class NodeTestHarness;
  std::vector<TMsg> pending_;
};

/**
 * @brief Handle returned by expect_service().
 *        Records all received requests and the responses that were sent.
 */
template <typename TReq, typename TRes> class ServiceCapture {
public:
  size_t request_count() const { return requests_.size(); }
  const TReq& request(size_t i) const { return requests_.at(i); }
  const TRes& response(size_t i) const { return responses_.at(i); }

private:
  friend class NodeTestHarness;
  std::vector<TReq> requests_;
  std::vector<TRes> responses_;
};

// ============================================================================
// Declared components — internal bookkeeping per expect_xxx() call
// ============================================================================

namespace detail {

struct DeclaredPublisher {
  std::string topic;
  std::array<uint64_t, 2> msg_hash;
  int subscriber_count;
  bool should_fail;
  // Callback to set up connection streams after the acceptor is created
  // (captures happen inside the connection stream's send_message mock)
  std::function<void(TransportManager&, bool notifications)> setup_connections;
};

struct DeclaredSubscriber {
  std::string topic;
  std::array<uint64_t, 2> msg_hash;
  bool should_fail;
  // Injects messages via publisher connections after notification
  std::function<void(TransportManager&, bool notifications)> setup_connections;
  Endpoint pub_endpoint{DEFAULT_IP, 9000};
};

struct DeclaredService {
  std::string name;
  std::array<uint64_t, 2> req_hash;
  std::array<uint64_t, 2> res_hash;
  int client_count;
  bool should_fail;
  std::function<void(TransportManager&, bool notifications)> setup_connections;
};

struct DeclaredServiceClient {
  std::string name;
  std::array<uint64_t, 2> req_hash;
  std::array<uint64_t, 2> res_hash;
  bool should_fail;
  Endpoint service_endpoint{DEFAULT_IP, 8000};
  std::function<void(TransportManager&, bool notifications)> setup_connections;
};

struct DeclaredAction {
  std::string name;
  std::array<uint64_t, 2> goal_hash;
  std::array<uint64_t, 2> feedback_hash;
  std::array<uint64_t, 2> result_hash;
  int client_count;
  bool should_fail;
  std::function<void(TransportManager&, bool notifications)> setup_connections;
};

struct DeclaredActionClient {
  std::string name;
  std::array<uint64_t, 2> goal_hash;
  std::array<uint64_t, 2> feedback_hash;
  std::array<uint64_t, 2> result_hash;
  bool should_fail;
  Endpoint action_endpoint{DEFAULT_IP, 8000};
  std::function<void(TransportManager&, bool notifications)> setup_connections;
};

} // namespace detail

// ============================================================================
// NodeTestHarness — Fixture 2: Educator-facing test fixture
// ============================================================================

/**
 * @brief A simple, high-level test harness for testing custom RIX Nodes.
 *
 * Designed for educators building autograding pipelines. The API hides
 * all IPC mocking details behind intuitive methods:
 *
 *   1. Declare expected components: expect_publisher, expect_subscriber, etc.
 *      These return lightweight handles for capturing or injecting messages.
 *   2. Call create<YourNode>(args...) to construct the node.
 *   3. Use spin() / advance_time() to drive execution.
 *   4. Assert on the captured data through the handles.
 *
 * Example:
 * @code
 *   rix::test::NodeTestHarness harness;
 *   auto& pub = harness.expect_publisher<std_msgs::Header>("/chatter", 1);
 *   auto& node = harness.create<SimplePublisher>(1.0, 0);
 *   EXPECT_TRUE(node.ok());
 *   harness.advance_time(3.0);
 *   harness.spin(3);
 *   EXPECT_EQ(pub.message_count(), 3);
 * @endcode
 */
class NodeTestHarness {
public:
  NodeTestHarness() : expected_id_(1), current_id_(0) {
    // Always provide a mock clock for deterministic time control
    clock_ = std::make_shared<rix::MockClock>();
    rix::Time::set_clock(clock_);
    rix::Pollable::set_poller(nullptr);
  }

  ~NodeTestHarness() {
    node_.reset();
    rix::reset_transport_factory(rix::Protocol::TCP);
    rix::Node::set_id_factory(rix::default_id_generator);
    rix::Time::set_clock(std::make_shared<rix::Clock>());
    rix::Pollable::set_poller(nullptr);
  }

  // ---------------------------------------------------------------------------
  // Phase 1: Declare expected node topology
  // ---------------------------------------------------------------------------

  /**
   * @brief Declare that the node-under-test will create a publisher on this topic.
   *
   * @param topic           Topic name the publisher will use.
   * @param subscriber_count How many mock subscribers to connect (default 1).
   *                          Set to 0 if you only want to verify registration.
   * @return A capture handle. After spinning, inspect captured messages here.
   */
  template <typename TMsg>
  PublisherCapture<TMsg>& expect_publisher(const std::string& topic, int subscriber_count = 1) {
    auto capture = std::make_shared<PublisherCapture<TMsg>>();
    publisher_captures_.push_back(capture);

    detail::DeclaredPublisher decl;
    decl.topic = topic;
    decl.msg_hash = TMsg().hash();
    decl.subscriber_count = subscriber_count;
    decl.should_fail = false;

    decl.setup_connections = [capture, subscriber_count](TransportManager& tm, bool notify) {
      for (int i = 0; i < subscriber_count; i++) {
        auto conn = tm.create_stream();
        // Configure the connection to record any messages the publisher sends
        ON_CALL(*conn, send_message)
            .WillByDefault([capture, weak_conn = std::weak_ptr<MockStream>(conn)](uint8_t op, const Message& msg) {
              if (op == OPCODE::PUB_MESSAGE) {
                const auto* typed = dynamic_cast<const TMsg*>(&msg);
                if (typed) {
                  capture->messages_.push_back(*typed);
                }
              }
              if (auto c = weak_conn.lock()) {
                c->notify_operation_complete();
              }
              return true;
            });
        EXPECT_CALL(*conn, send_message).Times(::testing::AtLeast(0));
        EXPECT_CALL(*conn, wait_writable(::testing::_))
            .Times(::testing::AtLeast(0))
            .WillRepeatedly(::testing::Return(true));
      }
    };

    declared_publishers_.push_back(std::move(decl));
    return *capture;
  }

  /**
   * @brief Declare that publisher registration should fail.
   */
  template <typename TMsg> NodeTestHarness& expect_publisher_fails(const std::string& topic) {
    detail::DeclaredPublisher decl;
    decl.topic = topic;
    decl.msg_hash = TMsg().hash();
    decl.subscriber_count = 0;
    decl.should_fail = true;
    decl.setup_connections = [](TransportManager&, bool) {};
    declared_publishers_.push_back(std::move(decl));
    return *this;
  }

  /**
   * @brief Declare that the node-under-test will create a subscriber on this topic.
   *
   * @param topic Topic name the subscriber will listen on.
   * @return An injector handle. Call inject() to feed messages before/during spin.
   */
  template <typename TMsg> SubscriberInjector<TMsg>& expect_subscriber(const std::string& topic) {
    auto injector = std::make_shared<SubscriberInjector<TMsg>>();
    subscriber_injectors_.push_back(injector);

    detail::DeclaredSubscriber decl;
    decl.topic = topic;
    decl.msg_hash = TMsg().hash();
    decl.should_fail = false;
    static uint16_t pub_port = 9000;
    decl.pub_endpoint = Endpoint(DEFAULT_IP, pub_port++);

    decl.setup_connections = [injector, topic, ep = decl.pub_endpoint](TransportManager& tm, bool notify) {
      // Notification stream (tells subscriber where publishers are)
      auto notify_conn = tm.create_stream();
      sys_msgs::SubNotify sub_notify;
      sub_notify.publishers.resize(1);
      sub_notify.publishers[0].topic_info.name = topic;
      sub_notify.publishers[0].topic_info.message_hash = TMsg().hash();
      sub_notify.publishers[0].endpoint.address = ep.address;
      sub_notify.publishers[0].endpoint.port = ep.port;
      sub_notify.publishers[0].id = 1;

      StreamBuilder(notify_conn).recv_message(OPCODE::SUB_NOTIFY, sub_notify).close();

      // Publisher connection stream (delivers actual messages)
      auto pub_conn = tm.create_stream();
      auto recv_count = std::make_shared<int>(0);
      auto expected_count = std::make_shared<int>(static_cast<int>(injector->pending_.size()));

      EXPECT_CALL(*pub_conn, set_blocking(false)).Times(1).WillOnce(::testing::Return(true));
      EXPECT_CALL(*pub_conn, get_blocking()).Times(::testing::AtLeast(0)).WillRepeatedly(::testing::Return(false));
      EXPECT_CALL(*pub_conn, set_blocking(true)).Times(1).WillOnce(::testing::Return(true));
      EXPECT_CALL(*pub_conn, wait_readable(::testing::_))
          .Times(::testing::AtLeast(0))
          .WillRepeatedly(::testing::Invoke([recv_count, expected_count]() {
            return *recv_count < *expected_count * 2; // *2 because each message = op header + body
          }));

      ::testing::Sequence seq;
      for (auto& msg : injector->pending_) {
        // Operation header
        EXPECT_CALL(*pub_conn, recv_message(::testing::_, ::testing::_))
            .Times(1)
            .InSequence(seq)
            .WillOnce(::testing::Invoke([msg, recv_count](Message& message, size_t sz) {
              auto op = dynamic_cast<sys_msgs::Operation*>(&message);
              if (op) {
                op->opcode = OPCODE::PUB_MESSAGE;
                op->len = msg.get_prefix_len();
                (*recv_count)++;
                return true;
              }
              return false;
            }));

        // Message body
        EXPECT_CALL(*pub_conn, recv_message(::testing::_, ::testing::_))
            .Times(1)
            .InSequence(seq)
            .WillOnce(::testing::Invoke(
                [msg, recv_count, weak_conn = std::weak_ptr<MockStream>(pub_conn)](Message& message, size_t) {
                  auto typed = dynamic_cast<TMsg*>(&message);
                  if (typed) {
                    *typed = msg;
                    (*recv_count)++;
                    if (auto c = weak_conn.lock())
                      c->notify_operation_complete();
                    return true;
                  }
                  return false;
                }));
      }
    };

    declared_subscribers_.push_back(std::move(decl));
    return *injector;
  }

  /**
   * @brief Declare that the node-under-test will create a service.
   */
  template <typename TReq, typename TRes>
  ServiceCapture<TReq, TRes>& expect_service(const std::string& name, int client_count = 1) {
    auto capture = std::make_shared<ServiceCapture<TReq, TRes>>();
    service_captures_.push_back(capture);

    detail::DeclaredService decl;
    decl.name = name;
    decl.req_hash = TReq().hash();
    decl.res_hash = TRes().hash();
    decl.client_count = client_count;
    decl.should_fail = false;

    decl.setup_connections = [capture, client_count](TransportManager& tm, bool notify) {
      for (int i = 0; i < client_count; i++) {
        auto conn = tm.create_stream();
        // Connection handles one request-response cycle
        auto recv_count = std::make_shared<int>(0);
        EXPECT_CALL(*conn, wait_readable(::testing::_))
            .Times(::testing::AtLeast(0))
            .WillRepeatedly(::testing::Invoke([recv_count]() { return *recv_count < 2; }));

        ::testing::Sequence seq;
        // Recv Operation header
        EXPECT_CALL(*conn, recv_message(::testing::_, ::testing::_))
            .Times(1)
            .InSequence(seq)
            .WillOnce(::testing::Invoke([recv_count](Message& message, size_t) {
              auto op = dynamic_cast<sys_msgs::Operation*>(&message);
              if (op) {
                op->opcode = OPCODE::SRV_REQUEST_MESSAGE;
                op->len = TReq().get_prefix_len();
                (*recv_count)++;
                return true;
              }
              return false;
            }));
        // Recv request body
        EXPECT_CALL(*conn, recv_message(::testing::_, ::testing::_))
            .Times(1)
            .InSequence(seq)
            .WillOnce(::testing::Invoke([capture, recv_count](Message& message, size_t) {
              auto typed = dynamic_cast<TReq*>(&message);
              if (typed) {
                capture->requests_.push_back(*typed);
                (*recv_count)++;
                return true;
              }
              return false;
            }));
        // Send response
        EXPECT_CALL(*conn, send_message)
            .Times(1)
            .InSequence(seq)
            .WillOnce(::testing::Invoke(
                [capture, weak_conn = std::weak_ptr<MockStream>(conn)](uint8_t op, const Message& msg) {
                  auto typed = dynamic_cast<const TRes*>(&msg);
                  if (typed)
                    capture->responses_.push_back(*typed);
                  if (auto c = weak_conn.lock())
                    c->notify_operation_complete();
                  return true;
                }));
        EXPECT_CALL(*conn, wait_writable(::testing::_))
            .Times(::testing::AtLeast(0))
            .WillRepeatedly(::testing::Return(true));
      }
    };

    declared_services_.push_back(std::move(decl));
    return *capture;
  }

  /**
   * @brief Declare that the node-under-test will create a service client.
   */
  template <typename TReq, typename TRes>
  NodeTestHarness& expect_service_client(const std::string& name,
                                         const Endpoint& service_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    detail::DeclaredServiceClient decl;
    decl.name = name;
    decl.req_hash = TReq().hash();
    decl.res_hash = TRes().hash();
    decl.should_fail = false;
    decl.service_endpoint = service_endpoint;
    decl.setup_connections = [](TransportManager&, bool) {};
    declared_service_clients_.push_back(std::move(decl));
    return *this;
  }

  /**
   * @brief Force node registration to fail (e.g. to test error handling).
   */
  NodeTestHarness& node_registration_fails() {
    node_should_fail_ = true;
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Phase 2: Create the node
  // ---------------------------------------------------------------------------

  /**
   * @brief Construct the node-under-test. Sets up all mock transport based on the
   *        previously declared topology, then instantiates TNode(args...).
   *
   * @tparam TNode  The node class (must derive from rix::Node).
   * @param args    Constructor arguments forwarded to TNode.
   * @return Reference to the constructed node.
   */
  template <typename TNode, typename... Args> TNode& create(Args&&... args) {
    static_assert(std::is_base_of_v<rix::Node, TNode>, "TNode must derive from rix::Node");

    // Wire up mock transport
    build_mock_transport();

    // Inject into global transport_factories
    rix::set_transport_factory(rix::Protocol::TCP, &transport_manager_.get_factory());
    rix::Node::set_id_factory([this]() { return ++current_id_; });

    // Construct the node
    auto node = std::make_unique<TNode>(std::forward<Args>(args)...);
    auto* ptr = node.get();
    node_ = std::move(node);
    return *ptr;
  }

  // ---------------------------------------------------------------------------
  // Phase 3: Drive execution
  // ---------------------------------------------------------------------------

  /**
   * @brief Spin the node n times.
   */
  void spin(int n = 1) {
    for (int i = 0; i < n; i++) {
      node_as_spinner()->spin_once();
    }
  }

  /**
   * @brief Advance the mock clock by the given number of seconds.
   */
  void advance_time(double seconds) { clock_->sleep_for(rix::Duration(seconds)); }

  /**
   * @brief Get direct access to the mock clock.
   */
  std::shared_ptr<rix::MockClock> clock() { return clock_; }

  /**
   * @brief Check if the node is ok.
   */
  bool node_ok() const { return node_ && node_as_spinner()->ok(); }

private:
  TransportManager transport_manager_;
  std::shared_ptr<rix::MockClock> clock_;
  std::unique_ptr<rix::Node> node_;

  uint64_t expected_id_;
  uint64_t current_id_;
  bool node_should_fail_ = false;

  // Declared components
  std::vector<detail::DeclaredPublisher> declared_publishers_;
  std::vector<detail::DeclaredSubscriber> declared_subscribers_;
  std::vector<detail::DeclaredService> declared_services_;
  std::vector<detail::DeclaredServiceClient> declared_service_clients_;
  std::vector<detail::DeclaredAction> declared_actions_;
  std::vector<detail::DeclaredActionClient> declared_action_clients_;

  // Capture handles (stored as void* to support heterogeneous types)
  std::vector<std::shared_ptr<void>> publisher_captures_;
  std::vector<std::shared_ptr<void>> subscriber_injectors_;
  std::vector<std::shared_ptr<void>> service_captures_;

  rix::Spinner* node_as_spinner() const { return static_cast<rix::Spinner*>(node_.get()); }

  /**
   * @brief Build the complete mock transport graph based on all declared components.
   *
   * Order of transport creation must match the order that the Node constructor
   * and its create_xxx methods request sockets:
   *   1. Node's server acceptor
   *   2. Node's registration stream
   *   3. For each component: its server acceptor + registration stream
   *   4. Connection/client streams (set up by each component's setup_connections)
   */
  void build_mock_transport() {
    static const Endpoint default_bound(DEFAULT_IP, 8000);
    uint16_t next_port = 8000;
    auto next_endpoint = [&next_port]() { return Endpoint(DEFAULT_IP, next_port++); };

    // --- Node server acceptor ---
    auto node_bound = next_endpoint();
    auto node_acceptor = transport_manager_.create_acceptor();
    int node_ping_count = 0; // No pings by default in harness
    AcceptorBuilder(node_acceptor).as_server(node_bound, node_ping_count, [this]() {
      return transport_manager_.get_factory().create_stream(Endpoint{}, true);
    });

    // --- Node registration stream ---
    // The node name is determined by the TNode subclass constructor, so we
    // cannot predict it here. Use flexible matching for the registration
    // message and only verify the opcode.
    uint64_t node_id = expected_id_++;

    sys_msgs::Status node_status;
    node_status.error = node_should_fail_ ? -1 : 0;
    node_status.id = node_id;

    auto node_reg = transport_manager_.create_stream();
    {
      ::testing::Sequence seq;

      // Accept any NODE_REGISTER send (name varies per TNode subclass)
      EXPECT_CALL(*node_reg, send_message(OPCODE::NODE_REGISTER, ::testing::_))
          .Times(1)
          .InSequence(seq)
          .WillOnce(::testing::Return(true));

      // Return Operation header with STATUS_RESPONSE opcode
      EXPECT_CALL(*node_reg, recv_message(::testing::_, ::testing::_))
          .Times(1)
          .InSequence(seq)
          .WillOnce(::testing::Invoke([node_status](Message& message, size_t) {
            auto* op = dynamic_cast<sys_msgs::Operation*>(&message);
            if (op) {
              op->opcode = OPCODE::STATUS_RESPONSE;
              op->len = node_status.get_prefix_len();
              return true;
            }
            return false;
          }));

      // Return Status body
      EXPECT_CALL(*node_reg, recv_message(::testing::_, ::testing::_))
          .Times(1)
          .InSequence(seq)
          .WillOnce(::testing::Invoke([node_status](Message& message, size_t) {
            auto* status = dynamic_cast<sys_msgs::Status*>(&message);
            if (status) {
              *status = node_status;
              return true;
            }
            return false;
          }));
    }

    if (node_should_fail_)
      return;

    // --- Publishers ---
    for (auto& pub_decl : declared_publishers_) {
      auto pub_bound = next_endpoint();

      auto pub_acceptor = transport_manager_.create_acceptor();
      AcceptorBuilder(pub_acceptor).as_server(pub_bound, pub_decl.subscriber_count, [this]() {
        return transport_manager_.get_factory().create_stream(Endpoint{}, true);
      });

      sys_msgs::PubInfo pub_info;
      pub_info.node_id = node_id;
      pub_info.id = expected_id_++;
      pub_info.topic_info.name = pub_decl.topic;
      pub_info.topic_info.message_hash = pub_decl.msg_hash;
      pub_info.endpoint.address = pub_bound.address;
      pub_info.endpoint.port = pub_bound.port;

      sys_msgs::Status pub_status;
      pub_status.error = pub_decl.should_fail ? -1 : 0;
      pub_status.id = pub_info.id;

      auto pub_reg = transport_manager_.create_stream();
      StreamBuilder(pub_reg)
          .send_message(OPCODE::PUB_REGISTER, pub_info)
          .recv_message(OPCODE::STATUS_RESPONSE, pub_status)
          .close();

      // Set up connection streams for subscribers
      pub_decl.setup_connections(transport_manager_, false);
    }

    // --- Subscribers ---
    for (auto& sub_decl : declared_subscribers_) {
      auto sub_bound = next_endpoint();

      // Subscriber server: accepts 1 notification connection
      auto sub_acceptor = transport_manager_.create_acceptor();
      AcceptorBuilder(sub_acceptor).as_server(sub_bound, 1, [this]() {
        return transport_manager_.get_factory().create_stream(Endpoint{}, true);
      });

      sys_msgs::SubInfo sub_info;
      sub_info.node_id = node_id;
      sub_info.id = expected_id_++;
      sub_info.topic_info.name = sub_decl.topic;
      sub_info.topic_info.message_hash = sub_decl.msg_hash;
      sub_info.endpoint.address = sub_bound.address;
      sub_info.endpoint.port = sub_bound.port;

      sys_msgs::Status sub_status;
      sub_status.error = sub_decl.should_fail ? -1 : 0;
      sub_status.id = sub_info.id;

      auto sub_reg = transport_manager_.create_stream();
      StreamBuilder(sub_reg)
          .send_message(OPCODE::SUB_REGISTER, sub_info)
          .recv_message(OPCODE::STATUS_RESPONSE, sub_status)
          .close();

      // Set up notification + publisher connection streams
      sub_decl.setup_connections(transport_manager_, false);
    }

    // --- Services ---
    for (auto& srv_decl : declared_services_) {
      auto srv_bound = next_endpoint();

      auto srv_acceptor = transport_manager_.create_acceptor();
      AcceptorBuilder(srv_acceptor).as_server(srv_bound, srv_decl.client_count, [this]() {
        return transport_manager_.get_factory().create_stream(Endpoint{}, true);
      });

      sys_msgs::SrvInfo srv_info;
      srv_info.node_id = node_id;
      srv_info.id = expected_id_++;
      srv_info.name = srv_decl.name;
      srv_info.request_hash = srv_decl.req_hash;
      srv_info.response_hash = srv_decl.res_hash;
      srv_info.endpoint.address = srv_bound.address;
      srv_info.endpoint.port = srv_bound.port;

      sys_msgs::Status srv_status;
      srv_status.error = srv_decl.should_fail ? -1 : 0;
      srv_status.id = srv_info.id;

      auto srv_reg = transport_manager_.create_stream();
      StreamBuilder(srv_reg)
          .send_message(OPCODE::SRV_REGISTER, srv_info)
          .recv_message(OPCODE::STATUS_RESPONSE, srv_status)
          .close();

      srv_decl.setup_connections(transport_manager_, false);
    }

    // --- Service Clients ---
    for (auto& cli_decl : declared_service_clients_) {
      sys_msgs::SrvRequest srv_req;
      srv_req.node_id = node_id;
      srv_req.name = cli_decl.name;
      srv_req.request_hash = cli_decl.req_hash;
      srv_req.response_hash = cli_decl.res_hash;

      sys_msgs::SrvResponse srv_res;
      srv_res.error = cli_decl.should_fail ? -1 : 0;
      if (!cli_decl.should_fail) {
        srv_res.srv_info.name = cli_decl.name;
        srv_res.srv_info.request_hash = cli_decl.req_hash;
        srv_res.srv_info.response_hash = cli_decl.res_hash;
        srv_res.srv_info.endpoint.address = cli_decl.service_endpoint.address;
        srv_res.srv_info.endpoint.port = cli_decl.service_endpoint.port;
      }

      auto cli_reg = transport_manager_.create_stream();
      StreamBuilder(cli_reg)
          .send_message(OPCODE::SRV_REQUEST, srv_req)
          .recv_message(OPCODE::SRV_RESPONSE, srv_res)
          .close();

      cli_decl.setup_connections(transport_manager_, false);
    }

    // Node deregistration stream (consumed on Node destruction)
    // Same flexibility as registration — accept any NODE_DEREGISTER message.
    auto dereg = transport_manager_.create_stream();
    EXPECT_CALL(*dereg, send_message(OPCODE::NODE_DEREGISTER, ::testing::_)).Times(1).WillOnce(::testing::Return(true));

    // Also set up deregistration streams for each component
    // (consumed on component destruction — Publisher/Subscriber/Service destructors send dereg)
    for (auto& pub_decl : declared_publishers_) {
      if (!pub_decl.should_fail) {
        auto dereg_stream = transport_manager_.create_stream();
        sys_msgs::PubInfo pub_info;
        pub_info.topic_info.name = pub_decl.topic;
        // We use AnyOf matcher pattern on deregistration — just accept any send
        EXPECT_CALL(*dereg_stream, send_message(::testing::_, ::testing::_))
            .Times(::testing::AtLeast(0))
            .WillRepeatedly(::testing::Return(true));
      }
    }
    for (auto& sub_decl : declared_subscribers_) {
      if (!sub_decl.should_fail) {
        auto dereg_stream = transport_manager_.create_stream();
        EXPECT_CALL(*dereg_stream, send_message(::testing::_, ::testing::_))
            .Times(::testing::AtLeast(0))
            .WillRepeatedly(::testing::Return(true));
      }
    }
    for (auto& srv_decl : declared_services_) {
      if (!srv_decl.should_fail) {
        auto dereg_stream = transport_manager_.create_stream();
        EXPECT_CALL(*dereg_stream, send_message(::testing::_, ::testing::_))
            .Times(::testing::AtLeast(0))
            .WillRepeatedly(::testing::Return(true));
      }
    }
  }
};

} // namespace rix::test
