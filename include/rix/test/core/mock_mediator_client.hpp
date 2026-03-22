#pragma once

#include "rix/core/mediator_client.hpp"
#include "rix/sys_msgs/ActInfo.hpp"
#include "rix/sys_msgs/ActRequest.hpp"
#include "rix/sys_msgs/SrvInfo.hpp"
#include "rix/sys_msgs/SrvRequest.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/sys_msgs/TopicInfo.hpp"
#include "rix/test/core/mock_action.hpp"
#include "rix/test/core/mock_action_client.hpp"
#include "rix/test/core/mock_publisher.hpp"
#include "rix/test/core/mock_service.hpp"
#include "rix/test/core/mock_service_client.hpp"
#include "rix/test/core/mock_subscriber.hpp"
#include "rix/test/core/mock_timer_callback.hpp"

namespace rix {
namespace test {

class MockMediatorClientImpl final : public MediatorClient {
public:
  static void set_should_fail() { should_fail_ = true; }
  static void reset_fail() { should_fail_ = false; }

  MockMediatorClientImpl(sys_msgs::NodeInfo& /*node_info*/,
                         const Endpoint& /*endpoint*/,
                         const Endpoint& /*rixhub_endpoint*/) {
    if (should_fail_) {
      should_fail_ = false; // consume the flag so subsequent nodes succeed
      shutdown();
    }
  }

  ~MockMediatorClientImpl() override {}

  bool set_parameter(const std::string& name, const Message& parameter) const override {
    parameter.serialize(parameters_[name]);
    return true;
  }

  bool get_parameter(const std::string& name, Message& parameter) const override {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) {
      parameter.deserialize(it->second);
      return true;
    }
    return false;
  }

  bool get_system_info(sys_msgs::SystemInfo& info) override {
    info = system_info_;
    return true;
  }

  void set_system_info(const sys_msgs::SystemInfo& info) { system_info_ = info; }

private:
  static inline bool should_fail_{false};
  mutable std::map<std::string, std::vector<uint8_t>> parameters_;
  sys_msgs::SystemInfo system_info_;

  /**
   * @brief Internal spin implementation for the MockMediatorClientImpl.
   */
  void on_spin() override {}
};

} // namespace test
} // namespace rix