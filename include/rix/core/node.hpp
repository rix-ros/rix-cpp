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
#include "rix/core/publisher.hpp"
#include "rix/core/service.hpp"
#include "rix/core/service_client.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/core/timer_callback.hpp"
#include "rix/ipc/socket.hpp"
#include "rix/sys_msgs/NodeInfo.hpp"
#include "rix/sys_msgs/ParamInfo.hpp"
#include "rix/sys_msgs/Status.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node : public Spinner {
public:
  /**
   * @brief Constructs a Node with the given name and endpoint.
   * @param name The name of the node.
   * @param endpoint The endpoint of the node.
   */
  explicit Node(const std::string& name, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  // Disable copy and move semantics
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Node(Node&&) = delete;
  Node& operator=(Node&&) = delete;

  /**
   * @brief Destructor. Deregisters the node from rixhub and removes references to all components.
   */
  ~Node() override;

  /**
   * @brief Creates a Publisher for the given topic.
   * @param topic The name of the topic.
   * @param endpoint The endpoint for the publisher.
   * @return A shared pointer to the created Publisher.
   */
  template <typename TMsg>
  std::shared_ptr<Publisher> create_publisher(const std::string& topic,
                                              const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

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
                                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

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
  auto
  create_subscriber(const std::string& topic, Callback&& callback, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0))
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
                                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

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
  std::shared_ptr<ServiceClient> create_service_client(const std::string& service);

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
                                          const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

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
  auto
  create_service(const std::string& service, Callback&& callback, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0))
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
                                          const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

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
                                                     ActionClient::ResultCallback<TResult> result_callback);

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
                            ResultCallback&& result_callback)
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
                                                     Class* instance);

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
                                        const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

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
  auto create_action(const std::string& action, Callback&& callback, const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0))
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
                                        const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));

  /**
   * @brief Sets a parameter on the RIXHub parameter server.
   * @param name The name of the parameter.
   * @param parameter The parameter value to set.
   * @return true if the parameter was set successfully, false otherwise.
   */
  template <typename TParam> bool set_parameter(const std::string& name, const TParam& parameter);

  /**
   * @brief Gets a parameter from the RIXHub parameter server.
   * @param name The name of the parameter.
   * @param parameter The variable to store the retrieved parameter value.
   * @return true if the parameter was retrieved successfully, false otherwise.
   */
  template <typename TParam> bool get_parameter(const std::string& name, TParam& parameter);

  /**
   * @brief Retrieves system information from RIXHub.
   * @param info The SystemInfo message to populate with system information.
   * @return true if the system information was retrieved successfully, false otherwise.
   */
  bool get_system_info(sys_msgs::SystemInfo& info);

  /**
   * @brief Sets the socket factory used for creating sockets.
   * @param factory The socket factory function.
   */
  static inline void set_socket_factory(SocketFactory factory) { socket_factory_ = std::move(factory); }

  /**
   * @brief Sets the ID factory used for generating unique IDs.
   * @param factory The ID factory function.
   */
  static inline void set_id_factory(IDFactory factory) { id_factory_ = std::move(factory); }

private:
  static inline SocketFactory socket_factory_{create_socket}; ///< Socket factory function.
  static inline IDFactory id_factory_{default_id_generator};  ///< ID factory function.

  Endpoint rixhub_endpoint_;                         ///< The RIXHub endpoint.
  sys_msgs::NodeInfo info_;                          ///< Node information.
  std::vector<std::shared_ptr<Spinner>> components_; ///< All components created by the node.
  std::shared_ptr<GenericSocket> server_;            ///< Server socket for the node.
  std::atomic<bool> registered_flag_;                ///< Flag indicating if the node is registered with RIXHub.

  /**
   * @brief Internal spin implementation for the Node.
   */
  void on_spin() override;

  /**
   * @brief Private implementation to create a Publisher.
   * @param topic_info The TopicInfo message containing topic details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the publisher.
   * @return A shared pointer to the created Publisher.
   */
  std::shared_ptr<Publisher>
  create_publisher(const sys_msgs::TopicInfo& topic_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint);

  /**
   * @brief Private implementation to create a Subscriber.
   * @param topic_info The TopicInfo message containing topic details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the subscriber.
   * @return A shared pointer to the created Subscriber.
   */
  std::shared_ptr<Subscriber>
  create_subscriber(const sys_msgs::TopicInfo& topic_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint);

  /**
   * @brief Private implementation to create a Service.
   * @param service_info The SrvInfo message containing service details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the service.
   * @return A shared pointer to the created Service.
   */
  std::shared_ptr<Service>
  create_service(sys_msgs::SrvInfo& service_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint);

  /**
   * @brief Private implementation to create a ServiceClient.
   * @param service_request The SrvRequest message containing service request details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @return A shared pointer to the created ServiceClient.
   */
  std::shared_ptr<ServiceClient> create_service_client(const sys_msgs::SrvRequest& service_request,
                                                       const Endpoint& rixhub_endpoint);

  /**
   * @brief Private implementation to create an Action.
   * @param action_info The ActInfo message containing action details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the action.
   * @return A shared pointer to the created Action.
   */
  std::shared_ptr<Action>
  create_action(sys_msgs::ActInfo& action_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint);

  /**
   * @brief Private implementation to create an ActionClient.
   * @param action_request The ActRequest message containing action request details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @return A shared pointer to the created ActionClient.
   */
  std::shared_ptr<ActionClient> create_action_client(const sys_msgs::ActRequest& action_request,
                                                     const Endpoint& rixhub_endpoint);
};

