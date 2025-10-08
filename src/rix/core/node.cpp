#include "rix/core/node.hpp"

namespace rix {

Node::Node(const std::string& name,
           const Endpoint&    rixhub_endpoint,
           SocketFactory      socket_factory)
    : rixhub_endpoint_(rixhub_endpoint), socket_factory_(socket_factory),
      registered_flag_(false) {
  info_.id = generate_id();
  info_.name = name;

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::NODE_REGISTER, info_)) {
    shutdown();
    return;
  }

  msg::mediator::Operation op;
  msg::mediator::Status    status;
  if (!client->recv_message(op, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;
}

Node::~Node() {
  if (registered_flag_) {
    auto client = socket_factory_();
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::NODE_DEREGISTER, info_);
    }
  }
}

void Node::spin_once() {
  // Spin all components, remove ones that are not 'ok'
  auto it = components_.begin();
  while (it != components_.end()) {
    auto component = *it;
    if (!component->ok()) {
      it = components_.erase(it);
      continue;
    }
#ifndef RIX_MULTITHREADED
    component->spin_once();
#endif
    it++;
  }

#ifdef RIX_MULTITHREADED
  // Sleep to prevent busy waiting
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
}

std::shared_ptr<Publisher>
Node::create_publisher(const msg::mediator::TopicInfo& topic_info,
                       const Endpoint&                 rixhub_endpoint,
                       const Endpoint&                 endpoint) {
  msg::mediator::PubInfo pub_info;
  pub_info.id = generate_id();
  pub_info.node_id = info_.id;
  pub_info.topic_info = topic_info;
  pub_info.endpoint.address = endpoint.address;
  pub_info.endpoint.port = endpoint.port;
  auto pub = std::shared_ptr<Publisher>(
      new Publisher(pub_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(pub);
  return pub;
}

std::shared_ptr<Subscriber>
Node::create_subscriber(const msg::mediator::TopicInfo& topic_info,
                        const Endpoint&                 rixhub_endpoint,
                        const Endpoint&                 endpoint) {
  msg::mediator::SubInfo sub_info;
  sub_info.id = generate_id();
  sub_info.node_id = info_.id;
  sub_info.topic_info = topic_info;
  sub_info.endpoint.address = endpoint.address;
  sub_info.endpoint.port = endpoint.port;
  auto sub = std::shared_ptr<Subscriber>(
      new Subscriber(sub_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(sub);
  return sub;
}

std::shared_ptr<Service> Node::create_service(msg::mediator::SrvInfo& service_info,
                                              const Endpoint&         rixhub_endpoint,
                                              const Endpoint&         endpoint) {
  service_info.id = generate_id();
  service_info.node_id = info_.id;
  service_info.endpoint.address = endpoint.address;
  service_info.endpoint.port = endpoint.port;
  auto srv = std::shared_ptr<Service>(
      new Service(service_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(srv);
  return srv;
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

  msg::mediator::Operation op;
  if (!client->recv_message(op, info)) {
    return false;
  }
  if (op.opcode != OPCODE::SYSTEM_GET_RESPONSE) {
    return false;
  }
  Log::debug << "Retrieved system info from RIXHub." << std::endl;
  return true;
}

} // namespace rix