#pragma once

#include "rix/core/node.hpp"
#include "rix/test/mock_poller.hpp"
#include "socket_builder.hpp"
#include "socket_manager.hpp"
#include <gtest/gtest.h>

namespace rix {

// TODO: Revise the functions for waiting on operations, should instead be
// "wait_for_publisher_accept" or "wait_for_subscriber_connect". The NodeTestFixture
// should not communicate details at the socket level, but rather at the higher level of
// publishers and subscribers.

// High-level test fixture for Node tests
class NodeTestFixture : std::enable_shared_from_this<NodeTestFixture> {
public:
  NodeTestFixture() : rixhub_endpoint_(RIXHUB_IP, RIXHUB_PORT), enable_notifications_(false), enable_poller_(false), enable_debug_clock_(false) {
    GenericSocket::set_poller(nullptr);
  }

  template <typename TNode> using TestFunction = std::function<void(NodeTestFixture&, std::unique_ptr<TNode>)>;

  // Build and return the configured node
  template <typename TNode = Node, typename... Args> void build(TestFunction<TNode> test_func, Args&&... args) {
    static_assert(std::is_base_of<Node, TNode>::value, "TNode must be Node or derived from Node");
    TNode::set_socket_factory(socket_manager_.get_factory());
    auto node = std::make_unique<TNode>(std::forward<Args>(args)...);
    test_func(*this, std::move(node));
  }

  ~NodeTestFixture() {
    if (enable_poller_) {
      rix::GenericSocket::set_poller(nullptr);
    }
    if (enable_debug_clock_) {
      rix::Time::set_clock(std::make_shared<Clock>());
    }
  }

  NodeTestFixture& enable_debug_clock() {
    if (enable_debug_clock_) {
      throw std::runtime_error("Debug clock already enabled");
    }
    rix::Time::set_clock(std::make_shared<MockClock>());
    return *this;
  }