template <typename TMsg>
std::shared_ptr<Publisher> Node::create_publisher(const std::string& topic, const Endpoint& endpoint) {
  static_assert(std::is_base_of<Message, TMsg>::value, "TMsg must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create publisher." << std::endl;
    return nullptr;
  }
  // Get topic information
  sys_msgs::TopicInfo topic_info;
  topic_info.name = topic;
  topic_info.message_hash = TMsg().hash();

  // Invoke private implementation
  return create_publisher(topic_info, rixhub_endpoint_, endpoint);
}

template <typename TMsg>
std::shared_ptr<Subscriber>
Node::create_subscriber(const std::string& topic, Subscriber::Callback<TMsg> callback, const Endpoint& endpoint) {
  static_assert(std::is_base_of<Message, TMsg>::value, "TMsg must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create subscriber." << std::endl;
    return nullptr;
  }

  // Get topic information
  sys_msgs::TopicInfo topic_info;
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

template <typename Callback>
auto Node::create_subscriber(const std::string& topic, Callback&& callback, const Endpoint& endpoint)
    -> std::enable_if_t<
        !std::is_same<
            std::decay_t<Callback>,
            Subscriber::Callback<typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType>>::value,
        std::shared_ptr<Subscriber>> {
  using TMsg = typename SubscriberCallbackTraits<std::decay_t<Callback>>::MessageType;
  return create_subscriber<TMsg>(topic, std::forward<Callback>(callback), endpoint);
}

template <typename TMsg, typename Class>
std::shared_ptr<Subscriber> Node::create_subscriber(const std::string& topic,
                                                    void (Class::*callback)(const TMsg&),
                                                    Class* instance,
                                                    const Endpoint& endpoint) {
  return create_subscriber<TMsg>(
      topic, [instance, callback](const TMsg& msg) { (instance->*callback)(msg); }, endpoint);
}

inline std::shared_ptr<TimerCallback> Node::create_timer(const Duration& d, const TimerCallback::Callback& callback) {
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create timer." << std::endl;
    return nullptr;
  }
  auto timer = std::shared_ptr<TimerCallback>(new TimerCallback(d, callback));
  components_.push_back(timer);
  return timer;
}

template <typename Class>
std::shared_ptr<TimerCallback>
Node::create_timer(const Duration& d, void (Class::*callback)(const TimerCallback::Event&), Class* instance) {
  return create_timer(d, [instance, callback](const TimerCallback::Event& event) { (instance->*callback)(event); });
}

template <typename TRequest, typename TResponse>
std::shared_ptr<Service> Node::create_service(const std::string& service,
                                              Service::Callback<TRequest, TResponse> callback,
                                              const Endpoint& endpoint) {
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

  auto srv = create_service(service_info, rixhub_endpoint_, endpoint);
  if (srv) {
    srv->set_callback(callback);
  }
  return srv;
}

template <typename Callback>
auto Node::create_service(const std::string& service, Callback&& callback, const Endpoint& endpoint)
    -> std::enable_if_t<
        !std::is_same<std::decay_t<Callback>,
                      Service::Callback<typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType,
                                        typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType>>::value,
        std::shared_ptr<Service>> {
  using TRequest = typename ServiceCallbackTraits<std::decay_t<Callback>>::RequestType;
  using TResponse = typename ServiceCallbackTraits<std::decay_t<Callback>>::ResponseType;
  return create_service<TRequest, TResponse>(service, std::forward<Callback>(callback), endpoint);
}

template <typename TRequest, typename TResponse, typename Class>
std::shared_ptr<Service> Node::create_service(const std::string& service,
                                              void (Class::*callback)(const TRequest&, TResponse&),
                                              Class* instance,
                                              const Endpoint& endpoint) {
  return create_service<TRequest, TResponse>(
      service,
      [instance, callback](const TRequest& req, TResponse& resp) { (instance->*callback)(req, resp); },
      endpoint);
}

template <typename TGoal, typename TFeedback, typename TResult>
std::shared_ptr<Action> Node::create_action(const std::string& action,
                                            Action::Callback<TGoal, TFeedback, TResult> callback,
                                            const Endpoint& endpoint) {
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

  auto act = create_action(action_info, rixhub_endpoint_, endpoint);
  if (act) {
    act->set_callback(callback);
  }
  return act;
}

