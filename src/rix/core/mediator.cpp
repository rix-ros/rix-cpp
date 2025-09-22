#include "rix/core/mediator.hpp"

namespace rix::core {

Mediator::Mediator(const rix::ipc::Endpoint &rixhub_endpoint, SocketFactory socket_factory)
    : socket_factory_(socket_factory), shutdown_flag_(true) {
  server_ = socket_factory_();
  server_->set_reuse_address(true);
  server_->bind(rixhub_endpoint);
  server_->listen(rix::ipc::MAX_CONN);
  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    return;
  }

  shutdown_flag_ = false;
  rix::util::Log::info << "rixhub started on " << server_->local_endpoint() << std::endl;
}

Mediator::~Mediator() {}

bool Mediator::ok() const { return !shutdown_flag_; }

void Mediator::shutdown() { shutdown_flag_ = true; }

void Mediator::spin_once() {
  if (!server_->wait_readable(rix::util::Duration(0.0))) {
    return;
  }

  auto conn = server_->accept();
  if (!conn) {
    return;
  }

  rix::msg::mediator::Operation operation;
  if (!conn->recv_message(operation, operation.size())) {
    return;
  }

  switch (operation.opcode) {
  case OPCODE::NODE_REGISTER: {
    handle_node_register(operation, conn);
    break;
  }
  case OPCODE::PUB_REGISTER: {
    handle_pub_register(operation, conn);
    break;
  }
  case OPCODE::SUB_REGISTER: {
    handle_sub_register(operation, conn);
    break;
  }
  case OPCODE::SRV_REGISTER: {
    handle_srv_register(operation, conn);
    break;
  }
  case OPCODE::NODE_DEREGISTER: {
    handle_node_deregister(operation, conn);
    break;
  }
  case OPCODE::PUB_DEREGISTER: {
    handle_pub_deregister(operation, conn);
    break;
  }
  case OPCODE::SUB_DEREGISTER: {
    handle_sub_deregister(operation, conn);
    break;
  }
  case OPCODE::SRV_DEREGISTER: {
    handle_srv_deregister(operation, conn);
    break;
  }
  case OPCODE::SRV_REQUEST: {
    handle_srv_request(operation, conn);
    break;
  }
  case OPCODE::PARAM_SET_REQUEST: {
    handle_param_set_request(operation, conn);
    break;
  }
  case OPCODE::PARAM_GET_REQUEST: {
    handle_param_get_request(operation, conn);
    break;
  }
  case OPCODE::SYSTEM_GET_REQUEST: {
    handle_system_get_request(operation, conn);
    break;
  }
  default: {
    rix::util::Log::warn << "Received invalid opcode: " << operation.opcode << std::endl;
    return;
  }
  }
}

