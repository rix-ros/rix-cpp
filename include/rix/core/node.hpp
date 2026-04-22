#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include "rix/core/action.hpp"
#include "rix/core/action_client.hpp"
#include "rix/core/callback_traits.hpp"
#include "rix/core/common.hpp"
#include "rix/core/component_factory.hpp"
#include "rix/core/mediator_client.hpp"
#include "rix/core/publisher.hpp"
#include "rix/core/service.hpp"
#include "rix/core/service_client.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/core/timer_callback.hpp"
#include "rix/ipc.hpp"
#include "rix/sys_msgs/NodeInfo.hpp"
#include "rix/sys_msgs/ParamInfo.hpp"
#include "rix/sys_msgs/Status.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/util.hpp"

namespace rix {

class Node : public Spinner {
public:
  /**
   * @brief Constructs a Node with the given name and endpoint.
   * @param name The name of the node.
   * @param endpoint The endpoint of the node.
   * @param rixhub_endpoint The endpoint of the RIXHub mediator to connect to.
   */
  explicit Node(const std::string& name,
                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                const Endpoint& rixhub_endpoint = Endpoint(RIXHUB_IP, RIXHUB_PORT));

  virtual ~Node();

  // TODO: Create private helper methods that do not require template parameters to call from header file to hide
  //       the implementation details.

  /**
   * @brief Creates a Publisher for the given topic.
   * @param topic The name of the topic.
   * @param endpoint The endpoint for the publisher.
   * @return A shared pointer to the created Publisher.
   */
  template <typename TMsg>
  std::shared_ptr<Publisher> create_publisher(const std::string& topic,
                                              const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                              TransportOptions options = {});

  /**
   * @brief Creates a Subscriber for the given topic with the specified callback.
   * @param topic The name of the topic.
   * @param callback The callback function to be invoked on message receipt.
   * @param endpoint The endpoint for the subscriber.
   * @return A shared pointer to the created Subscriber.
   */
  template <typename TMsg>
  std::shared_ptr<Subscriber> create_subscriber(const std::string& topic,
                                                Subscriber::Callback<TMsg> callback,
                                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                                TransportOptions options = {});

  /**
   * @brief Creates a Subscriber for the given topic with the specified callback.
   * @param topic The name of the topic.
   * @param callback The callback function to be invoked on message receipt.
   * @param endpoint The endpoint for the subscriber.
   *
   * @details This allows the deduction of the message type from the callback function.
   *
   * @return A shared pointer to the created Subscriber.
   */
  template <typename Callback>
  auto create_subscriber(const std::string& topic,
                         Callback&& callback,
                         const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                         TransportOptions options = {})
      -> std::enable_if_t<
          !std::is_same<
              std::decay_t<Callback>,
              Subscriber::Callback<typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType>>::value,
          std::shared_ptr<Subscriber>>;

  /**
   * @brief Creates a Subscriber for the given topic with the specified member function callback.
   * @param topic The name of the topic.
   * @param callback The member function callback to be invoked on message receipt.
   * @param instance The instance of the class containing the member function.
   * @param endpoint The endpoint for the subscriber.
   *
   * @details This is a convenience method to create subscribers using class member functions as callbacks.
   *
   * @return A shared pointer to the created Subscriber.
   */
  template <typename TMsg, typename Class>
  std::shared_ptr<Subscriber> create_subscriber(const std::string& topic,
                                                void (Class::*callback)(const TMsg&),
                                                Class* instance,
                                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                                TransportOptions options = {});

  /**
   * @brief Creates a TimerCallback with the specified duration and callback function.
   * @param d The duration for the timer.
   * @param callback The callback function to be invoked on timer events.
   * @return A shared pointer to the created TimerCallback.
   */
  std::shared_ptr<TimerCallback> create_timer(const Duration& d, const TimerCallback::Callback& callback);

