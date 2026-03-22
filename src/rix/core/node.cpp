#include "rix/core/node.hpp"
#include "rix/std_msgs/UInt64.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include "rix/sys_msgs/Status.hpp"

namespace rix {

Node::Node(const std::string& name, const Endpoint& endpoint) : rixhub_endpoint_(Endpoint(RIXHUB_IP, RIXHUB_PORT)) {
  if (factory_ == nullptr) {
    shutdown();
    return;
  }

  info_.id = id_factory_();
  info_.name = name;
  info_.protocol = Protocol::TCP;

  mediator_client_ = factory_->create_mediator_client(info_, endpoint, rixhub_endpoint_);
  if (!mediator_client_) {
    shutdown();
    return;
  }
  if (!mediator_client_->ok()) {
    shutdown();
    return;
  }
}

Node::~Node() {
  while (!components_.empty()) {
    components_.pop_back(); // Preserve order of destruction
  }
}

void Node::on_spin() {
  // Spin all components, remove ones that are not 'ok'
  auto it = components_.begin();
  while (it != components_.end()) {
    const auto component = *it;
    if (!component->ok()) {
      it = components_.erase(it);
      continue;
    }
    if (!MULTITHREADED) {
      component->spin_once();
    }
    ++it;
  }

  if (!MULTITHREADED) {
    mediator_client_->spin_once();
  } else {
    // Sleep to prevent busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
}

std::shared_ptr<TimerCallback> Node::create_timer(const Duration& d, const TimerCallback::Callback& callback) {
  if (!ok()) {
    Log::error << "Node is shutdown, cannot create timer." << std::endl;
    return nullptr;
  }

  auto timer = factory_->create_timer(d, callback);
  if (timer) {
    components_.push_back(timer);
  }
  return timer;
}

bool Node::get_system_info(sys_msgs::SystemInfo& info) {
  if (!ok()) {
    Log::error << "Node is shutdown, cannot get system info." << std::endl;
    return false;
  }
  return mediator_client_->get_system_info(info);
}

bool Node::set_parameter(const std::string& name, const Message& parameter) const {
  if (!ok()) {
    Log::error << "Node is shutdown, cannot set parameter." << std::endl;
    return false;
  }
  return mediator_client_->set_parameter(name, parameter);
}

bool Node::get_parameter(const std::string& name, Message& parameter) const {
  if (!ok()) {
    Log::error << "Node is shutdown, cannot get parameter." << std::endl;
    return false;
  }
  return mediator_client_->get_parameter(name, parameter);
}

} // namespace rix