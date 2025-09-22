#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "rix/core/common.hpp"
#include "rix/core/publisher.hpp"
#include "rix/core/service.hpp"
#include "rix/core/service_client.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/core/timer.hpp"
#include "rix/ipc/signal.hpp"
#include "rix/ipc/socket.hpp"
#include "rix/msg/standard/Void.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/ParamInfo.hpp"
#include "rix/msg/mediator/SystemInfo.hpp"
#include "rix/msg/standard/UInt64.hpp"
#include "rix/util/log.hpp"

namespace rix::core {

class Node : public Spinner {
public:
  Node(const std::string &name, const rix::ipc::Endpoint &rixhub_endpoint,
       SocketFactory socket_factory = rix::ipc::create_socket);

  Node(const Node &) = delete;
  Node &operator=(const Node &) = delete;
  Node(Node &&) = delete;
  Node &operator=(Node &&) = delete;

  virtual ~Node();

  template <typename TMsg>
  std::shared_ptr<Publisher> create_publisher(const std::string &topic,
                                              const rix::ipc::Endpoint &endpoint = rix::ipc::Endpoint("127.0.0.1", 0));

  template <typename TMsg>
  std::shared_ptr<Subscriber> create_subscriber(const std::string &topic, std::function<void(const TMsg &)> callback,
                                                const rix::ipc::Endpoint &endpoint = rix::ipc::Endpoint("127.0.0.1",
                                                                                                        0));

  std::shared_ptr<Timer> create_timer(const rix::util::Duration &d, Timer::Callback callback);

  template <typename TRequest, typename TResponse>
  std::shared_ptr<ServiceClient> create_service_client(const std::string &service);

  template <typename TRequest, typename TResponse>
  std::shared_ptr<Service> create_service(const std::string &service,
                                          std::function<void(const TRequest &, TResponse &)> callback,
                                          const rix::ipc::Endpoint &endpoint = rix::ipc::Endpoint("127.0.0.1", 0));

  bool ok() const override;
  void shutdown() override;
  void spin_once() override;

  template <typename TParam> bool set_parameter(const std::string &name, const TParam &parameter);
  template <typename TParam> bool get_parameter(const std::string &name, TParam &parameter);

  bool get_system_info(rix::msg::mediator::SystemInfo &info);

private:
  rix::ipc::Endpoint rixhub_endpoint_;
  SocketFactory socket_factory_;
  rix::msg::mediator::NodeInfo info_;
  std::vector<std::shared_ptr<Spinner>> components_;
  std::atomic<bool> shutdown_flag_;
  std::atomic<bool> registered_flag_;

  static uint64_t generate_id();

  std::shared_ptr<Publisher> create_publisher(const rix::msg::mediator::TopicInfo &topic_info,
                                              const rix::ipc::Endpoint &rixhub_endpoint,
                                              const rix::ipc::Endpoint &endpoint);

  std::shared_ptr<Subscriber> create_subscriber(const rix::msg::mediator::TopicInfo &topic_info,
                                                const rix::ipc::Endpoint &rixhub_endpoint,
                                                const rix::ipc::Endpoint &endpoint);

  std::shared_ptr<Service> create_service(rix::msg::mediator::SrvInfo &service_info,
                                          const rix::ipc::Endpoint &rixhub_endpoint,
                                          const rix::ipc::Endpoint &endpoint);
};

template <typename TMsg>
std::shared_ptr<Publisher> Node::create_publisher(const std::string &topic, const rix::ipc::Endpoint &endpoint) {
  static_assert(std::is_base_of<rix::msg::Message, TMsg>::value, "TMsg must be a subclass of rix::msg::Message.");
  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot create publisher." << std::endl;
    return nullptr;
  }
  // Get topic information
  rix::msg::mediator::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  // Invoke private implementation
  return create_publisher(topic_info, rixhub_endpoint_, endpoint);
}