  /**
   * @brief Creates a TimerCallback with the specified duration and member function callback.
   * @param d The duration for the timer.
   * @param callback The member function callback to be invoked on timer events.
   * @param instance The instance of the class containing the member function.
   *
   * @details This is a convenience method to create timer callbacks using class member functions.
   *
   * @return A shared pointer to the created TimerCallback.
   */
  template <typename Class>
  std::shared_ptr<TimerCallback>
  create_timer(const Duration& d, void (Class::*callback)(const TimerCallback::Event&), Class* instance);

  /**
   * @brief Creates a ServiceClient for the given service.
   * @param service The name of the service.
   * @return A shared pointer to the created ServiceClient.
   */
  template <typename TRequest, typename TResponse>
  std::shared_ptr<ServiceClient> create_service_client(const std::string& service, TransportOptions options = {});

  /**
   * @brief Creates a Service for the given service with the specified callback.
   * @param service The name of the service.
   * @param callback The callback function to be invoked on service requests.
   * @param endpoint The endpoint for the service.
   * @return A shared pointer to the created Service.
   */
  template <typename TRequest, typename TResponse>
  std::shared_ptr<Service> create_service(const std::string& service,
                                          Service::Callback<TRequest, TResponse> callback,
                                          const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                          TransportOptions options = {});

  /**
   * @brief Creates a Service for the given service with the specified callback.
   * @param service The name of the service.
   * @param callback The callback function to be invoked on service requests.
   * @param endpoint The endpoint for the service.
   *
   * @details This allows the deduction of the request and response types from the callback function.
   *
   * @return A shared pointer to the created Service.
   */
  template <typename Callback>
  auto create_service(const std::string& service,
                      Callback&& callback,
                      const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                      TransportOptions options = {})
      -> std::enable_if_t<
          !std::is_same<std::decay_t<Callback>,
                        Service::Callback<typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType,
                                          typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType>>::value,
          std::shared_ptr<Service>>;

  /**
   * @brief Creates a Service for the given service with the specified member function callback.
   * @param service The name of the service.
   * @param callback The member function callback to be invoked on service requests.
   * @param instance The instance of the class containing the member function.
   * @param endpoint The endpoint for the service.
   *
   * @details This is a convenience method to create services using class member functions as callbacks.
   *
   * @return A shared pointer to the created Service.
   */
  template <typename TRequest, typename TResponse, typename Class>
  std::shared_ptr<Service> create_service(const std::string& service,
                                          void (Class::*callback)(const TRequest&, TResponse&),
                                          Class* instance,
                                          const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                          TransportOptions options = {});

  /**
   * @brief Creates an ActionClient for the given action with specified feedback and result callbacks.
   * @param action The name of the action.
   * @param feedback_callback The callback function to be invoked on feedback receipt.
   * @param result_callback The callback function to be invoked on result receipt.
   * @return A shared pointer to the created ActionClient.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  std::shared_ptr<ActionClient> create_action_client(const std::string& action,
                                                     ActionClient::FeedbackCallback<TFeedback> feedback_callback,
                                                     ActionClient::ResultCallback<TResult> result_callback,
                                                     TransportOptions options = {});

  /**
   * @brief Creates an ActionClient for the given action with specified feedback and result callbacks.
   * @param action The name of the action.
   * @param feedback_callback The callback function to be invoked on feedback receipt.
   * @param result_callback The callback function to be invoked on result receipt.
   *
   * @details This allows the deduction of the feedback and result types from the callback functions.
   *
   * @return A shared pointer to the created ActionClient.
   */
  template <typename TGoal, typename FeedbackCallback, typename ResultCallback>
  auto create_action_client(const std::string& action,
                            FeedbackCallback&& feedback_callback,
                            ResultCallback&& result_callback,
                            TransportOptions options = {})
      -> std::enable_if_t<!std::is_same<std::decay_t<FeedbackCallback>,
                                        ActionClient::FeedbackCallback<typename ActionClientCallbackTraits<
                                            std::decay_t<FeedbackCallback>>::FeedbackType>>::value &&
                              !std::is_same<std::decay_t<ResultCallback>,
                                            ActionClient::ResultCallback<typename ActionClientCallbackTraits<
                                                std::decay_t<ResultCallback>>::ResultType>>::value,
                          std::shared_ptr<ActionClient>>;

