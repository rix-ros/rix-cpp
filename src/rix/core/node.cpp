#include "rix/core/node.hpp"

namespace rix::core {

Node::~Node() {
  shutdown();
  send_message_with_opcode_no_response(
      client_factory_(), info_, OPCODE::NODE_DEREGISTER, rixhub_endpoint_);
}

bool Node::ok() const { return !shutdown_flag_; }

void Node::shutdown() { shutdown_flag_ = true; }

void Node::spin_once() {
  // Spin all components, remove ones that are not 'ok'
  auto it = components_.begin();
  while (it != components_.end()) {
    auto component = *it;
    if (!component->ok()) {
      it = components_.erase(it);
      continue;
    }
    component->spin_once();
    it++;
  }
}

std::shared_ptr<Timer> Node::create_timer(const rix::util::Duration &d,
                                          Timer::Callback callback) {
  auto timer = std::make_shared<rix::core::Timer>(d, callback);
  components_.push_back(timer);
  return timer;
}

uint64_t Node::generate_id() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<uint64_t> dis;
  return dis(gen);
}

std::shared_ptr<Publisher>
Node::create_publisher(const rix::msg::mediator::TopicInfo &topic_info,
                       const rix::ipc::Endpoint &endpoint) {
  rix::msg::mediator::PubInfo pub_info;
  pub_info.id = generate_id();
  pub_info.node_id = info_.id;
  pub_info.topic_info = topic_info;
  auto server = server_factory_(endpoint);
  auto server_endpoint = server->local_endpoint();
  pub_info.endpoint.address = server_endpoint.address;
  pub_info.endpoint.port = server_endpoint.port;
  auto pub = std::shared_ptr<Publisher>(
      new Publisher(pub_info, server, client_factory_, rixhub_endpoint_));
  components_.push_back(pub);
  return pub;
}

std::shared_ptr<Subscriber>
Node::create_subscriber(const rix::msg::mediator::TopicInfo &topic_info,
                        const rix::ipc::Endpoint &endpoint) {
  rix::msg::mediator::SubInfo sub_info;
  sub_info.id = generate_id();
  sub_info.node_id = info_.id;
  sub_info.topic_info = topic_info;
  auto server = server_factory_(endpoint);
  auto server_endpoint = server->local_endpoint();
  sub_info.endpoint.address = server_endpoint.address;
  sub_info.endpoint.port = server_endpoint.port;
  auto sub = std::shared_ptr<Subscriber>(
      new Subscriber(sub_info, server, client_factory_, rixhub_endpoint_));
  components_.push_back(sub);
  return sub;
}

Node::Node(const std::string &name, const rix::ipc::Endpoint &rixhub_endpoint,
           ServerFactory server_factory, ClientFactory client_factory)
    : rixhub_endpoint_(rixhub_endpoint), server_factory_(server_factory),
      client_factory_(client_factory), shutdown_flag_(false) {
  info_.id = generate_id();
  info_.name = name;
  if (!send_message_with_opcode(client_factory_(), info_, OPCODE::NODE_REGISTER,
                                rixhub_endpoint_)) {
    shutdown();
  }
}

std::shared_ptr<Service>
Node::create_service(rix::msg::mediator::SrvInfo &service_info,
                     const rix::ipc::Endpoint &endpoint) {
  service_info.id = generate_id();
  service_info.node_id = info_.id;
  auto server = server_factory_(endpoint);
  auto server_endpoint = server->local_endpoint();
  service_info.endpoint.address = server_endpoint.address;
  service_info.endpoint.port = server_endpoint.port;
  auto srv = std::shared_ptr<Service>(
      new Service(service_info, server, client_factory_, rixhub_endpoint_));
  components_.push_back(srv);
  return srv;
}

bool Node::get_system_info(rix::msg::mediator::SystemInfo &info) {
  return send_opcode_with_response(
      client_factory_(), info, OPCODE::SYSTEM_GET_REQUEST, rixhub_endpoint_);
}

} // namespace rix::core