  NodeTestFixture& enable_poller(int max_poll_count = -1) {
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
  NodeTestFixture& enable_operation_notifications() {
    if (enable_notifications_) {
      throw std::runtime_error("Operation notifications already enabled");
    }
    enable_notifications_ = true;
    return *this;
  }

  // Disable operation notifications
  NodeTestFixture& disable_operation_notifications() {
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
  NodeTestFixture& register_node(bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_node_register(rixhub_endpoint_, should_fail);
    return *this;
  }

  // Configure node deregistration
  NodeTestFixture& deregister_node() {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_node_deregister(rixhub_endpoint_);
    return *this;
  }

  // Configure publisher registration
  template <typename TMsg>
  NodeTestFixture& register_publisher(const std::string& topic,
                                      bool should_fail = false,
                                      int subscriber_count = 0,
                                      const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                      const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    // Create the server socket for publisher connections
    create_server(endpoint, bound_endpoint, subscriber_count);

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::PubInfo pub_info;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = TMsg().hash();
    pub_info.endpoint.address = bound_endpoint.address;
    pub_info.endpoint.port = bound_endpoint.port;
    SocketBuilder(reg_socket).as_pub_register(rixhub_endpoint_, pub_info, should_fail);

    return *this;
  }

  // Configure publisher deregistration
  template <typename TMsg>
  NodeTestFixture& deregister_publisher(const std::string& topic,
                                        const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    auto socket = socket_manager_.create_socket();
    msg::mediator::PubInfo pub_info;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = TMsg().hash();
    pub_info.endpoint.address = bound_endpoint.address;
    pub_info.endpoint.port = bound_endpoint.port;
    SocketBuilder(socket).as_pub_deregister(rixhub_endpoint_, pub_info);
    return *this;
  }

  // Configure subscriber registration
  template <typename TMsg>
  NodeTestFixture& register_subscriber(const std::string& topic,
                                       bool should_fail = false,
                                       int notification_count = 0,
                                       const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                       const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    // Create the client socket for subscriber connections
    create_server(endpoint, bound_endpoint, notification_count);

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SubInfo sub_info;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = TMsg().hash();
    sub_info.endpoint.address = bound_endpoint.address;
    sub_info.endpoint.port = bound_endpoint.port;
    SocketBuilder(reg_socket).as_sub_register(rixhub_endpoint_, sub_info, should_fail);

    return *this;
  }

  // Configure subscriber deregistration
  template <typename TMsg>
  NodeTestFixture& deregister_subscriber(const std::string& topic,
                                         const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be derived from msg::Message");

    auto socket = socket_manager_.create_socket();
    msg::mediator::SubInfo sub_info;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = TMsg().hash();
    sub_info.endpoint.address = bound_endpoint.address;
    sub_info.endpoint.port = bound_endpoint.port;
    SocketBuilder(socket).as_sub_deregister(rixhub_endpoint_, sub_info);
    return *this;
  }

  // Configure service registration
  template <typename TReq, typename TRes>
  NodeTestFixture& register_service(const std::string& service,
                                    bool should_fail = false,
                                    int client_count = 0,
                                    const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                    const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    static_assert(std::is_base_of<msg::Message, TReq>::value, "TReq must be derived from msg::Message");
    static_assert(std::is_base_of<msg::Message, TRes>::value, "TRes must be derived from msg::Message");
    // Create the server socket for service connections
    create_server(endpoint, bound_endpoint, client_count);

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SrvInfo srv_info;
    srv_info.name = service;
    srv_info.request_hash = TReq().hash();
    srv_info.response_hash = TRes().hash();
    srv_info.endpoint.address = bound_endpoint.address;
    srv_info.endpoint.port = bound_endpoint.port;
    SocketBuilder(reg_socket).as_srv_register(rixhub_endpoint_, srv_info, should_fail);

    return *this;
  }

  // Configure service deregistration
  template <typename TReq, typename TRes>
  NodeTestFixture& deregister_service(const std::string& service,
                                      const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::SrvInfo srv_info;
    srv_info.name = service;
    srv_info.request_hash = TReq().hash();
    srv_info.response_hash = TRes().hash();
    srv_info.endpoint.address = bound_endpoint.address;
    srv_info.endpoint.port = bound_endpoint.port;
    SocketBuilder(socket).as_srv_deregister(rixhub_endpoint_, srv_info);
    return *this;
  }

  // Configure service client request
  template <typename TReq, typename TRes>
  NodeTestFixture& request_service_client(const std::string& service,
                                          bool should_fail = false,
                                          const Endpoint& service_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SrvRequest srv_req;
    srv_req.name = service;
    srv_req.request_hash = TReq().hash();
    srv_req.response_hash = TRes().hash();
    msg::mediator::SrvResponse srv_res;
    srv_res.srv_info.name = service;
    srv_res.srv_info.request_hash = TReq().hash();
    srv_res.srv_info.response_hash = TRes().hash();
    srv_res.srv_info.endpoint.address = service_endpoint.address;
    srv_res.srv_info.endpoint.port = service_endpoint.port;
    SocketBuilder(reg_socket).as_srv_cli_request(rixhub_endpoint_, srv_req, srv_res, should_fail);
    return *this;
  }

  // Configure parameter set request
  NodeTestFixture&
  request_parameter_set(const std::string& name, std::shared_ptr<msg::Message> value, bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::ParamInfo param_info;
    param_info.name = name;
    param_info.message_hash = value->hash();
    param_info.data.resize(value->size());
    size_t offset = 0;
    value->serialize(param_info.data.data(), offset);
    SocketBuilder(socket).as_param_set(rixhub_endpoint_, param_info, should_fail);
    return *this;
  }

  // Configure parameter get request
  NodeTestFixture&
  request_parameter_get(const std::string& name, std::shared_ptr<msg::Message> value, bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::ParamInfo param_info;
    param_info.name = name;
    param_info.message_hash = value->hash();
    param_info.data.resize(value->size());
    size_t offset = 0;
    value->serialize(param_info.data.data(), offset);
    SocketBuilder(socket).as_param_get(rixhub_endpoint_, param_info, should_fail);
    return *this;
  }

  // Configure system info get request
  NodeTestFixture& request_system_info(msg::mediator::SystemInfo info, bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_sys_info_request(rixhub_endpoint_, info, should_fail);
    return *this;
  }

  // Configure publisher server with connection sockets
  template <typename TMsg> NodeTestFixture& create_pub_connection(const std::vector<std::shared_ptr<TMsg>>& messages) {
    // Create connection sockets
    auto conn_socket = socket_manager_.create_socket();
    connection_sockets_.push_back(conn_socket);
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_pub_connection(messages);
    return *this;
  }

  // Configure subscriber connections
  template <typename TMsg>
  NodeTestFixture& create_sub_connection(const std::string& topic, const std::vector<Endpoint>& publisher_endpoints) {
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
    connection_sockets_.push_back(notify_socket);
    auto builder = SocketBuilder(notify_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_sub_connection(sub_notify);
    return *this;
  }

  // Configure subscriber clients
  template <typename TMsg>
  NodeTestFixture& create_sub_client(const Endpoint& publisher_endpoint,
                                     const std::vector<std::shared_ptr<TMsg>>& messages) {
    auto client_socket = socket_manager_.create_socket();
    client_sockets_.push_back(client_socket);
    auto builder = SocketBuilder(client_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_sub_client(publisher_endpoint, messages);
    return *this;
  }

  // Configure service server with connection sockets
  template <typename TRequest, typename TResponse>
  NodeTestFixture& create_srv_connection(std::shared_ptr<TRequest> request, std::shared_ptr<TResponse> response) {
    auto conn_socket = socket_manager_.create_socket();
    connection_sockets_.push_back(conn_socket);
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_srv_connection(request, response);
    return *this;
  }

  // Configure service client client
  template <typename TRequest, typename TResponse>
  NodeTestFixture& create_srv_cli_client(std::shared_ptr<TRequest> request,
                                         std::shared_ptr<TResponse> response,
                                         const Endpoint& service_endpoint = Endpoint(DEFAULT_IP, 8000)) {
    auto socket = socket_manager_.create_socket();
    client_sockets_.push_back(socket);
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_srv_cli_client(service_endpoint, request, response);
    return *this;
  }

private:
  SocketManager socket_manager_;
  Endpoint rixhub_endpoint_;
  bool enable_notifications_;
  bool enable_poller_;
  bool enable_debug_clock_;

  // Categorized socket tracking
  std::vector<std::shared_ptr<MockSocket>> server_sockets_;     // Server sockets (accept connections)
  std::vector<std::shared_ptr<MockSocket>> connection_sockets_; // Connection sockets (pub/srv connections)
  std::vector<std::shared_ptr<MockSocket>> client_sockets_;     // Client sockets (sub/srv clients)

  // Configure server
  NodeTestFixture& create_server(const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                 const Endpoint& bound_endpoint = Endpoint(DEFAULT_IP, 8000),
                                 int accept_count = 0) {
    auto socket = socket_manager_.create_socket();
    server_sockets_.push_back(socket);
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_server(
        endpoint, bound_endpoint, [this]() { return this->socket_manager_.get_factory()(); }, accept_count);
    return *this;
  }
};

} // namespace rix