  /**
   * @brief Creates an ActionClient for the given action with specified member function callbacks.
   * @param action The name of the action.
   * @param feedback_callback The member function callback to be invoked on feedback receipt.
   * @param result_callback The member function callback to be invoked on result receipt.
   * @param instance The instance of the class containing the member functions.
   *
   * @details This is a convenience method to create action clients using class member functions as callbacks.
   *
   * @return A shared pointer to the created ActionClient.
   */
  template <typename TGoal, typename TFeedback, typename TResult, typename Class>
  std::shared_ptr<ActionClient> create_action_client(const std::string& action,
                                                     void (Class::*feedback_callback)(const TFeedback&),
                                                     void (Class::*result_callback)(const TResult&),
                                                     Class* instance,
                                                     TransportOptions options = {});

  /**
   * @brief Creates an Action for the given action with the specified callback.
   * @param action The name of the action.
   * @param callback The callback function to be invoked on action goals.
   * @param endpoint The endpoint for the action.
   * @return A shared pointer to the created Action.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  std::shared_ptr<Action> create_action(const std::string& action,
                                        Action::Callback<TGoal, TFeedback, TResult> callback,
                                        const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                        TransportOptions options = {});

  /**
   * @brief Creates an Action for the given action with the specified callback.
   * @param action The name of the action.
   * @param callback The callback function to be invoked on action goals.
   * @param endpoint The endpoint for the action.
   *
   * @details This allows the deduction of the goal, feedback, and result types from the callback function.
   *
   * @return A shared pointer to the created Action.
   */

  template <typename Callback>
  auto create_action(const std::string& action,
                     Callback&& callback,
                     const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                     TransportOptions options = {})
      -> std::enable_if_t<
          !std::is_same<std::decay_t<Callback>,
                        Action::Callback<typename ActionCallbackTraits<std::decay_t<Callback>>::GoalType,
                                         typename ActionCallbackTraits<std::decay_t<Callback>>::FeedbackType,
                                         typename ActionCallbackTraits<std::decay_t<Callback>>::ResultType>>::value,
          std::shared_ptr<Action>>;

  /**
   * @brief Creates an Action for the given action with the specified member function callback.
   * @param action The name of the action.
   * @param callback The member function callback to be invoked on action goals.
   * @param instance The instance of the class containing the member function.
   * @param endpoint The endpoint for the action.
   *
   * @details This is a convenience method to create actions using class member functions as callbacks.
   *
   * @return A shared pointer to the created Action.
   */

  template <typename TGoal, typename TFeedback, typename TResult, typename Class>
  std::shared_ptr<Action> create_action(const std::string& action,
                                        bool (Class::*callback)(const TGoal&, TFeedback&, TResult&),
                                        Class* instance,
                                        const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0),
                                        TransportOptions options = {});

  /**
   * @brief Sets a parameter on the RIXHub parameter server.
   * @param name The name of the parameter.
   * @param parameter The parameter value to set.
   * @return true if the parameter was set successfully, false otherwise.
   */
  bool set_parameter(const std::string& name, const Message& parameter) const;

  /**
   * @brief Gets a parameter from the RIXHub parameter server.
   * @param name The name of the parameter.
   * @param parameter The variable to store the retrieved parameter value.
   * @return true if the parameter was retrieved successfully, false otherwise.
   */
  bool get_parameter(const std::string& name, Message& parameter) const;

  /**
   * @brief Retrieves system information from RIXHub.
   * @param info The SystemInfo message to populate with system information.
   * @return true if the system information was retrieved successfully, false otherwise.
   */
  bool get_system_info(sys_msgs::SystemInfo& info);

  /**
   * @brief Sets the ID factory used for generating unique IDs.
   * @param factory The ID factory function.
   */
  static inline void set_id_factory(IDFactory factory) { id_factory_ = std::move(factory); }
  static inline void set_component_factory(std::shared_ptr<ComponentFactory> factory) { factory_ = factory; }

protected:
private:
  static inline IDFactory id_factory_{default_id_generator}; ///< ID factory function.
  static inline std::shared_ptr<ComponentFactory> factory_{
      std::make_shared<detail::ComponentFactoryImpl>()}; ///< Component factory instance.
  Endpoint rixhub_endpoint_;                             ///< The RIXHub endpoint.
  sys_msgs::NodeInfo info_;                              ///< Node information.
  std::shared_ptr<MediatorClient> mediator_client_;      ///< Client for communicating with the RIXHub mediator.
  std::vector<std::shared_ptr<Spinner>> components_;     ///< All components created by the node.

