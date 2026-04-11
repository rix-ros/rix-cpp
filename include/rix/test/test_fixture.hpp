#pragma once

#include <gtest/gtest.h>

#include "rix/core/node.hpp"
#include "rix/std_msgs/UInt64.hpp"
#include "rix/std_msgs/Void.hpp"
#include "rix/sys_msgs/ActResponse.hpp"
#include "rix/sys_msgs/SrvResponse.hpp"
#include "rix/sys_msgs/SubNotify.hpp"
#include "rix/test/acceptor_builder.hpp"
#include "rix/test/mock_clock.hpp"
#include "rix/test/mock_poller.hpp"
#include "rix/test/stream_builder.hpp"
#include "rix/test/transport_manager.hpp"

namespace rix {

/**
 * @brief Fixture 1: Internal RIX Test Fixture.
 *
 * Provides a fluent builder API for testing RIX's own IPC and core components.
 * Tests set up expected mock transport behavior, then run a test function that
 * exercises the real Node / Mediator / component code against the mocks.
 *
 * Uses the new Acceptor/Stream interfaces introduced by the IPC refactor.
 *
 * Socket categorization (carried forward from old fixture for synchronization):
 *   - acceptor_sockets_: Server acceptors (Node, Publisher, Subscriber,
 * Service, Action)
 *   - connection_streams_: Streams returned by accept() (publisher→subscriber
 * connections, etc.)
 *   - client_streams_: Streams created by the SUT to connect outward
 * (subscriber→publisher, etc.)
 */
class TestFixture {
public:
  TestFixture()
      : rixhub_endpoint_(RIXHUB_IP, RIXHUB_PORT), enable_notifications_(false), enable_poller_(false),
        enable_clock_(false), expected_id_(1), current_id_(0) {
    Pollable::set_poller(nullptr);
  }

  ~TestFixture() {
    reset_transport_factory(Protocol::TCP);
    Node::set_id_factory(default_id_generator);
    if (enable_poller_) {
      Pollable::set_poller(nullptr);
    }
    if (enable_clock_) {
      Time::set_clock(std::make_shared<Clock>());
    }
  }

  template <typename TNode>
  using TestFunction = std::function<void(const TestFixture&)>;

  /**
   * @brief Inject mock transport, set ID factory, and invoke the test function.
   */
  template <typename TNode = Node>
  void build(TestFunction<TNode> test_func) {
    static_assert(std::is_base_of_v<Node, TNode>, "TNode must be Node or derived from Node");
    set_transport_factory(Protocol::TCP, &transport_manager_.get_factory());
    Node::set_id_factory([this]() { return ++current_id_; });
    test_func(*this);
  }

  // ---------------------------------------------------------------------------
  // Configuration: clock, poller, notifications
  // ---------------------------------------------------------------------------

  TestFixture& enable_clock(std::shared_ptr<MockClock>& clock) {
    if (enable_clock_)
      throw std::runtime_error("Clock already enabled");
    clock = std::make_shared<MockClock>();
    Time::set_clock(clock);
    enable_clock_ = true;
    return *this;
  }

  TestFixture& enable_poller(int max_poll_count = -1) {
    if (enable_poller_)
      throw std::runtime_error("Poller already enabled");
    enable_poller_ = true;
    auto poller = std::make_shared<MockPoller>(max_poll_count);
    Pollable::set_poller(poller);
    EXPECT_CALL(*poller, poll).Times(::testing::AtLeast(1));
    return *this;
  }

  TestFixture& enable_operation_notifications() {
    if (enable_notifications_)
      throw std::runtime_error("Operation notifications already enabled");
    enable_notifications_ = true;
    return *this;
  }

