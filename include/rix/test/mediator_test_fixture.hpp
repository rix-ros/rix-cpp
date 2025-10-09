#pragma once

#include "rix/core/mediator.hpp"
#include "socket_builder.hpp"
#include "socket_manager.hpp"
#include <gtest/gtest.h>

namespace rix {

// High-level test fixture for Node tests
class MediatorTestFixture {
public:
  MediatorTestFixture(const Endpoint& endpoint = Endpoint("127.0.0.1", 0))
      : endpoint_(endpoint) {}

  // Configure node registration to succeed
  MediatorTestFixture& register_node(const std::string& node_name,
                                     uint64_t node_id,
                                     bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::NodeInfo node_info;
    node_info.name = node_name;
    node_info.id = node_id;
    SocketBuilder(socket).as_med_node_register(node_info, should_fail);
    return *this;
  }

  // Configure node deregistration
  MediatorTestFixture& deregister_node(const std::string& node_name, uint64_t node_id) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::NodeInfo node_info;
    node_info.name = node_name;
    node_info.id = node_id;
    SocketBuilder(socket).as_med_node_deregister(node_info);
    return *this;
  }

  // Configure publisher registration
  MediatorTestFixture& register_publisher(uint64_t id,
                                          uint64_t node_id,
                                          const std::string& topic,
                                          std::array<uint64_t, 2> message_hash,
                                          bool should_fail = false,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                              8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::PubInfo pub_info;
    pub_info.node_id = node_id;
    pub_info.id = id;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_med_pub_register(pub_info, should_fail);

    return *this;
  }

  // Configure publisher deregistration
  MediatorTestFixture&
  deregister_publisher(uint64_t id,
                       uint64_t node_id,
                       const std::string& topic,
                       std::array<uint64_t, 2> message_hash,
                       const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::PubInfo pub_info;
    pub_info.node_id = node_id;
    pub_info.id = id;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;
    SocketBuilder(socket).as_med_pub_deregister(pub_info);
    return *this;
  }

  // Configure subscriber registration
  MediatorTestFixture&
  register_subscriber(uint64_t id,
                      uint64_t node_id,
                      const std::string& topic,
                      std::array<uint64_t, 2> message_hash,
                      bool should_fail = false,
                      const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SubInfo sub_info;
    sub_info.node_id = node_id;
    sub_info.id = id;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_med_sub_register(sub_info, should_fail);

    return *this;
  }

  // Configure subscriber deregistration
  MediatorTestFixture&
  deregister_subscriber(uint64_t id,
                        uint64_t node_id,
                        const std::string& topic,
                        std::array<uint64_t, 2> message_hash,
                        const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::SubInfo sub_info;
    sub_info.node_id = node_id;
    sub_info.id = id;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;
    SocketBuilder(socket).as_med_sub_deregister(sub_info);
    return *this;
  }

  // Configure service registration
  MediatorTestFixture& register_service(uint64_t id,
                                        uint64_t node_id,
                                        const std::string& service,
                                        std::array<uint64_t, 2> request_hash,
                                        std::array<uint64_t, 2> response_hash,
                                        bool should_fail = false,
                                        const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                            8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SrvInfo srv_info;
    srv_info.id = id;
    srv_info.node_id = node_id;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_med_srv_register(srv_info, should_fail);

    return *this;
  }

  // Configure service deregistration
  MediatorTestFixture& deregister_service(uint64_t id,
                                          uint64_t node_id,
                                          const std::string& service,
                                          std::array<uint64_t, 2> request_hash,
                                          std::array<uint64_t, 2> response_hash,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1",
                                                                              8001)) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::SrvInfo srv_info;
    srv_info.id = id;
    srv_info.node_id = node_id;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;
    SocketBuilder(socket).as_med_srv_deregister(srv_info);
    return *this;
  }

  // Configure service client request
  MediatorTestFixture&
  request_service_client(uint64_t node_id,
                         const std::string& service,
                         std::array<uint64_t, 2> request_hash,
                         std::array<uint64_t, 2> response_hash,
                         uint64_t srv_id,
                         bool should_fail = false,
                         const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    msg::mediator::SrvRequest srv_req;
    srv_req.node_id = node_id;
    srv_req.name = service;
    srv_req.request_hash = request_hash;
    srv_req.response_hash = response_hash;
    msg::mediator::SrvResponse srv_res;
    srv_res.srv_info.name = service;
    srv_res.srv_info.id = srv_id;
    srv_res.srv_info.node_id = node_id;
    srv_res.srv_info.request_hash = request_hash;
    srv_res.srv_info.response_hash = response_hash;
    srv_res.srv_info.endpoint.address = endpoint.address;
    srv_res.srv_info.endpoint.port = endpoint.port;
    SocketBuilder(reg_socket).as_med_srv_cli_request(srv_req, srv_res, should_fail);

    return *this;
  }

  // Configure parameter set request
  MediatorTestFixture& request_parameter_set(uint64_t node_id,
                                             const std::string& name,
                                             std::shared_ptr<msg::Message> value,
                                             bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::ParamInfo param_info;
    param_info.id = node_id;
    param_info.name = name;
    param_info.message_hash = value->hash();
    param_info.data.resize(value->size());
    size_t offset = 0;
    value->serialize(param_info.data.data(), offset);
    SocketBuilder(socket).as_med_param_set(param_info, should_fail);
    return *this;
  }

  // Configure parameter get request
  MediatorTestFixture& request_parameter_get(uint64_t node_id,
                                             const std::string& name,
                                             std::shared_ptr<msg::Message> value,
                                             bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    msg::mediator::ParamInfo param_info;
    param_info.id = node_id;
    param_info.name = name;
    param_info.message_hash = value->hash();
    param_info.data.resize(value->size());
    size_t offset = 0;
    value->serialize(param_info.data.data(), offset);
    SocketBuilder(socket).as_med_param_get(param_info, should_fail);
    return *this;
  }

  // Configure system info get request
  MediatorTestFixture& request_system_info(uint64_t node_id,
                                           const msg::mediator::SystemInfo& info,
                                           bool should_fail = false) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_med_sys_info_request(info, node_id, should_fail);
    return *this;
  }

  // Configure subscriber notification
  MediatorTestFixture& notify_subscriber(uint64_t id,
                                         const std::string& topic,
                                         std::array<uint64_t, 2> message_hash,
                                         const msg::mediator::SubNotify& notify) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_med_sub_notify(notify);
    return *this;
  }

  // Configure server
  MediatorTestFixture&
  create_server(const Endpoint& endpoint = Endpoint("127.0.0.1", 0),
                const Endpoint& bound_endpoint = Endpoint("127.0.0.1", 8001),
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
  std::unique_ptr<Mediator> build() {
    return std::make_unique<Mediator>(endpoint_, socket_manager_.get_factory());
  }

private:
  SocketManager socket_manager_;
  Endpoint endpoint_;
};

} // namespace rix