  /**
   * @brief Internal spin implementation for the Node.
   */
  void on_spin() override;
};

template <typename TMsg>
std::shared_ptr<Publisher>
Node::create_publisher(const std::string& topic, const Endpoint& endpoint, TransportOptions options) {
  static_assert(std::is_base_of<Message, TMsg>::value, "TMsg must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create publisher." << std::endl;
    return nullptr;
  }
  sys_msgs::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  sys_msgs::PubInfo pub_info;
  pub_info.id = id_factory_();
  pub_info.node_id = info_.id;
  pub_info.topic_info = topic_info;
  pub_info.endpoint.address = endpoint.address;
  pub_info.endpoint.port = endpoint.port;
  pub_info.protocol = options.protocol;

  ScopedTransportOverride guard(options.protocol, options.build_factory());
  auto pub = factory_->create_publisher(pub_info, rixhub_endpoint_);
  if (pub) {
    components_.push_back(pub);
  }
  return pub;
}

template <typename TMsg>
std::shared_ptr<Subscriber> Node::create_subscriber(const std::string& topic,
                                                    Subscriber::Callback<TMsg> callback,
                                                    const Endpoint& endpoint,
                                                    TransportOptions options) {
  static_assert(std::is_base_of<Message, TMsg>::value, "TMsg must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create subscriber." << std::endl;
    return nullptr;
  }

  sys_msgs::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  sys_msgs::SubInfo sub_info;
  sub_info.id = id_factory_();
  sub_info.node_id = info_.id;
  sub_info.topic_info = topic_info;
  sub_info.endpoint.address = endpoint.address;
  sub_info.endpoint.port = endpoint.port;
  sub_info.protocol = options.protocol;

  ScopedTransportOverride guard(options.protocol, options.build_factory());
  auto sub = factory_->create_subscriber(sub_info, rixhub_endpoint_);
  if (sub) {
    sub->set_callback(callback);
    components_.push_back(sub);
  }
  return sub;
}

template <typename Callback>
auto Node::create_subscriber(const std::string& topic,
                             Callback&& callback,
                             const Endpoint& endpoint,
                             TransportOptions options)
    -> std::enable_if_t<
        !std::is_same<
            std::decay_t<Callback>,
            Subscriber::Callback<typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType>>::value,
        std::shared_ptr<Subscriber>> {
  using TMsg = typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType;
  return create_subscriber<TMsg>(topic, std::forward<Callback>(callback), endpoint, options);
}

template <typename TMsg, typename Class>
std::shared_ptr<Subscriber> Node::create_subscriber(const std::string& topic,
                                                    void (Class::*callback)(const TMsg&),
                                                    Class* instance,
                                                    const Endpoint& endpoint,
                                                    TransportOptions options) {
  return create_subscriber<TMsg>(
      topic, [instance, callback](const TMsg& msg) { (instance->*callback)(msg); }, endpoint, options);
}

template <typename Class>
std::shared_ptr<TimerCallback>
Node::create_timer(const Duration& d, void (Class::*callback)(const TimerCallback::Event&), Class* instance) {
  return create_timer(d, [instance, callback](const TimerCallback::Event& event) { (instance->*callback)(event); });
}

