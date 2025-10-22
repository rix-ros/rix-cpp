#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "rix/core/action.hpp"
#include "rix/core/action_client.hpp"
#include "rix/core/callback_traits.hpp"
#include "rix/core/common.hpp"
#include "rix/core/publisher.hpp"
#include "rix/core/service.hpp"
#include "rix/core/service_client.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/core/timer_callback.hpp"
#include "rix/ipc/signal.hpp"
#include "rix/ipc/socket.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/ParamInfo.hpp"
#include "rix/msg/mediator/SystemInfo.hpp"
#include "rix/msg/standard/UInt64.hpp"
#include "rix/msg/standard/Void.hpp"
#include "rix/util/id.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node : public Spinner {
public:
  Node(const std::string& name, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Node(Node&&) = delete;
  Node& operator=(Node&&) = delete;

  virtual ~Node();

  template <typename TMsg>
  std::shared_ptr<Publisher> create_publisher(const std::string& topic,
                                              const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  // Legacy API with explicit template parameter (for backward compatibility)
  template <typename TMsg>
  std::shared_ptr<Subscriber> create_subscriber(const std::string& topic,
                                                Subscriber::Callback<TMsg> callback,
                                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  // New API with automatic type deduction from callback
  template <typename Callback>
  auto
  create_subscriber(const std::string& topic, Callback&& callback, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0))
      -> std::enable_if_t<
          !std::is_same<
              std::decay_t<Callback>,
              Subscriber::Callback<typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType>>::value,
          std::shared_ptr<Subscriber>> {
    using TMsg = typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType;
    return create_subscriber<TMsg>(topic, std::forward<Callback>(callback), endpoint);
  }

  // Member function pointer API - bind member function to object instance
  template <typename TMsg, typename Class>
  std::shared_ptr<Subscriber> create_subscriber(const std::string& topic,
                                                void (Class::*callback)(const TMsg&),
                                                Class* instance,
                                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0)) {
    return create_subscriber<TMsg>(
        topic, [instance, callback](const TMsg& msg) { (instance->*callback)(msg); }, endpoint);
  }

  std::shared_ptr<TimerCallback> create_timer(const Duration& d, TimerCallback::Callback callback);

  template <typename Class>
  std::shared_ptr<TimerCallback>
  create_timer(const Duration& d, void (Class::*callback)(const TimerCallback::Event&), Class* instance) {
    return create_timer(d, [instance, callback](const TimerCallback::Event& event) { (instance->*callback)(event); });
  }

  template <typename TRequest, typename TResponse>
  std::shared_ptr<ServiceClient> create_service_client(const std::string& service);

  // Legacy API with explicit template parameters (for backward compatibility)
  template <typename TRequest, typename TResponse>
  std::shared_ptr<Service> create_service(const std::string& service,
                                          Service::Callback<TRequest, TResponse> callback,
                                          const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  // New API with automatic type deduction from callback
  template <typename Callback>
  auto
  create_service(const std::string& service, Callback&& callback, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0))
      -> std::enable_if_t<
          !std::is_same<std::decay_t<Callback>,
                        Service::Callback<typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType,
                                          typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType>>::value,
          std::shared_ptr<Service>> {
    using TRequest = typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType;
    using TResponse = typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType;
    return create_service<TRequest, TResponse>(service, std::forward<Callback>(callback), endpoint);
  }

  // Member function pointer API - bind member function to object instance
  template <typename TRequest, typename TResponse, typename Class>
  std::shared_ptr<Service> create_service(const std::string& service,
                                          void (Class::*callback)(const TRequest&, TResponse&),
                                          Class* instance,
                                          const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0)) {
    return create_service<TRequest, TResponse>(
        service,
        [instance, callback](const TRequest& req, TResponse& resp) { (instance->*callback)(req, resp); },
        endpoint);
  }

  template <typename TGoal, typename TFeedback, typename TResult>
  std::shared_ptr<ActionClient> create_action_client(const std::string& action);

  template <typename TGoal, typename TFeedback, typename TResult>
  std::shared_ptr<Action> create_action(const std::string& action,
                                        Action::Callback<TGoal, TFeedback, TResult> callback,
                                        const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  // New API with automatic type deduction from callback
  template <typename Callback>
  auto create_action(const std::string& action, Callback&& callback, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0))
      -> std::enable_if_t<
          !std::is_same<std::decay_t<Callback>,
                        Action::Callback<typename ActionCallbackTraits<std::decay_t<Callback>>::GoalType,
                                         typename ActionCallbackTraits<std::decay_t<Callback>>::FeedbackType,
                                         typename ActionCallbackTraits<std::decay_t<Callback>>::ResultType>>::value,
          std::shared_ptr<Action>> {
    using TGoal = typename ActionCallbackTraits<std::decay_t<Callback>>::GoalType;
    using TFeedback = typename ActionCallbackTraits<std::decay_t<Callback>>::FeedbackType;
    using TResult = typename ActionCallbackTraits<std::decay_t<Callback>>::ResultType;
    return create_action<TGoal, TFeedback, TResult>(action, std::forward<Callback>(callback), endpoint);
  }

  // Member function pointer API - bind member function to object instance
  template <typename TGoal, typename TFeedback, typename TResult, typename Class>
  std::shared_ptr<Action> create_action(const std::string& action,
                                        bool (Class::*callback)(const TGoal&, TFeedback&, TResult&),
                                        Class* instance,
                                        const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0)) {
    return create_action<TGoal, TFeedback, TResult>(
        action,
        [instance, callback](const TGoal& goal, TFeedback& feedback, TResult& result) {
          return (instance->*callback)(goal, feedback, result);
        },
        endpoint);
  }

  template <typename TParam> bool set_parameter(const std::string& name, const TParam& parameter);
  template <typename TParam> bool get_parameter(const std::string& name, TParam& parameter);

  bool get_system_info(msg::mediator::SystemInfo& info);

  void on_spin() override;

  static inline void set_socket_factory(SocketFactory factory) { socket_factory_ = factory; }
  static inline void set_id_factory(IDFactory factory) { id_factory_ = factory; }

