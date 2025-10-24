#pragma once

#include "rix/core/node.hpp"
#include "rix/test/mock_clock.hpp"
#include "rix/test/mock_poller.hpp"
#include "socket_builder.hpp"
#include "socket_manager.hpp"
#include <gtest/gtest.h>
#include <rix/msg/mediator/ActResponse.hpp>
#include <rix/msg/mediator/SrvResponse.hpp>
#include <rix/msg/mediator/SubNotify.hpp>
#include <rix/msg/standard/UInt64.hpp>
#include <rix/msg/standard/Void.hpp>

namespace rix {

// TODO: Revise the functions for waiting on operations, should instead be
// "wait_for_publisher_accept" or "wait_for_subscriber_connect". The TestFixture
// should not communicate details at the socket level, but rather at the higher level of
// publishers and subscribers.

// High-level test fixture for Node tests
class TestFixture : std::enable_shared_from_this<TestFixture> {
public:
  TestFixture()
      : rixhub_endpoint_(RIXHUB_IP, RIXHUB_PORT), enable_notifications_(false), enable_poller_(false),
        enable_clock_(false), expected_id_(1), current_id_(0) {
    GenericSocket::set_poller(nullptr);
  }

  template <typename TNode> using TestFunction = std::function<void(TestFixture&)>;

  // Build and return the configured node
  template <typename TNode = Node> void build(TestFunction<TNode> test_func) {
    static_assert(std::is_base_of_v<Node, TNode>, "TNode must be Node or derived from Node");
    TNode::set_socket_factory(socket_manager_.get_factory());
    TNode::set_id_factory([this]() { return ++current_id_; });
    test_func(*this);
  }

  ~TestFixture() {
    if (enable_poller_) {
      rix::GenericSocket::set_poller(nullptr);
    }
    if (enable_clock_) {
      rix::Time::set_clock(std::make_shared<Clock>());
    }
  }

  TestFixture& enable_clock(std::shared_ptr<MockClock>& clock) {
    if (enable_clock_) {
      throw std::runtime_error("Clock already enabled");
    }
    clock = std::make_shared<MockClock>();
    rix::Time::set_clock(clock);
    enable_clock_ = true;
    return *this;
  }

  TestFixture& enable_poller(int max_poll_count = -1) {
    if (enable_poller_) {
      throw std::runtime_error("Poller already enabled");
    }
    enable_poller_ = true;
    auto poller = std::make_shared<rix::MockPoller>(max_poll_count);
    rix::GenericSocket::set_poller(poller);
    EXPECT_CALL(*poller, poll).Times(::testing::AtLeast(1));
    return *this;
  }

  // Enable operation notifications for synchronization in multithreaded tests
  TestFixture& enable_operation_notifications() {
    if (enable_notifications_) {
      throw std::runtime_error("Operation notifications already enabled");
    }
    enable_notifications_ = true;
    return *this;
  }

  // Disable operation notifications
  TestFixture& disable_operation_notifications() {
    if (!enable_notifications_) {
      throw std::runtime_error("Operation notifications not enabled");
    }
    enable_notifications_ = false;
    return *this;
  }

  // Get a specific server socket by index (0-based)
  std::shared_ptr<MockSocket> get_server_socket(size_t index = 0) const {
    if (index < server_sockets_.size()) {
      return server_sockets_[index];
    }
    return nullptr;
  }

  // Get a specific connection socket by index (0-based)
  std::shared_ptr<MockSocket> get_connection_socket(size_t index = 0) const {
    if (index < connection_sockets_.size()) {
      return connection_sockets_[index];
    }
    return nullptr;
  }

  // Get a specific client socket by index (0-based)
  std::shared_ptr<MockSocket> get_client_socket(size_t index = 0) const {
    if (index < client_sockets_.size()) {
      return client_sockets_[index];
    }
    return nullptr;
  }

  // Get all server sockets
  const std::vector<std::shared_ptr<MockSocket>>& get_server_sockets() const { return server_sockets_; }

  // Get connection sockets (pub connections, service connections)
  const std::vector<std::shared_ptr<MockSocket>>& get_connection_sockets() const { return connection_sockets_; }

