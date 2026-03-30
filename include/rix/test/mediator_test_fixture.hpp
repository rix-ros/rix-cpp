#pragma once

#include "rix/core/mediator.hpp"
#include "rix/std_msgs/UInt64.hpp"
#include "rix/sys_msgs/ActRequest.hpp"
#include "rix/sys_msgs/ActResponse.hpp"
#include "rix/sys_msgs/SrvRequest.hpp"
#include "rix/sys_msgs/SrvResponse.hpp"
#include "rix/sys_msgs/Status.hpp"
#include "rix/sys_msgs/SubNotify.hpp"
#include "rix/test/acceptor_builder.hpp"
#include "rix/test/stream_builder.hpp"
#include "rix/test/transport_manager.hpp"
#include <gtest/gtest.h>

namespace rix {

/**
 * @brief Updated MediatorTestFixture for testing the Mediator (rixhub).
 *
 * Uses the new Acceptor/Stream interfaces. The Mediator's server acceptor
 * is a MockAcceptor whose accept() calls pop MockStreams from the queue.
 * Each MockStream is pre-programmed with the message sequence for one
 * command (register, deregister, request, etc.).
 */
class MediatorTestFixture {
public:
  explicit MediatorTestFixture(const Endpoint& endpoint = Endpoint("127.0.0.1", 0))
      : endpoint_(endpoint), accept_count_(0) {}

  ~MediatorTestFixture() { reset_transport_factory(Protocol::TCP); }

  // ---------------------------------------------------------------------------
  // Server setup
  // ---------------------------------------------------------------------------

