#include "rix/core/node.hpp"

namespace rix::core {

Node::Node(const std::string &name, const rix::ipc::Endpoint &rixhub_endpoint, SocketFactory socket_factory)
    : rixhub_endpoint_(rixhub_endpoint), socket_factory_(socket_factory), shutdown_flag_(true),
      registered_flag_(false) {
  info_.id = generate_id();
  info_.name = name;

  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_))
    return;
  if (!client->send_message(OPCODE::NODE_REGISTER, info_))
    return;

  rix::msg::mediator::Operation op;
  rix::msg::mediator::Status status;
  if (!client->recv_message(op, status))
    return;
  if (status.error)
    return;

  shutdown_flag_ = false;
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

std::shared_ptr<Timer> Node::create_timer(const rix::util::Duration &d, Timer::Callback callback) {
  if (!ok()) {
    rix::util::Log::error << "Node is shutdown, cannot create timer." << std::endl;
    return nullptr;
  }
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

std::shared_ptr<Publisher> Node::create_publisher(const rix::msg::mediator::TopicInfo &topic_info,
                                                  const rix::ipc::Endpoint &rixhub_endpoint,
                                                  const rix::ipc::Endpoint &endpoint) {
  rix::msg::mediator::PubInfo pub_info;
  pub_info.id = generate_id();
  pub_info.node_id = info_.id;
  pub_info.topic_info = topic_info;
  pub_info.endpoint.address = endpoint.address;
  pub_info.endpoint.port = endpoint.port;
  auto pub = std::shared_ptr<Publisher>(new Publisher(pub_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(pub);
  return pub;
}

std::shared_ptr<Subscriber> Node::create_subscriber(const rix::msg::mediator::TopicInfo &topic_info,
                                                    const rix::ipc::Endpoint &rixhub_endpoint,
                                                    const rix::ipc::Endpoint &endpoint) {
  rix::msg::mediator::SubInfo sub_info;
  sub_info.id = generate_id();
  sub_info.node_id = info_.id;
  sub_info.topic_info = topic_info;
  sub_info.endpoint.address = endpoint.address;
  sub_info.endpoint.port = endpoint.port;
  auto sub = std::shared_ptr<Subscriber>(new Subscriber(sub_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(sub);
  return sub;
}

std::shared_ptr<Service> Node::create_service(rix::msg::mediator::SrvInfo &service_info,
                                              const rix::ipc::Endpoint &rixhub_endpoint,
                                              const rix::ipc::Endpoint &endpoint) {
  service_info.id = generate_id();
  service_info.node_id = info_.id;
  service_info.endpoint.address = endpoint.address;
  service_info.endpoint.port = endpoint.port;
  auto srv = std::shared_ptr<Service>(new Service(service_info, socket_factory_, rixhub_endpoint_));
  components_.push_back(srv);
  return srv;
}

bool Node::get_system_info(rix::msg::mediator::SystemInfo &info) {
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_))
    return false;

  rix::msg::standard::UInt64 node_id;
  node_id.data = info_.id;
  if (!client->send_message(OPCODE::SYSTEM_GET_REQUEST, node_id)) {
    return false;
  }

  rix::msg::mediator::Operation op;
  if (!client->recv_message(op, info)) {
    return false;
  }
  if (op.opcode != OPCODE::SYSTEM_GET_RESPONSE) {
    return false;
  }
  return true;
}

} // namespace rix::core