  TestFixture& disable_operation_notifications() {
    if (!enable_notifications_)
      throw std::runtime_error("Operation notifications not enabled");
    enable_notifications_ = false;
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Accessors for synchronization
  // ---------------------------------------------------------------------------

  // std::shared_ptr<MockAcceptor> get_acceptor(size_t index = 0) const {
  //   return index < acceptors_.size() ? acceptors_[index] : nullptr;
  // }

  // std::shared_ptr<MockStream> get_connection_stream(size_t index = 0) const {
  //   return index < connection_streams_.size() ? connection_streams_[index] : nullptr;
  // }

  // std::shared_ptr<MockStream> get_client_stream(size_t index = 0) const {
  //   return index < client_streams_.size() ? client_streams_[index] : nullptr;
  // }

  const std::vector<std::shared_ptr<MockAcceptor>>& get_acceptors() const { return acceptors_; }
  const std::vector<std::shared_ptr<MockStream>>& get_connection_streams() const { return connection_streams_; }
  const std::vector<std::shared_ptr<MockStream>>& get_client_streams() const { return client_streams_; }

  bool wait_for_all_connections(size_t ops_per_conn,
                                std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& conn : connection_streams_) {
      if (!conn->wait_for_operations(ops_per_conn, timeout))
        return false;
    }
    return true;
  }

  bool wait_for_all_clients(size_t ops_per_client,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& client : client_streams_) {
      if (!client->wait_for_operations(ops_per_client, timeout))
        return false;
    }
    return true;
  }

  // ---------------------------------------------------------------------------
  // Node lifecycle
  // ---------------------------------------------------------------------------