  // Get client sockets (subscriber clients, service clients)
  const std::vector<std::shared_ptr<MockSocket>>& get_client_sockets() const { return client_sockets_; }

  // Wait for all server sockets to complete the specified number of operations
  bool wait_for_all_servers(size_t operations_per_server,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& server : server_sockets_) {
      if (!server->wait_for_operations(operations_per_server, timeout)) {
        return false;
      }
    }
    return true;
  }

  // Wait for all connection sockets to complete the specified number of operations
  bool wait_for_all_connections(size_t operations_per_connection,
                                std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& conn : connection_sockets_) {
      if (!conn->wait_for_operations(operations_per_connection, timeout)) {
        return false;
      }
    }
    return true;
  }

  // Wait for all client sockets to complete the specified number of operations
  bool wait_for_all_clients(size_t operations_per_client,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& client : client_sockets_) {
      if (!client->wait_for_operations(operations_per_client, timeout)) {
        return false;
      }
    }
    return true;
  }

  // Configure node registration to succeed
  TestFixture& create_node(const std::string& name,
                           msg::mediator::NodeInfo& node_info,
                           bool should_fail = false,
                           int ping_count = 0,
                           const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                           const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    // Create the server socket for publisher connections
    create_server(endpoint, bound_endpoint, ping_count);

    node_info.id = expected_id_++;
    node_info.name = name;
    node_info.endpoint.address = bound_endpoint.address;
    node_info.endpoint.port = bound_endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = node_info.id;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::NODE_REGISTER, node_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // Configure node deregistration
  TestFixture& destroy_node(const msg::mediator::NodeInfo& node_info) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).connect(rixhub_endpoint_).send_message(OPCODE::NODE_DEREGISTER, node_info).close();
    return *this;
  }

  // Configure publisher registration
  template <typename TMsg>
  TestFixture& create_publisher(const std::string& topic,
                                msg::mediator::PubInfo& pub_info,
                                const msg::mediator::NodeInfo& node_info,
                                bool should_fail = false,
                                int subscriber_count = 0,
                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<msg::Message, TMsg>, "TMsg must be derived from msg::Message");

    // Create the server socket for publisher connections
    create_server(endpoint, bound_endpoint, subscriber_count);

    pub_info.node_id = node_info.id;
    pub_info.id = expected_id_++;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = TMsg().hash();
    pub_info.endpoint.address = bound_endpoint.address;
    pub_info.endpoint.port = bound_endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = pub_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::PUB_REGISTER, pub_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();

    return *this;
  }

  // Configure publisher deregistration
  TestFixture& destroy_publisher(const msg::mediator::PubInfo& pub_info) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).connect(rixhub_endpoint_).send_message(OPCODE::PUB_DEREGISTER, pub_info).close();
    return *this;
  }

  // Configure subscriber registration
  template <typename TMsg>
  TestFixture& create_subscriber(const std::string& topic,
                                 msg::mediator::SubInfo& sub_info,
                                 const msg::mediator::NodeInfo& node_info,
                                 bool should_fail = false,
                                 int notification_count = 0,
                                 const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                 const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<msg::Message, TMsg>, "TMsg must be derived from msg::Message");

    // Create the client socket for subscriber connections
    create_server(endpoint, bound_endpoint, notification_count);

    sub_info.node_id = node_info.id;
    sub_info.id = expected_id_++;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = TMsg().hash();
    sub_info.endpoint.address = bound_endpoint.address;
    sub_info.endpoint.port = bound_endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = sub_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::SUB_REGISTER, sub_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();

    return *this;
  }

  // Configure subscriber deregistration
  TestFixture& destroy_subscriber(const msg::mediator::SubInfo& sub_info) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).connect(rixhub_endpoint_).send_message(OPCODE::SUB_DEREGISTER, sub_info).close();
    return *this;
  }

  // Configure service registration
  template <typename TReq, typename TRes>
  TestFixture& create_service(const std::string& service,
                              msg::mediator::SrvInfo& srv_info,
                              const msg::mediator::NodeInfo& node_info,
                              bool should_fail = false,
                              int client_count = 0,
                              const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                              const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<msg::Message, TReq>, "TReq must be derived from msg::Message");
    static_assert(std::is_base_of_v<msg::Message, TRes>, "TRes must be derived from msg::Message");
    // Create the server socket for service connections
    create_server(endpoint, bound_endpoint, client_count);

    srv_info.node_id = node_info.id;
    srv_info.id = expected_id_++;
    srv_info.name = service;
    srv_info.request_hash = TReq().hash();
    srv_info.response_hash = TRes().hash();
    srv_info.endpoint.address = bound_endpoint.address;
    srv_info.endpoint.port = bound_endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = srv_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::SRV_REGISTER, srv_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();

    return *this;
  }

  // Configure service deregistration
  TestFixture& destroy_service(const msg::mediator::SrvInfo& srv_info) {
    const auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).connect(rixhub_endpoint_).send_message(OPCODE::SRV_DEREGISTER, srv_info).close();
    return *this;
  }

  // Configure action registration
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& create_action(const std::string& action,
                             msg::mediator::ActInfo& act_info,
                             const msg::mediator::NodeInfo& node_info,
                             int iters_between_accept = 0,
                             bool should_fail = false,
                             int client_count = 0,
                             const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                             const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of_v<msg::Message, TGoal>, "TGoal must be derived from msg::Message");
    static_assert(std::is_base_of_v<msg::Message, TFeedback>, "TFeedback must be derived from msg::Message");
    static_assert(std::is_base_of_v<msg::Message, TResult>, "TResult must be derived from msg::Message");
    // Create the server socket for action connections
    create_server(endpoint, bound_endpoint, client_count, iters_between_accept);

    act_info.node_id = node_info.id;
    act_info.id = expected_id_++;
    act_info.name = action;
    act_info.endpoint.address = bound_endpoint.address;
    act_info.endpoint.port = bound_endpoint.port;
    act_info.goal_hash = TGoal().hash();
    act_info.feedback_hash = TFeedback().hash();
    act_info.result_hash = TResult().hash();

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = act_info.id;
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::ACT_REGISTER, act_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // Configure action deregistration
  TestFixture& destroy_action(const msg::mediator::ActInfo& act_info) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).connect(rixhub_endpoint_).send_message(OPCODE::ACT_DEREGISTER, act_info).close();
    return *this;
  }

  // Configure service client request
  template <typename TReq, typename TRes>
  TestFixture& create_service_client(const std::string& service,
                                     const msg::mediator::NodeInfo& node_info,
                                     bool should_fail = false,
                                     const Endpoint& service_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    msg::mediator::SrvRequest srv_req;
    srv_req.node_id = node_info.id;
    srv_req.name = service;
    srv_req.request_hash = TReq().hash();
    srv_req.response_hash = TRes().hash();

    msg::mediator::SrvResponse srv_res;
    srv_res.error = should_fail ? -1 : 0;
    if (!should_fail) {
      srv_res.srv_info.name = service;
      srv_res.srv_info.request_hash = TReq().hash();
      srv_res.srv_info.response_hash = TRes().hash();
      srv_res.srv_info.endpoint.address = service_endpoint.address;
      srv_res.srv_info.endpoint.port = service_endpoint.port;
    }

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::SRV_REQUEST, srv_req)
        .recv_message(OPCODE::SRV_RESPONSE, srv_res)
        .close();
    return *this;
  }

  // Configure action client request
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& create_action_client(const std::string& action,
                                    const msg::mediator::NodeInfo& node_info,
                                    bool should_fail = false,
                                    const Endpoint& action_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    msg::mediator::ActRequest act_req;
    act_req.node_id = node_info.id;
    act_req.name = action;
    act_req.goal_hash = TGoal().hash();
    act_req.feedback_hash = TFeedback().hash();
    act_req.result_hash = TResult().hash();

    msg::mediator::ActResponse act_res;
    act_res.error = should_fail ? -1 : 0;
    if (!should_fail) {
      act_res.act_info.name = action;
      act_res.act_info.goal_hash = TGoal().hash();
      act_res.act_info.feedback_hash = TFeedback().hash();
      act_res.act_info.result_hash = TResult().hash();
      act_res.act_info.endpoint.address = action_endpoint.address;
      act_res.act_info.endpoint.port = action_endpoint.port;
    }

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::ACT_REQUEST, act_req)
        .recv_message(OPCODE::ACT_RESPONSE, act_res)
        .close();
    return *this;
  }

  // Configure parameter set request
  TestFixture& set_parameter(const std::string& name,
                             const msg::mediator::NodeInfo& node_info,
                             const std::shared_ptr<msg::Message>& value,
                             bool should_fail = false) {
    msg::mediator::ParamInfo param_info;
    param_info.id = node_info.id;
    param_info.name = name;
    param_info.message_hash = value->hash();
    param_info.data.resize(value->size());
    size_t offset = 0;
    value->serialize(param_info.data.data(), offset);

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = param_info.id;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::PARAM_SET_REQUEST, param_info)
        .recv_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // Configure parameter get request
  TestFixture& get_parameter(const std::string& name,
                             const msg::mediator::NodeInfo& node_info,
                             const std::shared_ptr<msg::Message>& value,
                             bool should_fail = false) {
    msg::mediator::ParamInfo request;
    request.id = node_info.id;
    request.name = name;
    request.message_hash = value->hash();

    msg::mediator::ParamInfo response;
    response.id = request.id;
    response.name = request.name;
    response.message_hash = request.message_hash;
    if (!should_fail) {
      response.data.resize(value->size());
      size_t offset = 0;
      value->serialize(response.data.data(), offset);
    }

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::PARAM_GET_REQUEST, request)
        .recv_message(OPCODE::PARAM_GET_RESPONSE, response)
        .close();
    return *this;
  }

  // Configure system info get request
  TestFixture& get_system_info(const msg::mediator::SystemInfo& info, const msg::mediator::NodeInfo& node_info) {
    msg::standard::UInt64 id;
    id.data = node_info.id;
    const auto socket = socket_manager_.create_socket();
    SocketBuilder(socket)
        .connect(rixhub_endpoint_)
        .send_message(OPCODE::SYSTEM_GET_REQUEST, id)
        .recv_message(OPCODE::SYSTEM_GET_RESPONSE, info)
        .close();
    return *this;
  }

  TestFixture& accept_ping() {
    auto conn_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(conn_socket);
    }
    msg::mediator::Operation operation;
    operation.opcode = OPCODE::PING;
    operation.len = 0;

    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.recv_message(operation, operation.size()).close();
    return *this;
  }

  // Configure publisher server with connection sockets
  template <typename TMsg> TestFixture& accept_subscriber(const std::vector<std::shared_ptr<TMsg>>& messages) {
    // Create connection sockets
    auto conn_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(conn_socket);
    }
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    for (auto& msg : messages) {
      builder.send_message(OPCODE::PUB_MESSAGE, *msg);
    }
    builder.close();
    return *this;
  }

  // Configure subscriber connections
  template <typename TMsg>
  TestFixture& accept_notification(const std::string& topic, const std::vector<Endpoint>& publisher_endpoints) {
    msg::mediator::SubNotify sub_notify;
    sub_notify.publishers.resize(publisher_endpoints.size());
    for (size_t i = 0; i < publisher_endpoints.size(); i++) {
      sub_notify.publishers[i].topic_info.name = topic;
      sub_notify.publishers[i].topic_info.message_hash = TMsg().hash();
      sub_notify.publishers[i].endpoint.address = publisher_endpoints[i].address;
      sub_notify.publishers[i].endpoint.port = publisher_endpoints[i].port;
      sub_notify.publishers[i].id = static_cast<uint32_t>(i + 1);
    }

    // Create notification connection socket
    auto notify_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(notify_socket);
    }
    auto builder = SocketBuilder(notify_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.recv_message(OPCODE::SUB_NOTIFY, sub_notify).close();
    return *this;
  }

  // Configure subscriber clients
  template <typename TMsg>
  TestFixture& connect_to_publisher(const Endpoint& publisher_endpoint,
                                    const std::vector<std::shared_ptr<TMsg>>& messages) {
    auto client_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      client_sockets_.push_back(client_socket);
    }
    auto builder = SocketBuilder(client_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.set_blocking(false).connect(publisher_endpoint);
    for (auto& msg : messages) {
      builder.recv_message(OPCODE::PUB_MESSAGE, *msg);
    }
    builder.close();
    return *this;
  }

  // Configure service server with connection sockets
  template <typename TRequest, typename TResponse>
  TestFixture& accept_service_client(std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    auto conn_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(conn_socket);
    }
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.recv_message(OPCODE::SRV_REQUEST_MESSAGE, *request)
        .send_message(OPCODE::SRV_RESPONSE_MESSAGE, *response)
        .close();
    return *this;
  }

  // Configure action server with connection sockets
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client(std::shared_ptr<TGoal> goal,
                                    std::vector<std::shared_ptr<TFeedback>> feedback,
                                    std::shared_ptr<TResult> result,
                                    bool should_fail = false) {
    auto conn_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(conn_socket);
    }
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    msg::mediator::Operation operation;
    operation.opcode = OPCODE::ACT_GOAL_MESSAGE;
    operation.len = goal->size();
    builder.recv_message(operation, operation.size());
    msg::mediator::Status status;
    if (should_fail) {
      // Immediately send result with error
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

  template <typename TGoal, typename TFeedback>
  TestFixture& accept_action_client_with_cancel(std::shared_ptr<TGoal> goal,
                                                std::vector<std::shared_ptr<TFeedback>> feedback,
                                                bool should_fail = false) {
    auto conn_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(conn_socket);
    }
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    msg::mediator::Operation operation;
    operation.opcode = OPCODE::ACT_GOAL_MESSAGE;
    operation.len = goal->size();
    builder.recv_message(operation, operation.size());
    msg::mediator::Status status;
    if (should_fail) {
      // Immediately send result with error
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
    operation.opcode = OPCODE::ACT_CANCEL_MESSAGE;
    operation.len = 0;
    builder.recv_message(operation, operation.size()).close();
    return *this;
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& accept_action_client_with_preempt(std::vector<std::shared_ptr<TGoal>> goals,
                                                 std::vector<std::vector<std::shared_ptr<TFeedback>>> feedbacks,
                                                 std::shared_ptr<TResult> result,
                                                 bool should_fail = false) {
    if (goals.size() != feedbacks.size()) {
      throw std::runtime_error("Goals and feedbacks size mismatch");
    }
    if (goals.size() < 2) {
      throw std::runtime_error("At least two goals required for preempt test");
    }
    auto conn_socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      connection_sockets_.push_back(conn_socket);
    }
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    msg::mediator::Operation operation;
    operation.opcode = OPCODE::ACT_GOAL_MESSAGE;
    operation.len = goals[0]->size();
    builder.recv_message(operation, operation.size());
    msg::mediator::Status status;
    if (should_fail) {
      // Immediately send result with error
      status.error = -1;
      builder.send_message(OPCODE::ACT_RESPONSE_MESSAGE, status).close();
      return *this;
    } else {
      status.error = 0;
      // First set of goal/feedbacks
      builder.recv_message(*goals[0], goals[0]->size()).send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
      for (size_t i = 0; i < feedbacks[0].size(); i++) {
        builder.send_message(OPCODE::ACT_FEEDBACK_MESSAGE, *feedbacks[0][i]);
      }
    }
    // Preempt with other goals
    for (size_t g = 1; g < goals.size(); g++) {
      // Preempt message
      operation.opcode = OPCODE::ACT_PREEMPT_MESSAGE;
      operation.len = goals[g]->size();
      builder.recv_message(operation, operation.size());
      builder.recv_message(*goals[g], goals[g]->size()).send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
      for (size_t i = 0; i < feedbacks[g].size(); i++) {
        builder.send_message(OPCODE::ACT_FEEDBACK_MESSAGE, *feedbacks[g][i]);
      }
    }
    builder.send_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

  // Configure service client
  template <typename TRequest, typename TResponse>
  TestFixture& call_service_client(std::shared_ptr<TRequest> request,
                                   std::shared_ptr<TResponse> response,
                                   const Endpoint& service_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    auto socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      client_sockets_.push_back(socket);
    }
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.connect(service_endpoint)
        .send_message(OPCODE::SRV_REQUEST_MESSAGE, *request)
        .recv_message(OPCODE::SRV_RESPONSE_MESSAGE, *response)
        .close();
    return *this;
  }

  // Configure action client
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& send_action_goal(std::shared_ptr<TGoal> goal,
                                std::vector<std::shared_ptr<TFeedback>> feedback,
                                std::shared_ptr<TResult> result,
                                bool should_fail = false,
                                const Endpoint& action_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    auto socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      client_sockets_.push_back(socket);
    }
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.connect(action_endpoint).send_message(OPCODE::ACT_GOAL_MESSAGE, *goal);

    msg::mediator::Status status;
    if (should_fail) {
      status.error = -1;
      builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status).close();
      return *this;
    } else {
      status.error = 0;
      builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    }

    for (auto& fb : feedback) {
      builder.recv_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
    }

    builder.recv_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

  // Configure action client
  template <typename TGoal, typename TFeedback>
  TestFixture& send_action_goal_with_cancel(std::shared_ptr<TGoal> goal,
                                std::vector<std::shared_ptr<TFeedback>> feedback,
                                bool should_fail = false,
                                const Endpoint& action_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    auto socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      client_sockets_.push_back(socket);
    }
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.connect(action_endpoint).send_message(OPCODE::ACT_GOAL_MESSAGE, *goal);

    msg::mediator::Status status;
    if (should_fail) {
      status.error = -1;
      builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status).close();
      return *this;
    } else {
      status.error = 0;
      builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    }

    for (auto& fb : feedback) {
      builder.recv_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
    }
    msg::standard::Void cancel_msg;
    builder.send_message(OPCODE::ACT_CANCEL_MESSAGE, cancel_msg).close();
    return *this;
  }

  // Configure action client
  template <typename TGoal, typename TFeedback, typename TResult>
  TestFixture& send_action_goal_with_preempt(std::vector<std::shared_ptr<TGoal>> goals,
                                             std::vector<std::vector<std::shared_ptr<TFeedback>>> feedback,
                                             std::shared_ptr<TResult> result,
                                             bool should_fail = false,
                                             const Endpoint& action_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    if (goals.size() != feedback.size()) {
      throw std::runtime_error("Goals and feedback size mismatch");
    }
    if (goals.size() < 2) {
      throw std::runtime_error("At least two goals required for preempt test");
    }
    auto socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      client_sockets_.push_back(socket);
    }
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.connect(action_endpoint);

    for (size_t i = 0; i < goals.size(); i++) {
      if (i == 0) {
        // First goal
        builder.send_message(OPCODE::ACT_GOAL_MESSAGE, *goals[i]);
      } else {
        // Preempt message
        builder.send_message(OPCODE::ACT_PREEMPT_MESSAGE, *goals[i]);
      }

      msg::mediator::Status status;
      if (should_fail) {
        status.error = -1;
        builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status).close();
        return *this;
      } else {
        status.error = 0;
        builder.recv_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
      }
      for (auto& fb : feedback[i]) {
        builder.recv_message(OPCODE::ACT_FEEDBACK_MESSAGE, *fb);
      }
    }

    builder.recv_message(OPCODE::ACT_RESULT_MESSAGE, *result).close();
    return *this;
  }

private:
  SocketManager socket_manager_;
  Endpoint rixhub_endpoint_;
  bool enable_notifications_;
  bool enable_poller_;
  bool enable_clock_;
  uint64_t expected_id_;
  uint64_t current_id_;

  // Categorized socket tracking
  std::vector<std::shared_ptr<MockSocket>> server_sockets_;     // Server sockets
  std::vector<std::shared_ptr<MockSocket>> connection_sockets_; // Connection sockets (pub/sub/srv connections)
  std::vector<std::shared_ptr<MockSocket>> client_sockets_;     // Client sockets (sub/srvcli clients)

  // Configure server
  TestFixture& create_server(const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                             const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000),
                             int accept_count = 0,
                             int iters_between_accept = 0) {
    auto socket = socket_manager_.create_socket();
    if (enable_notifications_) {
      server_sockets_.push_back(socket);
    }
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_server(
        endpoint,
        bound_endpoint,
        [this]() { return this->socket_manager_.get_factory()(); },
        accept_count,
        iters_between_accept);
    return *this;
  }
};

} // namespace rix