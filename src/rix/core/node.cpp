#include "rix/core/node.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt64.hpp"

namespace rix {

Node::Node(const std::string& name, const Endpoint& endpoint)
    : rixhub_endpoint_(Endpoint(RIXHUB_IP, RIXHUB_PORT)), registered_flag_(false) {
  server_ = socket_factory_();
  if (!server_) {
    shutdown();
    return;
  }

  if (!server_->set_reuse_address(true)) {
    return;
  }
  if (!server_->bind(Endpoint(endpoint.address, endpoint.port))) {
    return;
  }
  if (!server_->listen(MAX_CONN)) {
    return;
  }

  // Ensure server was initialized properly
  if (server_->is_exception()) {
    shutdown();
    return;
  }

  const auto server_endpoint = server_->local_endpoint();
  // Update the endpoint in case the port was set to 0 (ephemeral)
  info_.endpoint.address = server_endpoint.address;
  info_.endpoint.port = server_endpoint.port;

  info_.id = id_factory_();
  info_.name = name;

  const auto client = socket_factory_();
  if (!client) {
    shutdown();
    return;
  }
  if (!client->connect(rixhub_endpoint_)) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::NODE_REGISTER, info_)) {
    shutdown();
    return;
  }

  msg::mediator::Operation operation;
  msg::mediator::Status status;
  if (!client->recv_message(operation, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;

  // Create timer to handle pings at 2Hz
  create_timer(Duration(0.5), [this](const TimerCallback::Event&) {
    // Check for ping
    // std::cout << "Checking for ping..." << std::endl;
    if (server_->is_readable()) {
      const auto conn = server_->accept();
      if (conn) {
        msg::mediator::Operation operation;
        conn->recv_message(operation, operation.size());
        if (operation.opcode == OPCODE::PING) {
          msg::mediator::Status status;
          status.id = info_.id;
          status.error = 0;
          conn->send_message(OPCODE::STATUS_RESPONSE, status);
        }
      }
    }
  });
}

Node::~Node() {
  if (registered_flag_) {
    const auto client = socket_factory_();
    if (!client) {
      return;
    }
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::NODE_DEREGISTER, info_);
    }
  }
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
#ifndef RIX_MULTITHREADED
    component->spin_once();
#endif
    ++it;
  }

#ifdef RIX_MULTITHREADED
  // Sleep to prevent busy waiting
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
}

std::shared_ptr<Publisher> Node::create_publisher(const msg::mediator::TopicInfo& topic_info,
                                                  const Endpoint& rixhub_endpoint,
                                                  const Endpoint& endpoint) {
  msg::mediator::PubInfo pub_info;
  pub_info.id = id_factory_();
  pub_info.node_id = info_.id;
  pub_info.topic_info = topic_info;
  pub_info.endpoint.address = endpoint.address;
  pub_info.endpoint.port = endpoint.port;
  auto pub = std::shared_ptr<Publisher>(new Publisher(pub_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(pub);
  return pub;
}

std::shared_ptr<Subscriber> Node::create_subscriber(const msg::mediator::TopicInfo& topic_info,
                                                    const Endpoint& rixhub_endpoint,
                                                    const Endpoint& endpoint) {
  msg::mediator::SubInfo sub_info;
  sub_info.id = id_factory_();
  sub_info.node_id = info_.id;
  sub_info.topic_info = topic_info;
  sub_info.endpoint.address = endpoint.address;
  sub_info.endpoint.port = endpoint.port;
  auto sub = std::shared_ptr<Subscriber>(new Subscriber(sub_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(sub);
  return sub;
}

std::shared_ptr<Service>
Node::create_service(msg::mediator::SrvInfo& service_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint) {
  service_info.id = id_factory_();
  service_info.node_id = info_.id;
  service_info.endpoint.address = endpoint.address;
  service_info.endpoint.port = endpoint.port;
  auto srv = std::shared_ptr<Service>(new Service(service_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(srv);
  return srv;
}

std::shared_ptr<Action>
Node::create_action(msg::mediator::ActInfo& action_info, const Endpoint& rixhub_endpoint, const Endpoint& endpoint) {
  action_info.id = id_factory_();
  action_info.node_id = info_.id;
  action_info.endpoint.address = endpoint.address;
  action_info.endpoint.port = endpoint.port;
  auto act = std::shared_ptr<Action>(new Action(action_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(act);
  return act;
}

bool Node::get_system_info(msg::mediator::SystemInfo& info) {
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_))
    return false;

  msg::standard::UInt64 node_id;
  node_id.data = info_.id;
  if (!client->send_message(OPCODE::SYSTEM_GET_REQUEST, node_id)) {
    return false;
  }

  msg::mediator::Operation operation;
  if (!client->recv_message(operation, info)) {
    return false;
  }
  if (operation.opcode != OPCODE::SYSTEM_GET_RESPONSE) {
    return false;
  }
  Log::debug << "Retrieved system info from RIXHub." << std::endl;
  return true;
}

std::shared_ptr<ServiceClient> Node::create_service_client(const msg::mediator::SrvRequest& service_request,
                                                           const Endpoint& rixhub_endpoint) {
  auto srv_cli = std::shared_ptr<ServiceClient>(new ServiceClient(service_request, socket_factory_, rixhub_endpoint));
  components_.push_back(srv_cli);
  return srv_cli;
}

std::shared_ptr<ActionClient> Node::create_action_client(const msg::mediator::ActRequest& action_request,
                                                         const Endpoint& rixhub_endpoint) {
  auto act_cli = std::shared_ptr<ActionClient>(new ActionClient(action_request, socket_factory_, rixhub_endpoint));
  components_.push_back(act_cli);
  return act_cli;
}

} // namespace rix