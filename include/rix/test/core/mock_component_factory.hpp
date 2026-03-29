#pragma once

#include "rix/core/node.hpp"
#include "rix/sys_msgs/ActInfo.hpp"
#include "rix/sys_msgs/ActRequest.hpp"
#include "rix/sys_msgs/SrvInfo.hpp"
#include "rix/sys_msgs/SrvRequest.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/sys_msgs/TopicInfo.hpp"
#include "rix/test/core/mock_action.hpp"
#include "rix/test/core/mock_action_client.hpp"
#include "rix/test/core/mock_mediator_client.hpp"
#include "rix/test/core/mock_publisher.hpp"
#include "rix/test/core/mock_service.hpp"
#include "rix/test/core/mock_service_client.hpp"
#include "rix/test/core/mock_subscriber.hpp"
#include "rix/test/core/mock_timer_callback.hpp"
#include "rix/util/time.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace rix {
namespace test {

class MockComponentFactoryImpl : public ComponentFactory {
public:
  static void set_should_fail() { should_fail_ = true; }
  static void reset_fail() { should_fail_ = false; }

  std::shared_ptr<MediatorClient> create_mediator_client(sys_msgs::NodeInfo& node_info,
                                                         const Endpoint& endpoint,
                                                         const Endpoint& rixhub_endpoint) override {
    if (should_fail_) {
      return nullptr;
    }
    return std::make_shared<MockMediatorClientImpl>(node_info, endpoint, rixhub_endpoint);
  }

  std::shared_ptr<Publisher> create_publisher(const sys_msgs::PubInfo& pub_info,
                                              const Endpoint& /*endpoint*/) override {
    if (should_fail_) {
      return nullptr;
    }
    auto pub = std::make_shared<MockPublisher>(pub_info.topic_info.name, 0u);
    publishers_[pub_info.topic_info.name] = pub;
    return pub;
  }

  std::shared_ptr<Subscriber> create_subscriber(const sys_msgs::SubInfo& sub_info,
                                                const Endpoint& /*endpoint*/) override {
    if (should_fail_) {
      return nullptr;
    }
    auto sub = std::make_shared<MockSubscriber>(sub_info.topic_info.name);
    subscribers_[sub_info.topic_info.name] = sub;
    return sub;
  }

  std::shared_ptr<TimerCallback> create_timer(const Duration& d, const TimerCallback::Callback& cb) override {
    if (should_fail_) {
      return nullptr;
    }
    auto timer = std::make_shared<MockTimerCallback>(d, cb);
    return timer;
  }

  std::shared_ptr<Service> create_service(sys_msgs::SrvInfo& srv_info, const Endpoint& /*endpoint*/) override {
    if (should_fail_) {
      return nullptr;
    }
    auto srv = std::make_shared<MockService>(srv_info.name);
    services_[srv_info.name] = srv;
    return srv;
  }

  std::shared_ptr<ServiceClient> create_service_client(sys_msgs::SrvRequest& request,
                                                       const Endpoint& /*endpoint*/) override {
    if (should_fail_) {
      return nullptr;
    }
    auto cli = std::make_shared<MockServiceClient>(request.name);
    service_clients_[request.name] = cli;
    return cli;
  }

  std::shared_ptr<Action> create_action(sys_msgs::ActInfo& act_info, const Endpoint& /*endpoint*/) override {
    if (should_fail_) {
      return nullptr;
    }
    auto act = std::make_shared<MockAction>(act_info.name);
    actions_[act_info.name] = act;
    return act;
  }

  std::shared_ptr<ActionClient> create_action_client(sys_msgs::ActRequest& request,
                                                     const Endpoint& /*endpoint*/) override {
    if (should_fail_) {
      return nullptr;
    }
    auto cli = std::make_shared<MockActionClient>(request.name);
    action_clients_[request.name] = cli;
    return cli;
  }

  void preset_parameter(const std::string& name, const Message& parameter) {
    parameter.serialize(preset_parameters_[name]);
  }

  std::shared_ptr<MockPublisher> get_publisher(const std::string& topic) const {
    auto it = publishers_.find(topic);
    return it != publishers_.end() ? it->second : nullptr;
  }

  std::shared_ptr<MockSubscriber> get_subscriber(const std::string& topic) const {
    auto it = subscribers_.find(topic);
    return it != subscribers_.end() ? it->second : nullptr;
  }

  std::shared_ptr<MockService> get_service(const std::string& name) const {
    auto it = services_.find(name);
    return it != services_.end() ? it->second : nullptr;
  }

  std::shared_ptr<MockServiceClient> get_service_client(const std::string& name) const {
    auto it = service_clients_.find(name);
    return it != service_clients_.end() ? it->second : nullptr;
  }

  std::shared_ptr<MockAction> get_action(const std::string& name) const {
    auto it = actions_.find(name);
    return it != actions_.end() ? it->second : nullptr;
  }

  std::shared_ptr<MockActionClient> get_action_client(const std::string& name) const {
    auto it = action_clients_.find(name);
    return it != action_clients_.end() ? it->second : nullptr;
  }

private:
  std::map<std::string, std::shared_ptr<MockPublisher>> publishers_;
  std::map<std::string, std::shared_ptr<MockSubscriber>> subscribers_;
  std::map<std::string, std::shared_ptr<MockService>> services_;
  std::map<std::string, std::shared_ptr<MockServiceClient>> service_clients_;
  std::map<std::string, std::shared_ptr<MockAction>> actions_;
  std::map<std::string, std::shared_ptr<MockActionClient>> action_clients_;
  std::map<std::string, std::vector<uint8_t>> preset_parameters_;
  static inline bool should_fail_{false};
};

} // namespace test
} // namespace rix