template <typename TRequest, typename TResponse>
std::shared_ptr<Service> Node::create_service(const std::string& service,
                                              Service::Callback<TRequest, TResponse> callback,
                                              const Endpoint& endpoint,
                                              TransportOptions options) {
  static_assert(std::is_base_of<Message, TRequest>::value, "TRequest must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TResponse>::value, "TResponse must be a subclass of Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create service." << std::endl;
    return nullptr;
  }

  sys_msgs::SrvInfo service_info;
  service_info.name = service;
  service_info.request_hash = TRequest().hash();
  service_info.response_hash = TResponse().hash();
  service_info.id = id_factory_();
  service_info.node_id = info_.id;
  service_info.endpoint.address = endpoint.address;
  service_info.endpoint.port = endpoint.port;
  service_info.protocol = options.protocol;

  ScopedTransportOverride guard(options.protocol, options.build_factory());
  auto srv = factory_->create_service(service_info, rixhub_endpoint_);
  if (srv) {
    srv->set_callback(callback);
    components_.push_back(srv);
  }
  return srv;
}

template <typename Callback>
auto Node::create_service(const std::string& service,
                          Callback&& callback,
                          const Endpoint& endpoint,
                          TransportOptions options)
    -> std::enable_if_t<
        !std::is_same<std::decay_t<Callback>,
                      Service::Callback<typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType,
                                        typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType>>::value,
        std::shared_ptr<Service>> {
  using TRequest = typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType;
  using TResponse = typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType;
  return create_service<TRequest, TResponse>(service, std::forward<Callback>(callback), endpoint, options);
}

template <typename TRequest, typename TResponse, typename Class>
std::shared_ptr<Service> Node::create_service(const std::string& service,
                                              void (Class::*callback)(const TRequest&, TResponse&),
                                              Class* instance,
                                              const Endpoint& endpoint,
                                              TransportOptions options) {
  return create_service<TRequest, TResponse>(
      service,
      [instance, callback](const TRequest& req, TResponse& resp) { (instance->*callback)(req, resp); },
      endpoint,
      options);
}

template <typename TGoal, typename TFeedback, typename TResult>
std::shared_ptr<Action> Node::create_action(const std::string& action,
                                            Action::Callback<TGoal, TFeedback, TResult> callback,
                                            const Endpoint& endpoint,
                                            TransportOptions options) {
  static_assert(std::is_base_of<Message, TGoal>::value, "TGoal must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TFeedback>::value, "TFeedback must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TResult>::value, "TResult must be a subclass of Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create action." << std::endl;
    return nullptr;
  }

  sys_msgs::ActInfo action_info;
  action_info.name = action;
  action_info.goal_hash = TGoal().hash();
  action_info.feedback_hash = TFeedback().hash();
  action_info.result_hash = TResult().hash();
  action_info.id = id_factory_();
  action_info.node_id = info_.id;
  action_info.endpoint.address = endpoint.address;
  action_info.endpoint.port = endpoint.port;
  action_info.protocol = options.protocol;

  ScopedTransportOverride guard(options.protocol, options.build_factory());
  auto act = factory_->create_action(action_info, rixhub_endpoint_);
  if (act) {
    act->set_callback(callback);
    components_.push_back(act);
  }
  return act;
}

template <typename Callback>
auto Node::create_action(const std::string& action,
                         Callback&& callback,
                         const Endpoint& endpoint,
                         TransportOptions options)
    -> std::enable_if_t<
        !std::is_same<std::decay_t<Callback>,
                      Action::Callback<typename ActionCallbackTraits<std::decay_t<Callback>>::GoalType,
                                       typename ActionCallbackTraits<std::decay_t<Callback>>::FeedbackType,
                                       typename ActionCallbackTraits<std::decay_t<Callback>>::ResultType>>::value,
        std::shared_ptr<Action>> {
  using TGoal = typename ActionCallbackTraits<std::decay_t<Callback>>::GoalType;
  using TFeedback = typename ActionCallbackTraits<std::decay_t<Callback>>::FeedbackType;
  using TResult = typename ActionCallbackTraits<std::decay_t<Callback>>::ResultType;
  return create_action<TGoal, TFeedback, TResult>(action, std::forward<Callback>(callback), endpoint, options);
}

template <typename TGoal, typename TFeedback, typename TResult, typename Class>
std::shared_ptr<Action> Node::create_action(const std::string& action,
                                            bool (Class::*callback)(const TGoal&, TFeedback&, TResult&),
                                            Class* instance,
                                            const Endpoint& endpoint,
                                            TransportOptions options) {
  return create_action<TGoal, TFeedback, TResult>(
      action,
      [instance, callback](const TGoal& goal, TFeedback& feedback, TResult& result) {
        return (instance->*callback)(goal, feedback, result);
      },
      endpoint,
      options);
}