private:
  Endpoint rixhub_endpoint_;
  msg::mediator::NodeInfo info_;
  std::vector<std::shared_ptr<Spinner>> components_;
  std::shared_ptr<GenericSocket> server_;
  std::atomic<bool> registered_flag_;
  static inline SocketFactory socket_factory_{create_socket};
  static inline IDFactory id_factory_{default_id_generator};

  std::shared_ptr<Publisher> create_publisher(const msg::mediator::TopicInfo& topic_info,
                                              const Endpoint& rixhub_endpoint,
                                              const Endpoint& endpoint);

  std::shared_ptr<Subscriber> create_subscriber(const msg::mediator::TopicInfo& topic_info,
                                                const Endpoint& rixhub_endpoint,
                                                const Endpoint& endpoint);

  std::shared_ptr<Service>
  create_service(msg::mediator::SrvInfo& service_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint);

  std::shared_ptr<ServiceClient> create_service_client(const msg::mediator::SrvRequest& service_request,
                                                       const Endpoint& rixhub_endpoint);

  std::shared_ptr<Action>
  create_action(msg::mediator::ActInfo& action_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint);

  std::shared_ptr<ActionClient> create_action_client(const msg::mediator::ActRequest& action_request,
                                                     const Endpoint& rixhub_endpoint);
};

template <typename TMsg>
std::shared_ptr<Publisher> Node::create_publisher(const std::string& topic, const Endpoint& endpoint) {
  static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be a subclass of msg::Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create publisher." << std::endl;
    return nullptr;
  }
  // Get topic information
  msg::mediator::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  // Invoke private implementation
  return create_publisher(topic_info, rixhub_endpoint_, endpoint);
}

template <typename TMsg>
std::shared_ptr<Subscriber>
Node::create_subscriber(const std::string& topic, Subscriber::Callback<TMsg> callback, const Endpoint& endpoint) {
  static_assert(std::is_base_of<msg::Message, TMsg>::value, "TMsg must be a subclass of msg::Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create subscriber." << std::endl;
    return nullptr;
  }

  // Get topic information
  msg::mediator::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  // Invoke private implementation
  auto sub = create_subscriber(topic_info, rixhub_endpoint_, endpoint);

  // Set callback (need template info to do this)
  if (sub) {
    sub->set_callback(callback);
  }
  return sub;
}

inline std::shared_ptr<TimerCallback> Node::create_timer(const Duration& d, TimerCallback::Callback callback) {
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create timer." << std::endl;
    return nullptr;
  }
  auto timer = std::make_shared<TimerCallback>(d, callback);
  components_.push_back(timer);
  return timer;
}