template <typename TMsg>
std::shared_ptr<Subscriber> Node::create_subscriber(const std::string &topic,
                                                    std::function<void(const TMsg &)> callback,
                                                    const rix::ipc::Endpoint &endpoint) {
  static_assert(std::is_base_of<rix::msg::Message, TMsg>::value, "TMsg must be a subclass of rix::msg::Message.");
  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot create subscriber." << std::endl;
    return nullptr;
  }

  // Get topic information
  rix::msg::mediator::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  // Invoke private implementation
  auto sub = create_subscriber(topic_info, rixhub_endpoint_, endpoint);

  // Set callback (need template info to do this)
  if (sub)
    sub->set_callback(callback);

  return sub;
}

template <typename TRequest, typename TResponse>
std::shared_ptr<Service> Node::create_service(const std::string &service,
                                              std::function<void(const TRequest &, TResponse &)> callback,
                                              const rix::ipc::Endpoint &endpoint) {
  static_assert(std::is_base_of<rix::msg::Message, TRequest>::value,
                "TRequest must be a subclass of rix::msg::Message.");
  static_assert(std::is_base_of<rix::msg::Message, TResponse>::value,
                "TResponse must be a subclass of rix::msg::Message.");

  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot create service." << std::endl;
    return nullptr;
  }

  rix::msg::mediator::SrvInfo service_info;
  service_info.name = service;
  service_info.request_hash = TRequest().hash();
  service_info.response_hash = TResponse().hash();

  auto srv = create_service(service_info, rixhub_endpoint_, endpoint);
  if (srv)
    srv->set_callback<TRequest, TResponse>(callback);
  return srv;
}

template <typename TRequest, typename TResponse>
std::shared_ptr<ServiceClient> Node::create_service_client(const std::string &service) {
  static_assert(std::is_base_of<rix::msg::Message, TRequest>::value,
                "TRequest must be a subclass of rix::msg::Message.");
  static_assert(std::is_base_of<rix::msg::Message, TResponse>::value,
                "TResponse must be a subclass of rix::msg::Message.");

  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot create service client." << std::endl;
    return nullptr;
  }

  rix::msg::mediator::SrvRequest service_request;
  service_request.id = generate_id();
  service_request.name = service;
  service_request.node_id = info_.id;
  service_request.request_hash = TRequest().hash();
  service_request.response_hash = TResponse().hash();

  return std::shared_ptr<ServiceClient>(new ServiceClient(service_request, socket_factory_, rixhub_endpoint_));
}

template <typename TParam> bool Node::set_parameter(const std::string &name, const TParam &parameter) {
  static_assert(std::is_base_of<rix::msg::Message, TParam>::value, "TParam must be a subclass of rix::msg::Message.");
  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot set parameter." << std::endl;
    return false;
  }

  rix::msg::mediator::ParamInfo info;
  info.id = info_.id;
  info.name = name;
  info.message_hash = parameter.hash();
  info.data.resize(parameter.size());
  size_t offset = 0;
  parameter.serialize(info.data.data(), offset);

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    return false;
  }
  if (!client->send_message(OPCODE::PARAM_SET_REQUEST, info)) {
    return false;
  }

  rix::msg::mediator::Operation op;
  rix::msg::mediator::Status status;
  if (!client->recv_message(op, status)) {
    return false;
  }

  if (op.opcode != OPCODE::STATUS_RESPONSE) {
    return false;
  }

  return status.error == 0;
}

template <typename TParam> bool Node::get_parameter(const std::string &name, TParam &parameter) {
  static_assert(std::is_base_of<rix::msg::Message, TParam>::value, "TParam must be a subclass of rix::msg::Message.");
  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot get parameter." << std::endl;
    return false;
  }

  rix::msg::mediator::ParamInfo info;
  info.id = info_.id;
  info.name = name;
  info.message_hash = parameter.hash();
  rix::msg::mediator::ParamInfo info_received;

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    return false;
  }
  if (!client->send_message(OPCODE::PARAM_GET_REQUEST, info)) {
    return false;
  }
  rix::msg::mediator::Operation op;
  if (!client->recv_message(op, info_received)) {
    return false;
  }
  if (op.opcode != OPCODE::PARAM_GET_RESPONSE) {
    return false;
  }
  size_t offset = 0;
  return parameter.deserialize(info_received.data.data(), info_received.data.size(), offset);
}

} // namespace rix::core