void Mediator::handle_node_register(const rix::msg::mediator::Operation &operation,
                                    std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::Status status;
  status.error = 0;
  rix::msg::mediator::NodeInfo info;
  if (!conn->recv_message(info, operation.len)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }
  status.id = info.id;

  // Ensure that the node ID is not already registered
  if (nodes_.find(info.id) != nodes_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  if (status.error == 0) {
    nodes_.insert({info.id, info});
    rix::util::Log::info << "Registered node \"" << info.name << "\"." << std::endl;
  }
  conn->send_message(OPCODE::STATUS_RESPONSE, status);
}

void Mediator::handle_pub_register(const rix::msg::mediator::Operation &operation,
                                   std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::Status status;
  status.error = 0;
  rix::msg::mediator::PubInfo info;
  if (!conn->recv_message(info, operation.len)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }
  status.id = info.id;

  // Ensure that the publisher has a existing node ID
  if (nodes_.find(info.node_id) == nodes_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  // Ensure that the publisher ID is not already registered
  if (publishers_.find(info.id) != publishers_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  // Ensure that the topic hash matches the record, or is new
  if (!validate_topic_info(info.topic_info)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  publishers_.insert({info.id, info});
  rix::util::Log::info << "Registered publisher on \"" << info.topic_info.name << "\"." << std::endl;

  // Notify all subscribers of the new publisher on the
  // same topic
  std::vector<rix::msg::mediator::SubInfo> subs_to_notify;
  for (const auto &[id, sub_info] : subscribers_) {
    if (sub_info.topic_info.name == info.topic_info.name) {
      subs_to_notify.push_back(sub_info);
    }
  }

  conn->send_message(OPCODE::STATUS_RESPONSE, status);
  notify_subscribers(subs_to_notify, info);
}

void Mediator::handle_sub_register(const rix::msg::mediator::Operation &operation,
                                   std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::Status status;
  status.error = 0;
  rix::msg::mediator::SubInfo info;
  if (!conn->recv_message(info, operation.len)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }
  status.id = info.id;

  // Ensure that the subscriber has a existing node ID
  if (nodes_.find(info.node_id) == nodes_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  // Ensure that the subscriber ID is not already registered
  if (subscribers_.find(info.id) != subscribers_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  // Ensure that the topic hash matches the record, or is new
  if (!validate_topic_info(info.topic_info)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  subscribers_.insert({info.id, info});
  rix::util::Log::info << "Registered subscriber on \"" << info.topic_info.name << "\"." << std::endl;

  // Notify the new subscriber of all publishers on the
  // same topic
  std::vector<rix::msg::mediator::PubInfo> pubs_on_topic;
  for (const auto &[id, pub_info] : publishers_) {
    if (pub_info.topic_info.name == info.topic_info.name) {
      pubs_on_topic.push_back(pub_info);
    }
  }
  conn->send_message(OPCODE::STATUS_RESPONSE, status);
  notify_subscribers(info, pubs_on_topic);
}

void Mediator::handle_srv_register(const rix::msg::mediator::Operation &operation,
                                   std::shared_ptr<rix::ipc::GenericSocket> conn) {

  rix::msg::mediator::Status status;
  status.error = 0;
  rix::msg::mediator::SrvInfo info;
  if (!conn->recv_message(info, operation.len)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }
  status.id = info.id;

  // Ensure that the service has a existing node ID
  if (nodes_.find(info.node_id) == nodes_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  // Ensure that the service ID is not already registered
  if (services_.find(info.id) != services_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  // Ensure that the service hash matches the record, or is new
  if (!validate_service_info(info)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  services_.insert({info.id, info});
  rix::util::Log::info << "Registered service \"" << info.name << "\"." << std::endl;
  conn->send_message(OPCODE::STATUS_RESPONSE, status);
}

void Mediator::handle_node_deregister(const rix::msg::mediator::Operation &operation,
                                      std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::NodeInfo info;
  if (!conn->recv_message(info, operation.len)) {
    return;
  }
  if (nodes_.find(info.id) == nodes_.end()) {
    return;
  }
  nodes_.erase(info.id);
  rix::util::Log::info << "Deregistered node \"" << info.name << "\"." << std::endl;
}

void Mediator::handle_pub_deregister(const rix::msg::mediator::Operation &operation,
                                     std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::PubInfo info;
  if (!conn->recv_message(info, operation.len)) {
    return;
  }
  if (publishers_.find(info.id) == publishers_.end()) {
    return;
  }
  publishers_.erase(info.id);
  rix::util::Log::info << "Deregistered publisher on \"" << info.topic_info.name << "\"." << std::endl;
}

void Mediator::handle_sub_deregister(const rix::msg::mediator::Operation &operation,
                                     std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::SubInfo info;
  if (!conn->recv_message(info, operation.len)) {
    return;
  }
  if (subscribers_.find(info.id) == subscribers_.end()) {
    return;
  }
  subscribers_.erase(info.id);
  rix::util::Log::info << "Deregistered subscriber on \"" << info.topic_info.name << "\"." << std::endl;
}

void Mediator::handle_srv_deregister(const rix::msg::mediator::Operation &operation,
                                     std::shared_ptr<rix::ipc::GenericSocket> conn) {
  rix::msg::mediator::SrvInfo info;
  if (!conn->recv_message(info, operation.len)) {
    return;
  }
  if (services_.find(info.id) == services_.end()) {
    return;
  }
  services_.erase(info.id);
  rix::util::Log::info << "Deregistered service \"" << info.name << "\"." << std::endl;
}

void Mediator::handle_srv_request(const rix::msg::mediator::Operation &operation,
                                  std::shared_ptr<rix::ipc::GenericSocket> conn) {

  rix::msg::mediator::SrvResponse response;
  response.error = 0;

  rix::msg::mediator::SrvRequest request;
  if (!conn->recv_message(request, operation.len)) {
    response.error = -1;
    conn->send_message(OPCODE::SRV_RESPONSE, response);
    return;
  }

  // Ensure that the requester has a existing node ID
  if (nodes_.find(request.node_id) == nodes_.end()) {
    response.error = -1;
    conn->send_message(OPCODE::SRV_RESPONSE, response);
    return;
  }

  auto it = std::find_if(services_.begin(), services_.end(), [&](const auto &srv) {
    return srv.second.name == request.name && srv.second.request_hash == request.request_hash &&
           srv.second.response_hash == request.response_hash;
  });
  if (it == services_.end()) {
    response.error = -1;
    conn->send_message(OPCODE::SRV_RESPONSE, response);
    return;
  }

  response.srv_info = it->second;
  conn->send_message(OPCODE::SRV_RESPONSE, response);
}

void Mediator::handle_param_set_request(const rix::msg::mediator::Operation &operation,
                                        std::shared_ptr<rix::ipc::GenericSocket> conn) {

  rix::msg::mediator::Status status;
  status.error = 0;

  rix::msg::mediator::ParamInfo info;

  if (!conn->recv_message(info, operation.len)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  status.id = info.id;

  // Ensure that the requester has a existing node ID
  if (nodes_.find(info.id) == nodes_.end()) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  if (!set_parameter(info)) {
    status.error = -1;
    conn->send_message(OPCODE::STATUS_RESPONSE, status);
    return;
  }

  conn->send_message(OPCODE::STATUS_RESPONSE, status);
}

void Mediator::handle_param_get_request(const rix::msg::mediator::Operation &operation,
                                        std::shared_ptr<rix::ipc::GenericSocket> conn) {

  rix::msg::mediator::ParamInfo info;

  if (!conn->recv_message(info, operation.len)) {
    conn->send_message(OPCODE::PARAM_GET_RESPONSE, info);
    return;
  }

  // Ensure that the requester has a existing node ID
  if (nodes_.find(info.id) == nodes_.end()) {
    conn->send_message(OPCODE::PARAM_GET_RESPONSE, info);
    return;
  }

  if (!get_parameter(info)) {
    conn->send_message(OPCODE::PARAM_GET_RESPONSE, info);
    return;
  }

  conn->send_message(OPCODE::PARAM_GET_RESPONSE, info);
}

void Mediator::handle_system_get_request(const rix::msg::mediator::Operation &operation,
                                         std::shared_ptr<rix::ipc::GenericSocket> conn) {

  rix::msg::mediator::SystemInfo info;
  rix::msg::standard::UInt64 node_id;
  if (!conn->recv_message(node_id, operation.len)) {
    return;
  }

  // Ensure that the requester has a existing node ID
  if (nodes_.find(node_id.data) == nodes_.end()) {
    return;
  }

  for (const auto &node : nodes_) {
    info.nodes.push_back(node.second);
  }
  for (const auto &publisher : publishers_) {
    info.publishers.push_back(publisher.second);
  }
  for (const auto &subscriber : subscribers_) {
    info.subscribers.push_back(subscriber.second);
  }
  for (const auto &service : services_) {
    info.services.push_back(service.second);
  }

  conn->send_message(OPCODE::SYSTEM_GET_RESPONSE, info);
}

void Mediator::notify_subscribers(const std::vector<rix::msg::mediator::SubInfo> &subscribers,
                                  const rix::msg::mediator::PubInfo &publisher) {
  if (subscribers.empty()) {
    return;
  }
  rix::msg::mediator::SubNotify notify;
  notify.publishers.push_back(publisher);
  for (const auto &sub : subscribers) {
    rix::ipc::Endpoint endpoint(sub.endpoint.address, sub.endpoint.port);
    notify.id = sub.id;
    auto client = socket_factory_();
    client->connect(endpoint);
    client->send_message(OPCODE::SUB_NOTIFY, notify);
  }
}

void Mediator::notify_subscribers(const rix::msg::mediator::SubInfo &subscriber,
                                  const std::vector<rix::msg::mediator::PubInfo> &publishers) {
  if (publishers.empty()) {
    return;
  }
  rix::msg::mediator::SubNotify notify;
  notify.publishers = publishers;
  rix::ipc::Endpoint endpoint(subscriber.endpoint.address, subscriber.endpoint.port);
  notify.id = subscriber.id;
  auto client = socket_factory_();
  client->connect(endpoint);
  client->send_message(OPCODE::SUB_NOTIFY, notify);
}

bool Mediator::validate_topic_info(const rix::msg::mediator::TopicInfo &info) {
  const auto &topic_hash = info.message_hash;
  const auto &topic_name = info.name;
  auto it = topic_hashes_.find(topic_name);
  if (it == topic_hashes_.end()) {
    topic_hashes_.insert({topic_name, topic_hash});
  } else if (topic_hash != it->second) {
    // Invalid message type for existing topic
    return false;
  }
  return true;
}

bool Mediator::validate_service_info(const rix::msg::mediator::SrvInfo &info) {
  // return true if the service name does not exist
  const auto &service_name = info.name;
  auto it = std::find_if(services_.begin(), services_.end(),
                         [&](const auto &srv) { return srv.second.name == service_name; });
  return it == services_.end();
}

bool Mediator::set_parameter(const rix::msg::mediator::ParamInfo &info) {
  const auto &param_hash = info.message_hash;
  const auto &param_name = info.name;
  auto it = parameters_.find(param_name);

  // If parameter does not exist, insert it
  if (it == parameters_.end()) {
    parameters_.insert({param_name, {param_hash, info.data}});
    return true;
  }
  // If the parameter exists and the hashes match, update it
  if (it->second.first == param_hash) {
    it->second.second = info.data;
    return true;
  }
  // If the parameter exists and the hashes do not match, return false
  return false;
}

bool Mediator::get_parameter(rix::msg::mediator::ParamInfo &info) {
  const auto &param_hash = info.message_hash;
  const auto &param_name = info.name;
  auto it = parameters_.find(param_name);
  // If param does not exist, return false
  if (it == parameters_.end()) {
    return false;
  }
  // If hashes do not match, return false
  if (it->second.first != param_hash) {
    return false;
  }
  // Copy and return true
  info.data = it->second.second;
  return true;
}

} // namespace rix::core