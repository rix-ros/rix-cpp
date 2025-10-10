#pragma once

#include "rix/core/node.hpp"
#include "rix/ipc/mock_poller.hpp"
#include "socket_builder.hpp"
#include "socket_manager.hpp"
#include <gtest/gtest.h>

namespace rix {

// High-level test fixture for Node tests
template <typename Node = rix::Node> class NodeTestFixture {
public:
  static void enable_poller(int max_poll_count = -1) {
    auto poller = std::make_shared<rix::MockPoller>(max_poll_count);
    rix::GenericSocket::set_poller(poller);
    EXPECT_CALL(*poller, poll).Times(::testing::AtLeast(1));
  }

  static void disable_poller() { rix::GenericSocket::set_poller(nullptr); }

  NodeTestFixture(const std::string& node_name = "test_node",
                  const Endpoint& rixhub = Endpoint("127.0.0.1", 8000))
      : node_name_(node_name), rixhub_endpoint_(rixhub), node_id_(0),
        enable_notifications_(false) {}

  ~NodeTestFixture() {}

  // Enable operation notifications for synchronization in multithreaded tests
  NodeTestFixture& enable_operation_notifications() {
    enable_notifications_ = true;
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
  const std::vector<std::shared_ptr<MockSocket>>& get_server_sockets() const {
    return server_sockets_;
  }

  // Get connection sockets (pub connections, service connections)
  const std::vector<std::shared_ptr<MockSocket>>& get_connection_sockets() const {
    return connection_sockets_;
  }

  // Get client sockets (subscriber clients, service clients)
  const std::vector<std::shared_ptr<MockSocket>>& get_client_sockets() const {
    return client_sockets_;
  }

  // Convenience methods for waiting on multiple sockets

  // Wait for all server sockets to complete the specified number of operations
  bool wait_for_all_servers(
      size_t operations_per_server,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& server : server_sockets_) {
      if (!server->wait_for_operations(operations_per_server, timeout)) {
        return false;
      }
    }
    return true;
  }

  // Wait for all connection sockets to complete the specified number of operations
  bool wait_for_all_connections(
      size_t operations_per_connection,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) const {
    for (auto& conn : connection_sockets_) {
      if (!conn->wait_for_operations(operations_per_connection, timeout)) {
        return false;
      }
    }
    return true;
  }

  // Wait for all client sockets to complete the specified number of operations
  bool wait_for_all_clients(
      size_t operations_per_client,
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
    SocketBuilder(socket).as_node_register(
        rixhub_endpoint_, make_node_info(), should_fail);
    return *this;
  }

  // Configure node deregistration
  NodeTestFixture& deregister_node() {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_node_deregister(rixhub_endpoint_, make_node_info());
    return *this;
  }

  // Configure publisher registration
  NodeTestFixture& register_publisher(const std::string& topic,
                                      std::array<uint64_t, 2> message_hash,
                                      bool should_fail = false,
                                      const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                          8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::PubInfo pub_info;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_pub_register(rixhub_endpoint_, pub_info, should_fail);

    return *this;
  }

  // Configure publisher deregistration
  NodeTestFixture& deregister_publisher(const std::string& topic,
                                        std::array<uint64_t, 2> message_hash,
                                        const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                            8001)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::PubInfo pub_info;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;
    SocketBuilder(socket).as_pub_deregister(rixhub_endpoint_, pub_info);
    return *this;
  }

  // Configure subscriber registration
  NodeTestFixture& register_subscriber(const std::string& topic,
                                       std::array<uint64_t, 2> message_hash,
                                       bool should_fail = false,
                                       const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                           8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SubInfo sub_info;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_sub_register(rixhub_endpoint_, sub_info, should_fail);

    return *this;
  }