template <typename TRequest, typename TResponse>
std::shared_ptr<Service> Node::create_service(const std::string& service,
                                              Service::Callback<TRequest, TResponse> callback,
                                              const Endpoint& endpoint) {
  static_assert(std::is_base_of<msg::Message, TRequest>::value, "TRequest must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TResponse>::value, "TResponse must be a subclass of msg::Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create service." << std::endl;
    return nullptr;
  }

  msg::mediator::SrvInfo service_info;
  service_info.name = service;
  service_info.request_hash = TRequest().hash();
  service_info.response_hash = TResponse().hash();

  auto srv = create_service(service_info, rixhub_endpoint_, endpoint);
  if (srv) {
    srv->set_callback(callback);
  }
  return srv;
}

template <typename TGoal, typename TFeedback, typename TResult>
std::shared_ptr<Action> Node::create_action(const std::string& service,
                                            Action::Callback<TGoal, TFeedback, TResult> callback,
                                            const Endpoint& endpoint) {
  static_assert(std::is_base_of<msg::Message, TGoal>::value, "TGoal must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TFeedback>::value, "TFeedback must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TResult>::value, "TResult must be a subclass of msg::Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create service." << std::endl;
    return nullptr;
  }

  msg::mediator::ActInfo action_info;
  action_info.name = service;
  action_info.goal_hash = TGoal().hash();
  action_info.feedback_hash = TFeedback().hash();
  action_info.result_hash = TResult().hash();

  auto act = create_action(action_info, rixhub_endpoint_, endpoint);
  if (act) {
    act->set_callback(callback);
  }
  return act;
}

template <typename TRequest, typename TResponse>
std::shared_ptr<ServiceClient> Node::create_service_client(const std::string& service) {
  static_assert(std::is_base_of<msg::Message, TRequest>::value, "TRequest must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TResponse>::value, "TResponse must be a subclass of msg::Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create service client." << std::endl;
    return nullptr;
  }

  msg::mediator::SrvRequest service_request;
  service_request.name = service;
  service_request.node_id = info_.id;
  service_request.request_hash = TRequest().hash();
  service_request.response_hash = TResponse().hash();

  return create_service_client(service_request, rixhub_endpoint_);
}

template <typename TGoal, typename TFeedback, typename TResult>
std::shared_ptr<ActionClient> Node::create_action_client(const std::string& action) {
  static_assert(std::is_base_of<msg::Message, TGoal>::value, "TGoal must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TFeedback>::value, "TFeedback must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TResult>::value, "TResult must be a subclass of msg::Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create action client." << std::endl;
    return nullptr;
  }
  msg::mediator::ActRequest action_request;
  action_request.name = action;
  action_request.node_id = info_.id;
  action_request.goal_hash = TGoal().hash();
  action_request.feedback_hash = TFeedback().hash();
  action_request.result_hash = TResult().hash();
  return create_action_client(action_request, rixhub_endpoint_);
}

template <typename TParam> bool Node::set_parameter(const std::string& name, const TParam& parameter) {
  static_assert(std::is_base_of<msg::Message, TParam>::value, "TParam must be a subclass of msg::Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot set parameter." << std::endl;
    return false;
  }

  msg::mediator::ParamInfo info;
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

  msg::mediator::Operation op;
  msg::mediator::Status status;
  if (!client->recv_message(op, status)) {
    return false;
  }

  if (op.opcode != OPCODE::STATUS_RESPONSE) {
    return false;
  }

  return status.error == 0;
}

template <typename TParam> bool Node::get_parameter(const std::string& name, TParam& parameter) {
  static_assert(std::is_base_of<msg::Message, TParam>::value, "TParam must be a subclass of msg::Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot get parameter." << std::endl;
    return false;
  }

  msg::mediator::ParamInfo info;
  info.id = info_.id;
  info.name = name;
  info.message_hash = parameter.hash();
  msg::mediator::ParamInfo info_received;

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    return false;
  }
  if (!client->send_message(OPCODE::PARAM_GET_REQUEST, info)) {
    return false;
  }
  msg::mediator::Operation op;
  if (!client->recv_message(op, info_received)) {
    return false;
  }
  if (op.opcode != OPCODE::PARAM_GET_RESPONSE) {
    return false;
  }
  size_t offset = 0;
  return parameter.deserialize(info_received.data.data(), info_received.data.size(), offset);
}

} // namespace rix