template <typename Callback>
auto Node::create_action(const std::string& action, Callback&& callback, const Endpoint& endpoint) -> std::enable_if_t<
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

template <typename TGoal, typename TFeedback, typename TResult, typename Class>
std::shared_ptr<Action> Node::create_action(const std::string& action,
                                            bool (Class::*callback)(const TGoal&, TFeedback&, TResult&),
                                            Class* instance,
                                            const Endpoint& endpoint) {
  return create_action<TGoal, TFeedback, TResult>(
      action,
      [instance, callback](const TGoal& goal, TFeedback& feedback, TResult& result) {
        return (instance->*callback)(goal, feedback, result);
      },
      endpoint);
}

template <typename TRequest, typename TResponse>
std::shared_ptr<ServiceClient> Node::create_service_client(const std::string& service) {
  static_assert(std::is_base_of<Message, TRequest>::value, "TRequest must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TResponse>::value, "TResponse must be a subclass of Message.");

  if (!ok()) {
    Log::error << "Node is shutdown, cannot create service client." << std::endl;
    return nullptr;
  }

  sys_msgs::SrvRequest service_request;
  service_request.name = service;
  service_request.node_id = info_.id;
  service_request.request_hash = TRequest().hash();
  service_request.response_hash = TResponse().hash();

  return create_service_client(service_request, rixhub_endpoint_);
}

template <typename TGoal, typename TFeedback, typename TResult>
std::shared_ptr<ActionClient> Node::create_action_client(const std::string& action,
                                                         ActionClient::FeedbackCallback<TFeedback> feedback_callback,
                                                         ActionClient::ResultCallback<TResult> result_callback) {
  static_assert(std::is_base_of<Message, TGoal>::value, "TGoal must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TFeedback>::value, "TFeedback must be a subclass of Message.");
  static_assert(std::is_base_of<Message, TResult>::value, "TResult must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create action client." << std::endl;
    return nullptr;
  }
  sys_msgs::ActRequest action_request;
  action_request.name = action;
  action_request.node_id = info_.id;
  action_request.goal_hash = TGoal().hash();
  action_request.feedback_hash = TFeedback().hash();
  action_request.result_hash = TResult().hash();
  auto act_cli = create_action_client(action_request, rixhub_endpoint_);
  if (act_cli) {
    act_cli->set_feedback_callback(feedback_callback);
    act_cli->set_result_callback(result_callback);
  }
  return act_cli;
}

template <typename TGoal, typename FeedbackCallback, typename ResultCallback>
auto Node::create_action_client(const std::string& action,
                                FeedbackCallback&& feedback_callback,
                                ResultCallback&& result_callback)
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
  return create_action_client<TGoal, TFeedback, TResult>(
      action, std::forward<FeedbackCallback>(feedback_callback), std::forward<ResultCallback>(result_callback));
}

template <typename TGoal, typename TFeedback, typename TResult, typename Class>
std::shared_ptr<ActionClient> Node::create_action_client(const std::string& action,
                                                         void (Class::*feedback_callback)(const TFeedback&),
                                                         void (Class::*result_callback)(const TResult&),
                                                         Class* instance) {
  return create_action_client<TGoal, TFeedback, TResult>(
      action,
      [instance, feedback_callback](const TFeedback& feedback) { (instance->*feedback_callback)(feedback); },
      [instance, result_callback](const TResult& result) { (instance->*result_callback)(result); });
}

template <typename TParam> bool Node::set_parameter(const std::string& name, const TParam& parameter) {
  static_assert(std::is_base_of<Message, TParam>::value, "TParam must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot set parameter." << std::endl;
    return false;
  }

  sys_msgs::ParamInfo info;
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

  sys_msgs::Operation operation;
  sys_msgs::Status status;
  if (!client->recv_message(operation, status)) {
    return false;
  }

  if (operation.opcode != OPCODE::STATUS_RESPONSE) {
    return false;
  }

  return status.error == 0;
}

template <typename TParam> bool Node::get_parameter(const std::string& name, TParam& parameter) {
  static_assert(std::is_base_of<Message, TParam>::value, "TParam must be a subclass of Message.");
  if (!ok()) {
    Log::error << "Node is shutdown, cannot get parameter." << std::endl;
    return false;
  }

  sys_msgs::ParamInfo info;
  info.id = info_.id;
  info.name = name;
  info.message_hash = parameter.hash();
  sys_msgs::ParamInfo info_received;

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    return false;
  }
  if (!client->send_message(OPCODE::PARAM_GET_REQUEST, info)) {
    return false;
  }
  sys_msgs::Operation operation;
  if (!client->recv_message(operation, info_received)) {
    return false;
  }
  if (operation.opcode != OPCODE::PARAM_GET_RESPONSE) {
    return false;
  }
  size_t offset = 0;
  return parameter.deserialize(info_received.data.data(), info_received.data.size(), offset);
}

} // namespace rix