#pragma once

#include "rix/core/mediator.hpp"
#include "rix/msg/mediator/ActRequest.hpp"
#include "rix/msg/mediator/ActResponse.hpp"
#include "rix/msg/mediator/SrvRequest.hpp"
#include "rix/msg/mediator/SrvResponse.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/mediator/SubNotify.hpp"
#include "rix/test/socket_builder.hpp"
#include "rix/test/socket_manager.hpp"
#include <gtest/gtest.h>
#include <rix/msg/standard/UInt64.hpp>

namespace rix {

// High-level test fixture for Node tests
class MediatorTestFixture {
public:
  explicit MediatorTestFixture(const Endpoint& endpoint = Endpoint("127.0.0.1", 0)) : endpoint_(endpoint) {}

  MediatorTestFixture& ping() {
    msg::mediator::Operation operation;
    operation.opcode = OPCODE::PING;
    operation.len = 0;

    msg::mediator::Status status;
    status.error = 0;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).recv_message(operation, operation.size()).send_message(OPCODE::STATUS_RESPONSE, status).close();
    return *this;
  }

  // Configure node registration to succeed
  MediatorTestFixture& register_node(const std::string& node_name, uint64_t node_id, bool should_fail = false) {
    msg::mediator::NodeInfo node_info;
    node_info.name = node_name;
    node_info.id = node_id;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = node_info.id;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket)
        .recv_message(OPCODE::NODE_REGISTER, node_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // Configure node deregistration
  MediatorTestFixture& deregister_node(const std::string& node_name, uint64_t node_id) {
    msg::mediator::NodeInfo node_info;
    node_info.name = node_name;
    node_info.id = node_id;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).recv_message(OPCODE::NODE_DEREGISTER, node_info).close();
    return *this;
  }

  // Configure publisher registration
  MediatorTestFixture& register_publisher(uint64_t id,
                                          uint64_t node_id,
                                          const std::string& topic,
                                          std::array<uint64_t, 2> message_hash,
                                          bool should_fail = false,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::PubInfo pub_info;
    pub_info.node_id = node_id;
    pub_info.id = id;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = pub_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .recv_message(OPCODE::PUB_REGISTER, pub_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();

    return *this;
  }

  // Configure publisher deregistration
  MediatorTestFixture& deregister_publisher(uint64_t id,
                                            uint64_t node_id,
                                            const std::string& topic,
                                            std::array<uint64_t, 2> message_hash,
                                            const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::PubInfo pub_info;
    pub_info.node_id = node_id;
    pub_info.id = id;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).recv_message(OPCODE::PUB_DEREGISTER, pub_info).close();
    return *this;
  }

  // Configure subscriber registration
  MediatorTestFixture& register_subscriber(uint64_t id,
                                           uint64_t node_id,
                                           const std::string& topic,
                                           std::array<uint64_t, 2> message_hash,
                                           bool should_fail = false,
                                           const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::SubInfo sub_info;
    sub_info.node_id = node_id;
    sub_info.id = id;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = sub_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .recv_message(OPCODE::SUB_REGISTER, sub_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();

    return *this;
  }

  // Configure subscriber deregistration
  MediatorTestFixture& deregister_subscriber(uint64_t id,
                                             uint64_t node_id,
                                             const std::string& topic,
                                             std::array<uint64_t, 2> message_hash,
                                             const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::SubInfo sub_info;
    sub_info.node_id = node_id;
    sub_info.id = id;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).recv_message(OPCODE::SUB_DEREGISTER, sub_info).close();
    return *this;
  }

  // Configure service registration
  MediatorTestFixture& register_service(uint64_t id,
                                        uint64_t node_id,
                                        const std::string& service,
                                        std::array<uint64_t, 2> request_hash,
                                        std::array<uint64_t, 2> response_hash,
                                        bool should_fail = false,
                                        const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::SrvInfo srv_info;
    srv_info.id = id;
    srv_info.node_id = node_id;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = srv_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .recv_message(OPCODE::SRV_REGISTER, srv_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();

    return *this;
  }

  // Configure service deregistration
  MediatorTestFixture& deregister_service(uint64_t id,
                                          uint64_t node_id,
                                          const std::string& service,
                                          std::array<uint64_t, 2> request_hash,
                                          std::array<uint64_t, 2> response_hash,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::SrvInfo srv_info;
    srv_info.id = id;
    srv_info.node_id = node_id;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).recv_message(OPCODE::SRV_DEREGISTER, srv_info).close();
    return *this;
  }

  // Configure service client request
  MediatorTestFixture& request_service_client(uint64_t node_id,
                                              const std::string& service,
                                              std::array<uint64_t, 2> request_hash,
                                              std::array<uint64_t, 2> response_hash,
                                              uint64_t srv_id,
                                              bool should_fail = false,
                                              const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::SrvRequest srv_req;
    srv_req.node_id = node_id;
    srv_req.name = service;
    srv_req.request_hash = request_hash;
    srv_req.response_hash = response_hash;

    msg::mediator::SrvResponse srv_res;
    srv_res.error = should_fail ? -1 : 0;
    if (!should_fail) {
      srv_res.srv_info.name = service;
      srv_res.srv_info.id = srv_id;
      srv_res.srv_info.node_id = node_id;
      srv_res.srv_info.request_hash = request_hash;
      srv_res.srv_info.response_hash = response_hash;
      srv_res.srv_info.endpoint.address = endpoint.address;
      srv_res.srv_info.endpoint.port = endpoint.port;
    }

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .recv_message(OPCODE::SRV_REQUEST, srv_req)
        .send_message(OPCODE::SRV_RESPONSE, srv_res)
        .close();

    return *this;
  }