  MediatorTestFixture& create_server(const Endpoint& endpoint = Endpoint("127.0.0.1", 0),
                                     const Endpoint& bound_endpoint = Endpoint("127.0.0.1", 48104),
                                     int accept_count = 0) {
    accept_count_ = accept_count;
    bound_endpoint_ = bound_endpoint;
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Ping
  // ---------------------------------------------------------------------------

  MediatorTestFixture& ping() {
    sys_msgs::Operation operation;
    operation.opcode = OPCODE::PING;
    operation.len = 0;

    sys_msgs::Status status;
    status.error = 0;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(operation, operation.get_prefix_len())
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Node registration / deregistration
  // ---------------------------------------------------------------------------

  MediatorTestFixture& register_node(const std::string& node_name, uint64_t node_id, bool should_fail = false) {
    sys_msgs::NodeInfo node_info;
    node_info.name = node_name;
    node_info.id = node_id;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = node_info.id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::NODE_REGISTER, node_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  MediatorTestFixture& deregister_node(const std::string& node_name, uint64_t node_id) {
    sys_msgs::NodeInfo node_info;
    node_info.name = node_name;
    node_info.id = node_id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).recv_message(OPCODE::NODE_DEREGISTER, node_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Publisher registration / deregistration
  // ---------------------------------------------------------------------------

  MediatorTestFixture& register_publisher(uint64_t id,
                                          uint64_t node_id,
                                          const std::string& topic,
                                          std::array<uint64_t, 2> message_hash,
                                          bool should_fail = false,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::PubInfo pub_info;
    pub_info.node_id = node_id;
    pub_info.id = id;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = pub_info.id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::PUB_REGISTER, pub_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  MediatorTestFixture& deregister_publisher(uint64_t id,
                                            uint64_t node_id,
                                            const std::string& topic,
                                            std::array<uint64_t, 2> message_hash,
                                            const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::PubInfo pub_info;
    pub_info.node_id = node_id;
    pub_info.id = id;
    pub_info.topic_info.name = topic;
    pub_info.topic_info.message_hash = message_hash;
    pub_info.endpoint.address = endpoint.address;
    pub_info.endpoint.port = endpoint.port;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).recv_message(OPCODE::PUB_DEREGISTER, pub_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Subscriber registration / deregistration
  // ---------------------------------------------------------------------------

  MediatorTestFixture& register_subscriber(uint64_t id,
                                           uint64_t node_id,
                                           const std::string& topic,
                                           std::array<uint64_t, 2> message_hash,
                                           bool should_fail = false,
                                           const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::SubInfo sub_info;
    sub_info.node_id = node_id;
    sub_info.id = id;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = sub_info.id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::SUB_REGISTER, sub_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  MediatorTestFixture& deregister_subscriber(uint64_t id,
                                             uint64_t node_id,
                                             const std::string& topic,
                                             std::array<uint64_t, 2> message_hash,
                                             const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::SubInfo sub_info;
    sub_info.node_id = node_id;
    sub_info.id = id;
    sub_info.topic_info.name = topic;
    sub_info.topic_info.message_hash = message_hash;
    sub_info.endpoint.address = endpoint.address;
    sub_info.endpoint.port = endpoint.port;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).recv_message(OPCODE::SUB_DEREGISTER, sub_info).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Service registration / deregistration / request
  // ---------------------------------------------------------------------------

  MediatorTestFixture& register_service(uint64_t id,
                                        uint64_t node_id,
                                        const std::string& service,
                                        std::array<uint64_t, 2> request_hash,
                                        std::array<uint64_t, 2> response_hash,
                                        bool should_fail = false,
                                        const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::SrvInfo srv_info;
    srv_info.id = id;
    srv_info.node_id = node_id;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = srv_info.id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::SRV_REGISTER, srv_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  MediatorTestFixture& deregister_service(uint64_t id,
                                          uint64_t node_id,
                                          const std::string& service,
                                          std::array<uint64_t, 2> request_hash,
                                          std::array<uint64_t, 2> response_hash,
                                          const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::SrvInfo srv_info;
    srv_info.id = id;
    srv_info.node_id = node_id;
    srv_info.name = service;
    srv_info.request_hash = request_hash;
    srv_info.response_hash = response_hash;
    srv_info.endpoint.address = endpoint.address;
    srv_info.endpoint.port = endpoint.port;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).recv_message(OPCODE::SRV_DEREGISTER, srv_info).close();
    return *this;
  }

  MediatorTestFixture& request_service_client(uint64_t node_id,
                                              const std::string& service,
                                              std::array<uint64_t, 2> request_hash,
                                              std::array<uint64_t, 2> response_hash,
                                              uint64_t srv_id,
                                              bool should_fail = false,
                                              const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::SrvRequest srv_req;
    srv_req.node_id = node_id;
    srv_req.name = service;
    srv_req.request_hash = request_hash;
    srv_req.response_hash = response_hash;

    sys_msgs::SrvResponse srv_res;
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

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::SRV_REQUEST, srv_req)
        .send_message(OPCODE::SRV_RESPONSE, srv_res)
        .close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Action registration / deregistration / request
  // ---------------------------------------------------------------------------

  MediatorTestFixture& register_action(uint64_t id,
                                       uint64_t node_id,
                                       const std::string& action,
                                       std::array<uint64_t, 2> goal_hash,
                                       std::array<uint64_t, 2> feedback_hash,
                                       std::array<uint64_t, 2> result_hash,
                                       bool should_fail = false,
                                       const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::ActInfo act_info;
    act_info.id = id;
    act_info.node_id = node_id;
    act_info.name = action;
    act_info.goal_hash = goal_hash;
    act_info.feedback_hash = feedback_hash;
    act_info.result_hash = result_hash;
    act_info.endpoint.address = endpoint.address;
    act_info.endpoint.port = endpoint.port;

    sys_msgs::Status status;
    status.error = should_fail ? -1 : 0;
    status.id = act_info.id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::ACT_REGISTER, act_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  MediatorTestFixture& deregister_action(uint64_t id,
                                         uint64_t node_id,
                                         const std::string& action,
                                         bool should_fail = false,
                                         const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::ActInfo act_info;
    act_info.id = id;
    act_info.node_id = node_id;
    act_info.name = action;
    act_info.endpoint.address = endpoint.address;
    act_info.endpoint.port = endpoint.port;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).recv_message(OPCODE::ACT_DEREGISTER, act_info).close();
    return *this;
  }

  MediatorTestFixture& request_action_client(uint64_t node_id,
                                             const std::string& action,
                                             std::array<uint64_t, 2> goal_hash,
                                             std::array<uint64_t, 2> feedback_hash,
                                             std::array<uint64_t, 2> result_hash,
                                             uint64_t act_id,
                                             bool should_fail = false,
                                             const Endpoint& endpoint = Endpoint("127.0.0.1", 8001)) {
    sys_msgs::ActRequest act_req;
    act_req.node_id = node_id;
    act_req.name = action;
    act_req.goal_hash = goal_hash;
    act_req.feedback_hash = feedback_hash;
    act_req.result_hash = result_hash;

    sys_msgs::ActResponse act_res;
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

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::ACT_REQUEST, act_req)
        .send_message(OPCODE::ACT_RESPONSE, act_res)
        .close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Parameters and system info
  // ---------------------------------------------------------------------------

  MediatorTestFixture& request_parameter_set(uint64_t node_id,
                                             const std::string& name,
                                             std::shared_ptr<Message> value,
                                             bool should_fail = false) {
    sys_msgs::ParamInfo param_info;
    param_info.id = node_id;
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
        .recv_message(OPCODE::PARAM_SET_REQUEST, param_info)
        .send_message(OPCODE::STATUS_RESPONSE, status)
        .close();
    return *this;
  }

  MediatorTestFixture& request_parameter_get(uint64_t node_id,
                                             const std::string& name,
                                             std::shared_ptr<Message> value,
                                             bool should_fail = false) {
    sys_msgs::ParamInfo request;
    request.id = node_id;
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
        .recv_message(OPCODE::PARAM_GET_REQUEST, request)
        .send_message(OPCODE::PARAM_GET_RESPONSE, response)
        .close();
    return *this;
  }

  MediatorTestFixture& request_system_info(uint64_t node_id, const sys_msgs::SystemInfo& info) {
    std_msgs::UInt64 id;
    id.data = node_id;

    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream)
        .recv_message(OPCODE::SYSTEM_GET_REQUEST, id)
        .send_message(OPCODE::SYSTEM_GET_RESPONSE, info)
        .close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Subscriber notification (Mediator sends to subscriber endpoint)
  // ---------------------------------------------------------------------------

  MediatorTestFixture& notify_subscriber(uint64_t id,
                                         const Endpoint& endpoint,
                                         const std::string& topic,
                                         std::array<uint64_t, 2> message_hash,
                                         const sys_msgs::SubNotify& notify) {
    auto stream = transport_manager_.create_stream();
    StreamBuilder(stream).send_message(OPCODE::SUB_NOTIFY, notify).close();
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Build
  // ---------------------------------------------------------------------------

  std::unique_ptr<Mediator> build() {
    // Create the server acceptor and wire it up
    auto acceptor = transport_manager_.create_acceptor();
    AcceptorBuilder(acceptor).as_server(bound_endpoint_, accept_count_, [this]() {
      return transport_manager_.get_factory().create_stream(Endpoint{}, true);
    });

    // Inject mock transport
    set_transport_factory(Protocol::TCP, &transport_manager_.get_factory());
    return std::make_unique<Mediator>(endpoint_);
  }

private:
  TransportManager transport_manager_;
  Endpoint endpoint_;
  Endpoint bound_endpoint_{Endpoint("127.0.0.1", 48104)};
  int accept_count_ = 0;
};

} // namespace rix
