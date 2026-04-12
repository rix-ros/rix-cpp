#pragma once

#include "rix/core/action.hpp"
#include "rix/core/action_client.hpp"
#include "rix/core/mediator_client.hpp"
#include "rix/core/publisher.hpp"
#include "rix/core/service.hpp"
#include "rix/core/service_client.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/core/timer_callback.hpp"

namespace rix {

class ComponentFactory {
public:
  virtual ~ComponentFactory() = default;

  virtual std::shared_ptr<MediatorClient>
  create_mediator_client(sys_msgs::NodeInfo& node_info, const Endpoint& endpoint, const Endpoint& rixhub_endpoint) = 0;

  /**
   * @brief Private implementation to create a Publisher.
   * @param topic_info The TopicInfo message containing topic details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the publisher.
   * @return A shared pointer to the created Publisher.
   */
  virtual std::shared_ptr<Publisher> create_publisher(const sys_msgs::PubInfo& pub_info,
                                                      const Endpoint& rixhub_endpoint) = 0;

  /**
   * @brief Private implementation to create a Subscriber.
   * @param topic_info The TopicInfo message containing topic details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the subscriber.
   * @return A shared pointer to the created Subscriber.
   */
  virtual std::shared_ptr<Subscriber> create_subscriber(const sys_msgs::SubInfo& sub_info,
                                                        const Endpoint& rixhub_endpoint) = 0;

  virtual std::shared_ptr<TimerCallback> create_timer(const Duration& d, const TimerCallback::Callback& callback) = 0;

  /**
   * @brief Private implementation to create a Service.
   * @param service_info The SrvInfo message containing service details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the service.
   * @return A shared pointer to the created Service.
   */
  virtual std::shared_ptr<Service> create_service(sys_msgs::SrvInfo& service_info, const Endpoint& rixhub_endpoint) = 0;

  /**
   * @brief Private implementation to create a ServiceClient.
   * @param service_request The SrvRequest message containing service request details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @return A shared pointer to the created ServiceClient.
   */
  virtual std::shared_ptr<ServiceClient> create_service_client(sys_msgs::SrvRequest& service_request,
                                                               const Endpoint& rixhub_endpoint) = 0;

  /**
   * @brief Private implementation to create an Action.
   * @param action_info The ActInfo message containing action details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the action.
   * @return A shared pointer to the created Action.
   */
  virtual std::shared_ptr<Action> create_action(sys_msgs::ActInfo& action_info, const Endpoint& rixhub_endpoint) = 0;

  /**
   * @brief Private implementation to create an ActionClient.
   * @param action_request The ActRequest message containing action request details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @return A shared pointer to the created ActionClient.
   */
  virtual std::shared_ptr<ActionClient> create_action_client(sys_msgs::ActRequest& action_request,
                                                             const Endpoint& rixhub_endpoint) = 0;
};

namespace detail {

class ComponentFactoryImpl : public ComponentFactory {
public:
  ComponentFactoryImpl() = default;

  // Disable copy and move semantics
  ComponentFactoryImpl(const ComponentFactoryImpl&) = delete;
  ComponentFactoryImpl& operator=(const ComponentFactoryImpl&) = delete;
  ComponentFactoryImpl(ComponentFactoryImpl&&) = delete;
  ComponentFactoryImpl& operator=(ComponentFactoryImpl&&) = delete;

  /**
   * @brief Destructor. Deregisters the node from rixhub and removes references to all components.
   */
  ~ComponentFactoryImpl() override = default;

  std::shared_ptr<MediatorClient> create_mediator_client(sys_msgs::NodeInfo& node_info,
                                                         const Endpoint& endpoint,
                                                         const Endpoint& rixhub_endpoint) override;

  /**
   * @brief Private implementation to create a Publisher.
   * @param topic_info The TopicInfo message containing topic details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the publisher.
   * @return A shared pointer to the created Publisher.
   */
  std::shared_ptr<Publisher> create_publisher(const sys_msgs::PubInfo& pub_info,
                                              const Endpoint& rixhub_endpoint) override;

  /**
   * @brief Private implementation to create a Subscriber.
   * @param topic_info The TopicInfo message containing topic details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the subscriber.
   * @return A shared pointer to the created Subscriber.
   */
  std::shared_ptr<Subscriber> create_subscriber(const sys_msgs::SubInfo& sub_info,
                                                const Endpoint& rixhub_endpoint) override;

  std::shared_ptr<TimerCallback> create_timer(const Duration& d, const TimerCallback::Callback& callback) override;

  /**
   * @brief Private implementation to create a Service.
   * @param service_info The SrvInfo message containing service details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the service.
   * @return A shared pointer to the created Service.
   */
  std::shared_ptr<Service> create_service(sys_msgs::SrvInfo& service_info, const Endpoint& rixhub_endpoint) override;

  /**
   * @brief Private implementation to create a ServiceClient.
   * @param service_request The SrvRequest message containing service request details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @return A shared pointer to the created ServiceClient.
   */
  std::shared_ptr<ServiceClient> create_service_client(sys_msgs::SrvRequest& service_request,
                                                       const Endpoint& rixhub_endpoint) override;

  /**
   * @brief Private implementation to create an Action.
   * @param action_info The ActInfo message containing action details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @param endpoint The endpoint for the action.
   * @return A shared pointer to the created Action.
   */
  std::shared_ptr<Action> create_action(sys_msgs::ActInfo& action_info, const Endpoint& rixhub_endpoint) override;

  /**
   * @brief Private implementation to create an ActionClient.
   * @param action_request The ActRequest message containing action request details.
   * @param rixhub_endpoint The RIXHub endpoint.
   * @return A shared pointer to the created ActionClient.
   */
  std::shared_ptr<ActionClient> create_action_client(sys_msgs::ActRequest& action_request,
                                                     const Endpoint& rixhub_endpoint) override;
};

} // namespace detail
} // namespace rix