  // Configure subscriber deregistration
  NodeTestFixture& deregister_subscriber(const std::string& topic,
                                         std::array<uint64_t, 2> message_hash,
                                         const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                             8001)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::SubInfo sub_info;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;
    SocketBuilder(socket).as_sub_deregister(rixhub_endpoint_, sub_info);
    return *this;
  }

  // Configure service registration
  NodeTestFixture& register_service(const std::string& service,
                                    std::array<uint64_t, 2> request_hash,
                                    std::array<uint64_t, 2> response_hash,
                                    bool should_fail = false,
                                    const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                        8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SrvInfo srv_info;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_srv_register(rixhub_endpoint_, srv_info, should_fail);

    return *this;
  }

  // Configure service deregistration
  NodeTestFixture& deregister_service(const std::string& service,
                                      std::array<uint64_t, 2> request_hash,
                                      std::array<uint64_t, 2> response_hash,
                                      const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                          8001)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::SrvInfo srv_info;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;
    SocketBuilder(socket).as_srv_deregister(rixhub_endpoint_, srv_info);
    return *this;
  }

  // Configure service client request
  NodeTestFixture& request_service_client(const std::string& service,
                                          std::array<uint64_t, 2> request_hash,
                                          std::array<uint64_t, 2> response_hash,
                                          bool should_fail = false,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                              8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SrvRequest srv_req;
    srv_req.name = service;
    srv_req.request_hash = request_hash;
    srv_req.response_hash = response_hash;
    msg::mediator::SrvResponse srv_res;
    srv_res.srv_info.name = service;
    srv_res.srv_info.request_hash = request_hash;
    srv_res.srv_info.response_hash = response_hash;
    srv_res.srv_info.endpoint.address = endpoint.address;
    srv_res.srv_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket)
        .as_srv_cli_request(rixhub_endpoint_, srv_req, srv_res, should_fail);

    return *this;
  }

  // Configure parameter set request
  NodeTestFixture& request_parameter_set(const std::string& name,
                                         std::shared_ptr<msg::Message> value,
                                         bool should_fail = false) {
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
  NodeTestFixture& request_parameter_get(const std::string& name,
                                         std::shared_ptr<msg::Message> value,
                                         bool should_fail = false) {
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
  NodeTestFixture& request_system_info(msg::mediator::SystemInfo info,
                                       bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_sys_info_request(rixhub_endpoint_, info, should_fail);
    return *this;
  }

  // Configure server
  NodeTestFixture& create_server(const Endpoint& endpoint = Endpoint("127.0.0.1", 0),
                                 const Endpoint& bound_endpoint = Endpoint("127.0.0.1",
                                                                           8001),
                                 int accept_count = 0) {
    auto socket = socket_manager_.create_socket();
    server_sockets_.push_back(socket);
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_server(
        endpoint,
        bound_endpoint,
        [this]() { return this->socket_manager_.get_factory()(); },
        accept_count);
    return *this;
  }

  // Configure publisher server with connection sockets
  template <typename TMsg>
  NodeTestFixture&
  create_pub_connection(int messages_per_connection,
                        const std::vector<std::shared_ptr<TMsg>>& messages) {
    // Create connection sockets
    auto conn_socket = socket_manager_.create_socket();
    connection_sockets_.push_back(conn_socket);
    auto builder = SocketBuilder(conn_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_pub_connection(messages_per_connection, messages);
    return *this;
  }

  // Configure subscriber connections
  NodeTestFixture& create_sub_connection(msg::mediator::SubNotify sub_notify) {
    // Create notification connection socket
    auto notify_socket = socket_manager_.create_socket();
    SocketBuilder(notify_socket).as_sub_connection(sub_notify);
    return *this;
  }

  // Configure subscriber clients
  template <typename TMsg>
  NodeTestFixture& create_sub_client(const Endpoint& endpoint,
                                     int messages_to_receive,
                                     const std::vector<std::shared_ptr<TMsg>>& messages) {
    auto client_socket = socket_manager_.create_socket();
    client_sockets_.push_back(client_socket);
    auto builder = SocketBuilder(client_socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_sub_client(messages_to_receive, endpoint, messages);
    return *this;
  }

  // Configure service server with connection sockets
  template <typename TRequest, typename TResponse>
  NodeTestFixture& create_srv_connection(std::shared_ptr<TRequest> request,
                                         std::shared_ptr<TResponse> response) {
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
  NodeTestFixture& create_srv_cli_client(const Endpoint& service_endpoint,
                                         std::shared_ptr<TRequest> request,
                                         std::shared_ptr<TResponse> response) {
    auto socket = socket_manager_.create_socket();
    client_sockets_.push_back(socket);
    auto builder = SocketBuilder(socket);
    if (enable_notifications_) {
      builder.enable_operation_notifications();
    }
    builder.as_srv_cli_client(service_endpoint, request, response);
    return *this;
  }

  // Build and return the configured node
  std::unique_ptr<Node> build() {
    return std::make_unique<Node>(
        node_name_, rixhub_endpoint_, socket_manager_.get_factory());
  }

  template <typename... Args> std::unique_ptr<Node> build_custom(Args&&... args) {
    return std::make_unique<Node>(std::forward<Args>(args)...);
  }

private:
  SocketManager socket_manager_;
  std::string node_name_;
  Endpoint rixhub_endpoint_;
  uint64_t node_id_;
  bool enable_notifications_;

  // Categorized socket tracking
  std::vector<std::shared_ptr<MockSocket>>
      server_sockets_; // Server sockets (accept connections)
  std::vector<std::shared_ptr<MockSocket>>
      connection_sockets_; // Connection sockets (pub/srv connections)
  std::vector<std::shared_ptr<MockSocket>>
      client_sockets_; // Client sockets (sub/srv clients)

  msg::mediator::NodeInfo make_node_info() const {
    msg::mediator::NodeInfo info;
    info.name = node_name_;
    info.id = node_id_;
    return info;
  }
};

} // namespace rix