template <typename TRequest, typename TResponse>
std::shared_ptr<ServiceClient> Node::create_service_client(const std::string& service, TransportOptions options) {
  static_assert(std::is_base_of<Message, TRequest>::value, "TRequest must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TResponse>::value, "TResponse must be a subclass of Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create service client." << std::endl;
    return nullptr;
  }

  sys_msgs::SrvRequest service_request;
  service_request.name = service;
  service_request.request_hash = TRequest().hash();
  service_request.response_hash = TResponse().hash();
  service_request.node_id = info_.id;
  ScopedTransportOverride guard(options.protocol, options.build_factory());
  auto srv_cli = factory_->create_service_client(service_request, rixhub_endpoint_);
  if (srv_cli) {
    components_.push_back(srv_cli);
  }
  return srv_cli;
}

template <typename TGoal, typename TFeedback, typename TResult>
std::shared_ptr<ActionClient> Node::create_action_client(const std::string& action,
                                                         ActionClient::FeedbackCallback<TFeedback> feedback_callback,
                                                         ActionClient::ResultCallback<TResult> result_callback,
                                                         TransportOptions options) {
  static_assert(std::is_base_of<Message, TGoal>::value, "TGoal must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TFeedback>::value, "TFeedback must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TResult>::value, "TResult must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create action client." << std::endl;
    return nullptr;
  }
  sys_msgs::ActRequest action_request;
  action_request.name = action;
  action_request.goal_hash = TGoal().hash();
  action_request.feedback_hash = TFeedback().hash();
  action_request.result_hash = TResult().hash();
  action_request.node_id = info_.id;
  ScopedTransportOverride guard(options.protocol, options.build_factory());
  auto act_cli = factory_->create_action_client(action_request, rixhub_endpoint_);
  if (act_cli) {
    act_cli->set_feedback_callback(feedback_callback);
    act_cli->set_result_callback(result_callback);
    components_.push_back(act_cli);
  }
  return act_cli;
}

template <typename TGoal, typename FeedbackCallback, typename ResultCallback>
auto Node::create_action_client(const std::string& action,
                                FeedbackCallback&& feedback_callback,
                                ResultCallback&& result_callback,
                                TransportOptions options)
    -> std::enable_if_t<
        !std::is_same<std::decay_t<FeedbackCallback>,
                      ActionClient::FeedbackCallback<
                          typename ActionClientCallbackTraits<std::decay_t<FeedbackCallback>>::FeedbackType>>::value &&
            !std::is_same<std::decay_t<ResultCallback>,
                          ActionClient::ResultCallback<
                              typename ActionClientCallbackTraits<std::decay_t<ResultCallback>>::ResultType>>::value,
        std::shared_ptr<ActionClient>> {
  using TFeedback = typename ActionClientCallbackTraits<std::decay_t<FeedbackCallback>>::FeedbackType;
  using TResult = typename ActionClientCallbackTraits<std::decay_t<ResultCallback>>::ResultType;
  return create_action_client<TGoal, TFeedback, TResult>(action,
                                                         std::forward<FeedbackCallback>(feedback_callback),
                                                         std::forward<ResultCallback>(result_callback),
                                                         options);
}

template <typename TGoal, typename TFeedback, typename TResult, typename Class>
std::shared_ptr<ActionClient> Node::create_action_client(const std::string& action,
                                                         void (Class::*feedback_callback)(const TFeedback&),
                                                         void (Class::*result_callback)(const TResult&),
                                                         Class* instance,
                                                         TransportOptions options) {
  return create_action_client<TGoal, TFeedback, TResult>(
      action,
      [instance, feedback_callback](const TFeedback& feedback) { (instance->*feedback_callback)(feedback); },
      [instance, result_callback](const TResult& result) { (instance->*result_callback)(result); },
      options);
}

} // namespace rix