  // Configure action registration
  MediatorTestFixture& register_action(uint64_t id,
                                       uint64_t node_id,
                                       const std::string& action,
                                       std::array<uint64_t, 2> goal_hash,
                                       std::array<uint64_t, 2> feedback_hash,
                                       std::array<uint64_t, 2> result_hash,
                                       bool should_fail = false,
                                       const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::ActInfo act_info;
    act_info.id = id;
    act_info.node_id = node_id;
    act_info.name = action;
    act_info.goal_hash = goal_hash;
    act_info.feedback_hash = feedback_hash;
    act_info.result_hash = result_hash;
    act_info.endpoint.address = endpoint.address;
    act_info.endpoint.port = endpoint.port;

    msg::mediator::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = act_info.id;

    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .recv_message(OPCODE::ACT_REGISTER, act_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // Configure action deregistration
  MediatorTestFixture& deregister_action(uint64_t id,
                                         uint64_t node_id,
                                         const std::string& action,
                                         bool should_fail = false,
                                         const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::ActInfo act_info;
    act_info.id = id;
    act_info.node_id = node_id;
    act_info.name = action;
    act_info.endpoint.address = endpoint.address;
    act_info.endpoint.port = endpoint.port;

    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).recv_message(OPCODE::ACT_DEREGISTER, act_info).close();
    return *this;
  }

  // Configure action client request
  MediatorTestFixture& request_action_client(uint64_t node_id,
                                             const std::string& action,
                                             std::array<uint64_t, 2> goal_hash,
                                             std::array<uint64_t, 2> feedback_hash,
                                             std::array<uint64_t, 2> result_hash,
                                             uint64_t act_id,
                                             bool should_fail = false,
                                             const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    msg::mediator::ActRequest act_req;
    act_req.node_id = node_id;
    act_req.name = action;
    act_req.goal_hash = goal_hash;
    act_req.feedback_hash = feedback_hash;
    act_req.result_hash = result_hash;

    msg::mediator::ActResponse act_res;
    act_res.error = should_fail ? -1 : 0;
    if (!should_fail) {
      act_res.act_info.name = action;
      act_res.act_info.node_id = node_id;
      act_res.act_info.id = act_id;
      act_res.act_info.goal_hash = goal_hash;
      act_res.act_info.feedback_hash = feedback_hash;
      act_res.act_info.result_hash = result_hash;
      act_res.act_info.endpoint.address = endpoint.address;
      act_res.act_info.endpoint.port = endpoint.port;
    }
    // Registration socket
    auto reg_socket = socket_manager_.create_socket();
    SocketBuilder(reg_socket)
        .recv_message(OPCODE::ACT_REQUEST, act_req)
        .send_message(OPCODE::ACT_RESPONSE, act_res)
        .close();
    return *this;
  }

  // Configure parameter get request
  MediatorTestFixture& request_parameter_get(uint64_t node_id,
                                             const std::string& name,
                                             std::shared_ptr<msg::Message> value,
                                             bool should_fail = false) {
    msg::mediator::ParamInfo request;
    request.id = node_id;
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
        .recv_message(OPCODE::PARAM_GET_REQUEST, request)
        .send_message(OPCODE::PARAM_GET_RESPONSE, response)
        .close();
    return *this;
  }

  // Configure parameter set request
  MediatorTestFixture& request_parameter_set(uint64_t node_id,
                                             const std::string& name,
                                             std::shared_ptr<msg::Message> value,
                                             bool should_fail = false) {
    msg::mediator::ParamInfo param_info;
    param_info.id = node_id;
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
        .recv_message(OPCODE::PARAM_SET_REQUEST, param_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // Configure system info get request
  MediatorTestFixture& request_system_info(uint64_t node_id, const msg::mediator::SystemInfo& info) {
    auto socket = socket_manager_.create_socket();
    msg::standard::UInt64 id;
    id.data = node_id;
    SocketBuilder(socket)
        .recv_message(OPCODE::SYSTEM_GET_REQUEST, id)
        .send_message(OPCODE::SYSTEM_GET_RESPONSE, info)
        .close();
    return *this;
  }

  // Configure subscriber notification
  MediatorTestFixture& notify_subscriber(uint64_t id,
                                         const Endpoint& endpoint,
                                         const std::string& topic,
                                         std::array<uint64_t, 2> message_hash,
                                         const msg::mediator::SubNotify& notify) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).connect(endpoint).send_message(OPCODE::SUB_NOTIFY, notify).close();
    return *this;
  }

  // Configure server
  MediatorTestFixture& create_server(const Endpoint& endpoint = Endpoint("127.0.0.1", 0),
                                     const Endpoint& bound_endpoint = Endpoint("127.0.0.1", 8001),
                                     int accept_count = 0) {
    auto socket = socket_manager_.create_socket();
    SocketBuilder(socket).as_server(
        endpoint, bound_endpoint, [this]() { return this->socket_manager_.get_factory()(); }, accept_count);
    return *this;
  }

  // Build and return the configured node
  std::unique_ptr<Mediator> build() { return std::make_unique<Mediator>(endpoint_, socket_manager_.get_factory()); }

private:
  SocketManager socket_manager_;
  Endpoint endpoint_;
};

} // namespace rix