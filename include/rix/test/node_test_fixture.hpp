#pragma once

#include "rix/core/node.hpp"
#include "socket_builder.hpp"
#include "socket_manager.hpp"
#include <gtest/gtest.h>

namespace rix {

// High-level test fixture for Node tests
template <typename Node = rix::Node> class NodeTestFixture {
public:
  NodeTestFixture(const std::string& node_name = "test_node",
                  const Endpoint& rixhub = Endpoint("127.0.0.1", 8000))
      : node_name_(node_name), rixhub_endpoint_(rixhub), node_id_(0) {}

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
    SocketBuilder(socket).as_server(
        endpoint,
        bound_endpoint,
        [this]() { return this->socket_manager_.get_factory()(); },
        accept_count);
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

  msg::mediator::NodeInfo make_node_info() const {
    msg::mediator::NodeInfo info;
    info.name = node_name_;
    info.id = node_id_;
    return info;
  }
};

} // namespace rix