  TestFixture& create_node(const std::string& name,
                           sys_msgs::NodeInfo& node_info,
                           bool should_fail = false,
                           int ping_count = 0,
                           const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                           const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    // Node's server acceptor
    create_server(bound_endpoint, ping_count);

    node_info.id = expected_id_++;
    node_info.name = name;
    node_info.endpoint.address = bound_endpoint.address;
    node_info.endpoint.port = bound_endpoint.port;
    node_info.protocol = Protocol::TCP;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = node_info.id;

    // Registration stream (connects to rixhub)
    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg)
        .send_message(OPCODE::NODE_REGISTER, node_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  TestFixture& destroy_node(const sys_msgs::NodeInfo& node_info) {
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).send_message(OPCODE::NODE_DEREGISTER, node_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Publisher lifecycle
  // ---------------------------------------------------------------------------

  template <typename TMsg>
  TestFixture& create_publisher(const std::string& topic,
                                sys_msgs::PubInfo& pub_info,
                                const sys_msgs::NodeInfo& node_info,
                                bool should_fail = false,
                                int subscriber_count = 0,
                                Protocol protocol = Protocol::TCP,
                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    create_server(bound_endpoint, subscriber_count);

    pub_info.node_id = node_info.id;
    pub_info.id = expected_id_++;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = TMsg().hash();
    pub_info.endpoint.address = bound_endpoint.address;
    pub_info.endpoint.port = bound_endpoint.port;
    pub_info.protocol = protocol;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = pub_info.id;

    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg)
        .send_message(OPCODE::PUB_REGISTER, pub_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  TestFixture& destroy_publisher(const sys_msgs::PubInfo& pub_info) {
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).send_message(OPCODE::PUB_DEREGISTER, pub_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Subscriber lifecycle
  // ---------------------------------------------------------------------------

  template <typename TMsg>
  TestFixture& create_subscriber(const std::string& topic,
                                 sys_msgs::SubInfo& sub_info,
                                 const sys_msgs::NodeInfo& node_info,
                                 bool should_fail = false,
                                 int notification_count = 0,
                                 Protocol protocol = Protocol::TCP,
                                 const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                 const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be derived from Message");

    create_server(bound_endpoint, notification_count);

    sub_info.node_id = node_info.id;
    sub_info.id = expected_id_++;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = TMsg().hash();
    sub_info.endpoint.address = bound_endpoint.address;
    sub_info.endpoint.port = bound_endpoint.port;
    sub_info.protocol = protocol;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = sub_info.id;

    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg)
        .send_message(OPCODE::SUB_REGISTER, sub_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  TestFixture& destroy_subscriber(const sys_msgs::SubInfo& sub_info) {
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).send_message(OPCODE::SUB_DEREGISTER, sub_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Service lifecycle
  // ---------------------------------------------------------------------------

  template <typename TReq, typename TRes>
  TestFixture& create_service(const std::string& service,
                              sys_msgs::SrvInfo& srv_info,
                              const sys_msgs::NodeInfo& node_info,
                              bool should_fail = false,
                              int client_count = 0,
                              Protocol protocol = Protocol::TCP,
                              const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                              const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<Message, TReq>, "TReq must be derived from Message");
    static_assert(std::is_base_of_v<Message, TRes>, "TRes must be derived from Message");

    create_server(bound_endpoint, client_count);

    srv_info.node_id = node_info.id;
    srv_info.id = expected_id_++;
    srv_info.name = service;
    srv_info.request_hash = TReq().hash();
    srv_info.response_hash = TRes().hash();
    srv_info.endpoint.address = bound_endpoint.address;
    srv_info.endpoint.port = bound_endpoint.port;
    srv_info.protocol = protocol;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = srv_info.id;

    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg)
        .send_message(OPCODE::SRV_REGISTER, srv_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  TestFixture& destroy_service(const sys_msgs::SrvInfo& srv_info) {
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).send_message(OPCODE::SRV_DEREGISTER, srv_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Action lifecycle
  // ---------------------------------------------------------------------------

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& create_action(const std::string& action,
                             sys_msgs::ActInfo& act_info,
                             const sys_msgs::NodeInfo& node_info,
                             int iters_between_accept = 0,
                             bool should_fail = false,
                             int client_count = 0,
                             Protocol protocol = Protocol::TCP,
                             const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                             const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<Message, TGoal>, "TGoal must be derived from Message");
    static_assert(std::is_base_of_v<Message, TFeedback>, "TFeedback must be derived from Message");
    static_assert(std::is_base_of_v<Message, TResult>, "TResult must be derived from Message");

    create_server(bound_endpoint, client_count, iters_between_accept);

    act_info.node_id = node_info.id;
    act_info.id = expected_id_++;
    act_info.name = action;
    act_info.endpoint.address = bound_endpoint.address;
    act_info.endpoint.port = bound_endpoint.port;
    act_info.goal_hash = TGoal().hash();
    act_info.feedback_hash = TFeedback().hash();
    act_info.result_hash = TResult().hash();
    act_info.protocol = protocol;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = act_info.id;

    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg)
        .send_message(OPCODE::ACT_REGISTER, act_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  TestFixture& destroy_action(const sys_msgs::ActInfo& act_info) {
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).send_message(OPCODE::ACT_DEREGISTER, act_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Service client / Action client registration
  // ---------------------------------------------------------------------------

  template <typename TReq, typename TRes>
  TestFixture& create_service_client(const std::string& service,
                                     const sys_msgs::NodeInfo& node_info,
                                     bool should_fail = false,
                                     Protocol protocol = Protocol::TCP,
                                     const Endpoint& service_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    sys_msgs::SrvRequest srv_req;
    srv_req.node_id = node_info.id;
    srv_req.name = service;
    srv_req.request_hash = TReq().hash();
    srv_req.response_hash = TRes().hash();
    srv_req.protocol = protocol;

    sys_msgs::SrvResponse srv_res;
    srv_res.error = should_fail ? -1 : 0;
    if (!should_fail) {
      srv_res.srv_info.name = service;
      srv_res.srv_info.request_hash = TReq().hash();
      srv_res.srv_info.response_hash = TRes().hash();
      srv_res.srv_info.endpoint.address = service_endpoint.address;
      srv_res.srv_info.endpoint.port = service_endpoint.port;
    }

    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg).send_message(OPCODE::SRV_REQUEST, srv_req).recv_message(OPCODE::SRV_RESPONSE, srv_res).close();
    return *this;
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& create_action_client(const std::string& action,
                                    const sys_msgs::NodeInfo& node_info,
                                    bool should_fail = false,
                                    Protocol protocol = Protocol::TCP,
                                    const Endpoint& action_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    sys_msgs::ActRequest act_req;
    act_req.node_id = node_info.id;
    act_req.name = action;
    act_req.goal_hash = TGoal().hash();
    act_req.feedback_hash = TFeedback().hash();
    act_req.result_hash = TResult().hash();
    act_req.protocol = protocol;

    sys_msgs::ActResponse act_res;
    act_res.error = should_fail ? -1 : 0;
    if (!should_fail) {
      act_res.act_info.name = action;
      act_res.act_info.goal_hash = TGoal().hash();
      act_res.act_info.feedback_hash = TFeedback().hash();
      act_res.act_info.result_hash = TResult().hash();
      act_res.act_info.endpoint.address = action_endpoint.address;
      act_res.act_info.endpoint.port = action_endpoint.port;
    }

    auto reg = transport_manager_.create_stream();
    StreamBuilder(reg).send_message(OPCODE::ACT_REQUEST, act_req).recv_message(OPCODE::ACT_RESPONSE, act_res).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Ping
  // ---------------------------------------------------------------------------

  TestFixture& accept_ping(const sys_msgs::NodeInfo& node_info) {
    std::shared_ptr<MockStream> unused;
    return accept_ping(node_info, unused);
  }

  TestFixture& accept_ping(const sys_msgs::NodeInfo& node_info, std::shared_ptr<MockStream>& conn) {
    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    sys_msgs::Operation operation;
    operation.opcode = OPCODE::PING;
    operation.len = 0;

    sys_msgs::Status status;
    status.id = node_info.id;
    status.error = 0;

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();
    builder.recv_message(operation, operation.get_prefix_len()).send_message(OPCODE::STATUS_RESPONSE, status).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Publisher connection handling
  // ---------------------------------------------------------------------------

  template <typename TMsg>
  TestFixture& accept_subscriber(const std::vector<std::shared_ptr<TMsg>>& messages) {
    std::shared_ptr<MockStream> unused;
    return accept_subscriber<TMsg>(messages, unused);
  }

  template <typename TMsg>
  TestFixture& accept_subscriber(const std::vector<std::shared_ptr<TMsg>>& messages,
                                 std::shared_ptr<MockStream>& conn) {
    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();
    for (auto& msg : messages) {
      builder.send_message(OPCODE::PUB_MESSAGE, *msg);
    }
    builder.close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Subscriber notification and connection
  // ---------------------------------------------------------------------------

  template <typename TMsg>
  TestFixture& accept_notification(const std::string& topic, const std::vector<Endpoint>& publisher_endpoints) {
    std::shared_ptr<MockStream> unused;
    return accept_notification<TMsg>(topic, publisher_endpoints, unused);
  }

  template <typename TMsg>
  TestFixture& accept_notification(const std::string& topic,
                                   const std::vector<Endpoint>& publisher_endpoints,
                                   std::shared_ptr<MockStream>& conn) {
    sys_msgs::SubNotify sub_notify;
    sub_notify.publishers.resize(publisher_endpoints.size());
    for (size_t i = 0; i < publisher_endpoints.size(); i++) {
      sub_notify.publishers[i].topic_info.name = topic;
      sub_notify.publishers[i].topic_info.message_hash = TMsg().hash();
      sub_notify.publishers[i].endpoint.address = publisher_endpoints[i].address;
      sub_notify.publishers[i].endpoint.port = publisher_endpoints[i].port;
      sub_notify.publishers[i].id = static_cast<uint32_t>(i + 1);
    }

    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();
    builder.recv_message(OPCODE::SUB_NOTIFY, sub_notify).close();
    return *this;
  }

  template <typename TMsg>
  TestFixture& connect_to_publisher(const std::string& topic,
                                    const std::vector<Endpoint>& publisher_endpoints,
                                    const std::vector<std::shared_ptr<TMsg>>& messages) {
    std::shared_ptr<MockStream> unused;
    return connect_to_publisher<TMsg>(topic, publisher_endpoints, messages, unused);
  }

  template <typename TMsg>
  TestFixture& connect_to_publisher(const Endpoint& publisher_endpoint,
                                    const std::vector<std::shared_ptr<TMsg>>& messages,
                                    std::shared_ptr<MockStream>& client) {
    client = transport_manager_.create_stream();
    if (enable_notifications_)
      client_streams_.push_back(client);

    auto builder = StreamBuilder(client);
    if (enable_notifications_)
      builder.enable_operation_notifications();
    builder.set_blocking(true);
    for (auto& msg : messages) {
      builder.recv_message(OPCODE::PUB_MESSAGE, *msg);
    }
    builder.close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Service connection handling
  // ---------------------------------------------------------------------------

  template <typename TRequest, typename TResponse>
  TestFixture& accept_service_client(std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    std::shared_ptr<MockStream> unused;
    return accept_service_client<TRequest, TResponse>(request, response, unused);
  }

  template <typename TRequest, typename TResponse>
  TestFixture& accept_service_client(std::shared_ptr<TRequest> request,
                                     std::shared_ptr<TResponse> response,
                                     std::shared_ptr<MockStream>& conn) {
    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();
    builder.recv_message(OPCODE::SRV_REQUEST_MESSAGE, *request)
        .send_message(OPCODE::SRV_RESPONSE_MESSAGE, *response)
        .close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Service client call
  // ---------------------------------------------------------------------------

  template <typename TRequest, typename TResponse>
  TestFixture& call_service_client(std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    std::shared_ptr<MockStream> unused;
    return call_service_client<TRequest, TResponse>(request, response, unused);
  }

  template <typename TRequest, typename TResponse>
  TestFixture& call_service_client(std::shared_ptr<TRequest> request,
                                   std::shared_ptr<TResponse> response,
                                   std::shared_ptr<MockStream>& client) {
    client = transport_manager_.create_stream();
    if (enable_notifications_)
      client_streams_.push_back(client);

    auto builder = StreamBuilder(client);
    if (enable_notifications_)
      builder.enable_operation_notifications();
    builder.send_message(OPCODE::SRV_REQUEST_MESSAGE, *request)
        .recv_message(OPCODE::SRV_RESPONSE_MESSAGE, *response)
        .close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Action connection handling
  // ---------------------------------------------------------------------------

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client(std::shared_ptr<TGoal> goal,
                                    std::vector<std::shared_ptr<TFeedback>> feedback,
                                    std::shared_ptr<TResult> result) {
    std::shared_ptr<MockStream> unused;
    return accept_action_client<TGoal, TFeedback, TResult>(goal, feedback, result, unused);
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client(std::shared_ptr<TGoal> goal,
                                    std::vector<std::shared_ptr<TFeedback>> feedback,
                                    std::shared_ptr<TResult> result,
                                    bool should_fail) {
    std::shared_ptr<MockStream> unused;
    return accept_action_client<TGoal, TFeedback, TResult>(goal, feedback, result, unused, should_fail);
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client(std::shared_ptr<TGoal> goal,
                                    std::vector<std::shared_ptr<TFeedback>> feedback,
                                    std::shared_ptr<TResult> result,
                                    std::shared_ptr<MockStream>& conn,
                                    bool should_fail = false) {
    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    sys_msgs::Operation operation;
    operation.opcode = OPCODE::ACT_GOAL_MESSAGE;
    operation.len = goal->size();
    builder.recv_message(operation, operation.get_prefix_len());

    sys_msgs::Status status;
    if (should_fail) {
      status.error = -1;
      builder.send_message(OPCODE::ACT_RESPONSE_MESSAGE, status).close();
      return *this;
    } else {
      status.error = 0;
      builder.recv_message(*goal, goal->size()).send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    }
    for (auto& fb : feedback) {
      builder.send_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
    }
    builder.send_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

  /**
   * @brief Mock an action client that connects and then sends a cancel after receiving the goal status.
   */
  template <typename TGoal, typename TFeedback>
  TestFixture& accept_action_client_with_cancel(std::shared_ptr<TGoal> goal,
                                                std::vector<std::shared_ptr<TFeedback>> feedback) {
    std::shared_ptr<MockStream> unused;
    return accept_action_client_with_cancel<TGoal, TFeedback>(goal, feedback, unused);
  }

  template <typename TGoal, typename TFeedback>
  TestFixture& accept_action_client_with_cancel(std::shared_ptr<TGoal> goal,
                                                std::vector<std::shared_ptr<TFeedback>> feedback,
                                                std::shared_ptr<MockStream>& conn) {
    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    sys_msgs::Operation goal_op;
    goal_op.opcode = OPCODE::ACT_GOAL_MESSAGE;
    goal_op.len = goal->size();
    builder.recv_message(goal_op, goal_op.get_prefix_len());

    sys_msgs::Status status;
    status.error = 0;
    builder.recv_message(*goal, goal->size()).send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);

    // Expect a cancel message from the client
    sys_msgs::Operation cancel_op;
    cancel_op.opcode = OPCODE::ACT_CANCEL_MESSAGE;
    cancel_op.len = 0;
    builder.recv_message(cancel_op, cancel_op.get_prefix_len()).close();
    return *this;
  }

  /**
   * @brief Mock an action client that connects, gets preempted with new goals, and eventually completes.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client_with_preempt(std::vector<std::shared_ptr<TGoal>> goals,
                                                 std::vector<std::vector<std::shared_ptr<TFeedback>>> feedbacks,
                                                 std::shared_ptr<TResult> result) {
    std::shared_ptr<MockStream> unused;
    return accept_action_client_with_preempt<TGoal, TFeedback, TResult>(goals, feedbacks, result, unused);
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client_with_preempt(std::vector<std::shared_ptr<TGoal>> goals,
                                                 std::vector<std::vector<std::shared_ptr<TFeedback>>> feedbacks,
                                                 std::shared_ptr<TResult> result,
                                                 std::shared_ptr<MockStream>& conn) {
    conn = transport_manager_.create_stream();
    if (enable_notifications_)
      connection_streams_.push_back(conn);

    auto builder = StreamBuilder(conn);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    sys_msgs::Status status;
    status.error = 0;

    for (size_t i = 0; i < goals.size(); ++i) {
      sys_msgs::Operation op;
      if (i == 0) {
        op.opcode = OPCODE::ACT_GOAL_MESSAGE;
      } else {
        op.opcode = OPCODE::ACT_PREEMPT_MESSAGE;
      }
      op.len = goals[i]->size();
      builder.recv_message(op, op.get_prefix_len());
      builder.recv_message(*goals[i], goals[i]->size());
      builder.send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);

      for (auto& fb : feedbacks[i]) {
        builder.send_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
      }
    }
    builder.send_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Action client goal / feedback / result
  // ---------------------------------------------------------------------------

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& send_action_goal(std::shared_ptr<TGoal> goal,
                                std::vector<std::shared_ptr<TFeedback>> feedback,
                                std::shared_ptr<TResult> result,
                                bool should_fail = false) {
    std::shared_ptr<MockStream> unused;
    return send_action_goal<TGoal, TFeedback, TResult>(goal, feedback, result, unused, should_fail);
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& send_action_goal(std::shared_ptr<TGoal> goal,
                                std::vector<std::shared_ptr<TFeedback>> feedback,
                                std::shared_ptr<TResult> result,
                                std::shared_ptr<MockStream>& client,
                                bool should_fail = false) {
    client = transport_manager_.create_stream();
    if (enable_notifications_)
      client_streams_.push_back(client);

    auto builder = StreamBuilder(client);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    sys_msgs::Operation goal_op;
    goal_op.opcode = OPCODE::ACT_GOAL_MESSAGE;
    goal_op.len = goal->get_prefix_len();
    builder.send_message(OPCODE::ACT_GOAL_MESSAGE, *goal);

    sys_msgs::Status status;
    if (should_fail) {
      status.error = -1;
      builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status).close();
      return *this;
    }
    status.error = 0;
    builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    for (auto& fb : feedback) {
      builder.recv_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
    }
    builder.recv_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

  /**
   * @brief Mock an action server that accepts a goal, sends feedback, then receives a cancel.
   */
  template <typename TGoal, typename TFeedback>
  TestFixture& send_action_goal_with_cancel(std::shared_ptr<TGoal> goal,
                                            std::vector<std::shared_ptr<TFeedback>> feedback) {
    std::shared_ptr<MockStream> unused;
    return send_action_goal_with_cancel<TGoal, TFeedback>(goal, feedback, unused);
  }

  template <typename TGoal, typename TFeedback>
  TestFixture& send_action_goal_with_cancel(std::shared_ptr<TGoal> goal,
                                            std::vector<std::shared_ptr<TFeedback>> feedback,
                                            std::shared_ptr<MockStream>& client) {
    client = transport_manager_.create_stream();
    if (enable_notifications_)
      client_streams_.push_back(client);

    auto builder = StreamBuilder(client);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    builder.send_message(OPCODE::ACT_GOAL_MESSAGE, *goal);

    sys_msgs::Status status;
    status.error = 0;
    builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    for (auto& fb : feedback) {
      builder.recv_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
    }
    // Client sends cancel
    std_msgs::Void void_msg;
    builder.send_message(OPCODE::ACT_CANCEL_MESSAGE, void_msg).close();
    return *this;
  }

  /**
   * @brief Mock an action server that accepts multiple goals via preempt, sends feedback/result.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& send_action_goal_with_preempt(std::vector<std::shared_ptr<TGoal>> goals,
                                             std::vector<std::vector<std::shared_ptr<TFeedback>>> feedbacks,
                                             std::shared_ptr<TResult> result) {
    std::shared_ptr<MockStream> unused;
    return send_action_goal_with_preempt<TGoal, TFeedback, TResult>(goals, feedbacks, result, unused);
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& send_action_goal_with_preempt(std::vector<std::shared_ptr<TGoal>> goals,
                                             std::vector<std::vector<std::shared_ptr<TFeedback>>> feedbacks,
                                             std::shared_ptr<TResult> result,
                                             std::shared_ptr<MockStream>& client) {
    client = transport_manager_.create_stream();
    if (enable_notifications_)
      client_streams_.push_back(client);

    auto builder = StreamBuilder(client);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    sys_msgs::Status status;
    status.error = 0;

    for (size_t i = 0; i < goals.size(); ++i) {
      if (i == 0) {
        builder.send_message(OPCODE::ACT_GOAL_MESSAGE, *goals[i]);
      } else {
        builder.send_message(OPCODE::ACT_PREEMPT_MESSAGE, *goals[i]);
      }
      builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
      for (auto& fb : feedbacks[i]) {
        builder.recv_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
      }
    }
    builder.recv_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Parameter server
  // ---------------------------------------------------------------------------

  TestFixture& set_parameter(const std::string& name,
                             const sys_msgs::NodeInfo& node_info,
                             const std::shared_ptr<Message>& value,
                             bool should_fail = false) {
    sys_msgs::ParamInfo param_info;
    param_info.id = node_info.id;
    param_info.name = name;
    param_info.message_hash = value->hash();
    param_info.data.resize(value->size());
    size_t offset = 0;
    value->serialize(param_info.data.data(), offset);

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = param_info.id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .send_message(OPCODE::PARAM_SET_REQUEST, param_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  TestFixture& get_parameter(const std::string& name,
                             const sys_msgs::NodeInfo& node_info,
                             const std::shared_ptr<Message>& value,
                             bool should_fail = false) {
    sys_msgs::ParamInfo request;
    request.id = node_info.id;
    request.name = name;
    request.message_hash = value->hash();

    sys_msgs::ParamInfo response;
    response.id = request.id;
    response.name = request.name;
    response.message_hash = request.message_hash;
    if (!should_fail) {
      response.data.resize(value->size());
      size_t offset = 0;
      value->serialize(response.data.data(), offset);
    }

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .send_message(OPCODE::PARAM_GET_REQUEST, request)
        .recv_message(OPCODE::PARAM_GET_RESPONSE, response)
        .close();
    return *this;
  }

  TestFixture& get_system_info(const sys_msgs::SystemInfo& info, const sys_msgs::NodeInfo& node_info) {
    std_msgs::UInt64 id;
    id.data = node_info.id;
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .send_message(OPCODE::SYSTEM_GET_REQUEST, id)
        .recv_message(OPCODE::SYSTEM_GET_RESPONSE, info)
        .close();
    return *this;
  }

private:
  Endpoint rixhub_endpoint_;
  TransportManager transport_manager_;
  bool enable_notifications_;
  bool enable_poller_;
  bool enable_clock_;
  uint64_t expected_id_;
  uint64_t current_id_;

  // Categorized mocks for synchronization
  std::vector<std::shared_ptr<MockAcceptor>> acceptors_;
  std::vector<std::shared_ptr<MockStream>> connection_streams_;
  std::vector<std::shared_ptr<MockStream>> client_streams_;

  /**
   * @brief Internal helper to create a server (MockAcceptor) and its accept()
   * streams. The AcceptorBuilder is used to wire up the mock expectations.
   */
  void create_server(const Endpoint& bound_endpoint, int accept_count, int iters_between_accept = 0) {
    auto acceptor = transport_manager_.create_acceptor();
    if (enable_notifications_)
      acceptors_.push_back(acceptor);

    auto builder = AcceptorBuilder(acceptor);
    if (enable_notifications_)
      builder.enable_operation_notifications();

    // The accept() calls return streams from the TransportManager.
    // Those streams are created later by accept_subscriber,
    // accept_notification, etc.
    auto factory = transport_manager_.get_factory();
    builder.as_server(
        bound_endpoint,
        accept_count,
        [factory]() { return factory.create_stream(Endpoint{}, true); },
        iters_between_accept);
  }
};

} // namespace rix
