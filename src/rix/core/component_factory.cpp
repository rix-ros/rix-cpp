#include "rix/core/component_factory.hpp"

namespace rix {
namespace detail {
std::shared_ptr<MediatorClient> ComponentFactoryImpl::create_mediator_client(sys_msgs::NodeInfo& node_info,
                                                                             const Endpoint& endpoint,
                                                                             const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<MediatorClient>(new detail::MediatorClientImpl(node_info, endpoint, rixhub_endpoint));
}

std::shared_ptr<TimerCallback> ComponentFactoryImpl::create_timer(const Duration& d,
                                                                  const TimerCallback::Callback& callback) {
  return std::shared_ptr<TimerCallback>(new detail::TimerCallbackImpl(d, callback));
}

std::shared_ptr<Publisher> ComponentFactoryImpl::create_publisher(const sys_msgs::PubInfo& pub_info,
                                                                  const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<Publisher>(new detail::PublisherImpl(pub_info, rixhub_endpoint));
}

std::shared_ptr<Subscriber> ComponentFactoryImpl::create_subscriber(const sys_msgs::SubInfo& sub_info,
                                                                    const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<Subscriber>(new detail::SubscriberImpl(sub_info, rixhub_endpoint));
}

std::shared_ptr<Service> ComponentFactoryImpl::create_service(sys_msgs::SrvInfo& service_info,
                                                              const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<Service>(new detail::ServiceImpl(service_info, rixhub_endpoint));
}

std::shared_ptr<Action> ComponentFactoryImpl::create_action(sys_msgs::ActInfo& action_info,
                                                            const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<Action>(new detail::ActionImpl(action_info, rixhub_endpoint));
}

std::shared_ptr<ServiceClient> ComponentFactoryImpl::create_service_client(sys_msgs::SrvRequest& service_request,
                                                                           const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<ServiceClient>(new detail::ServiceClientImpl(service_request, rixhub_endpoint));
}

std::shared_ptr<ActionClient> ComponentFactoryImpl::create_action_client(sys_msgs::ActRequest& action_request,
                                                                         const Endpoint& rixhub_endpoint) {
  return std::shared_ptr<ActionClient>(new detail::ActionClientImpl(action_request, rixhub_endpoint));
}

} // namespace detail
} // namespace rix