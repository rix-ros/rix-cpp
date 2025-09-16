#include "rix/core/mediator.hpp"

namespace rix::core {

Mediator::~Mediator() {}

bool Mediator::ok() const { return !shutdown_flag_; }

void Mediator::shutdown() { shutdown_flag_ = true; }

void Mediator::spin_once() {
  if (!server_->wait_acceptable(rix::util::Duration(0.001))) {
    return;
  }

  std::shared_ptr<rix::ipc::Connection> conn = server_->accept();
  if (!conn) {
    rix::util::Log::warn << "Failed to accept client." << std::endl;
    return;
  }

  rix::msg::mediator::Operation op;
  std::vector<uint8_t> buffer(op.size());
  ssize_t bytes_read = conn->read(buffer.data(), buffer.size());
  size_t offset = 0;

  // If the deserialize operation fails, return
  if (!op.deserialize(buffer.data(), bytes_read, offset)) {
    rix::util::Log::warn << "Failed to deserialize Operation message"
                         << std::endl;
    return;
  }

  if (op.len > 0) {
    buffer.resize(op.len);
    bytes_read = conn->read(buffer.data(), buffer.size());
    if (bytes_read <= 0) {
      rix::util::Log::warn << "Failed to read message" << std::endl;
      return;
    }
  }

  offset = 0;
  switch (op.opcode) {
  case OPCODE::NODE_REGISTER: {
    rix::msg::mediator::NodeInfo info;
    rix::msg::mediator::Status status;
    status.id = info.id;
    status.error = 0;

    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      status.error = -1;
      rix::util::Log::warn << "Failed to deserialize NodeInfo message."
                           << std::endl;
      send_status_message(conn, status);
      break;
    }
    nodes_.insert({info.id, info});
    rix::util::Log::info << "Registered node \"" << info.name << "\"."
                         << std::endl;
    send_status_message(conn, status);
    break;
  }
  case OPCODE::PUB_REGISTER: {
    rix::msg::mediator::PubInfo info;
    rix::msg::mediator::Status status;
    status.id = info.id;
    status.error = 0;

    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      status.error = -1;
      rix::util::Log::warn << "Failed to deserialize PubInfo message."
                           << std::endl;
      send_status_message(conn, status);
      break;
    }

    // Ensure that the topic hash matches the record, or is new
    if (!validate_topic_info(info.topic_info)) {
      status.error = -1;
      rix::util::Log::warn << "Topic message type mismatch!" << std::endl;
      send_status_message(conn, status);
      break;
    }

    publishers_.insert({info.id, info});
    rix::util::Log::info << "Registered publisher on \"" << info.topic_info.name
                         << "\"." << std::endl;

    // Notify all subscribers of the new publisher on the
    // same topic
    std::vector<rix::msg::mediator::SubInfo> subs_to_notify;
    for (const auto &[id, sub_info] : subscribers_) {
      if (sub_info.topic_info.name == info.topic_info.name) {
        subs_to_notify.push_back(sub_info);
      }
    }

    send_status_message(conn, status);
    notify_subscribers(subs_to_notify, info);
    break;
  }
  case OPCODE::SUB_REGISTER: {
    rix::msg::mediator::SubInfo info;
    rix::msg::mediator::Status status;
    status.id = info.id;
    status.error = 0;

    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      status.error = -1;
      rix::util::Log::warn << "Failed to deserialize SubInfo message."
                           << std::endl;
      send_status_message(conn, status);
      break;
    }

    // Ensure that the topic hash matches the record, or is new
    if (!validate_topic_info(info.topic_info)) {
      status.error = -1;
      rix::util::Log::warn << "Topic message type mismatch!" << std::endl;
      send_status_message(conn, status);
      break;
    }

    subscribers_.insert({info.id, info});
    rix::util::Log::info << "Registered subscriber on \""
                         << info.topic_info.name << "\"." << std::endl;

    // Notify the new subscriber of all publishers on the
    // same topic
    std::vector<rix::msg::mediator::PubInfo> pubs_on_topic;
    for (const auto &[id, pub_info] : publishers_) {
      if (pub_info.topic_info.name == info.topic_info.name) {
        pubs_on_topic.push_back(pub_info);
      }
    }
    send_status_message(conn, status);
    notify_subscribers(info, pubs_on_topic);
    break;
  }
  case OPCODE::SRV_REGISTER: {
    rix::msg::mediator::SrvInfo info;
    rix::msg::mediator::Status status;
    status.id = info.id;
    status.error = 0;

    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      status.error = -1;
      rix::util::Log::warn << "Failed to deserialize SrvInfo message."
                           << std::endl;
      send_status_message(conn, status);
      break;
    }

    // Ensure that the topic hash matches the record, or is new
    if (services_.find(info.id) != services_.end()) {
      status.error = -1;
      rix::util::Log::warn << "Service already exists!" << std::endl;
      send_status_message(conn, status);
      break;
    }

    services_.insert({info.id, info});
    rix::util::Log::info << "Registered service \"" << info.name << "\"."
                         << std::endl;
    send_status_message(conn, status);
    break;
  }
  case OPCODE::NODE_DEREGISTER: {
    rix::msg::mediator::NodeInfo info;
    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      rix::util::Log::warn << "Failed to deserialize NodeInfo message."
                           << std::endl;
      return;
    }
    nodes_.erase(info.id);
    rix::util::Log::info << "Deregistered node \"" << info.name << "\"."
                         << std::endl;
    break;
  }
  case OPCODE::PUB_DEREGISTER: {
    rix::msg::mediator::PubInfo info;
    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      rix::util::Log::warn << "Failed to deserialize PubInfo message."
                           << std::endl;
      return;
    }
    publishers_.erase(info.id);
    rix::util::Log::info << "Deregistered publisher on \""
                         << info.topic_info.name << "\"." << std::endl;
    break;
  }
  case OPCODE::SUB_DEREGISTER: {
    rix::msg::mediator::SubInfo info;
    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      rix::util::Log::warn << "Failed to deserialize SubInfo message."
                           << std::endl;
      return;
    }
    subscribers_.erase(info.id);
    rix::util::Log::info << "Deregistered subscriber on \""
                         << info.topic_info.name << "\"." << std::endl;
    break;
  }
  case OPCODE::SRV_DEREGISTER: {
    rix::msg::mediator::SrvInfo info;
    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      rix::util::Log::warn << "Failed to deserialize SubInfo message."
                           << std::endl;
      return;
    }
    services_.erase(info.id);
    rix::util::Log::info << "Deregistered service \"" << info.name << "\"."
                         << std::endl;
    break;
  }
  case OPCODE::SRV_REQUEST: {
    rix::msg::mediator::SrvRequest request;
    rix::msg::mediator::SrvResponse response;
    response.error = 0;

    if (!request.deserialize(buffer.data(), bytes_read, offset)) {
      response.error = -1;
      rix::util::Log::warn << "Failed to deserialize SrvInfo message."
                           << std::endl;
      send_response_message(conn, response);
      break;
    }

    // Ensure that the topic hash matches the record, or is new
    bool service_exists = false;
    rix::msg::mediator::SrvInfo info;
    for (const auto &srv : services_) {
      if (srv.second.name == request.name &&
          srv.second.request_hash == request.request_hash &&
          srv.second.response_hash == request.response_hash) {
        service_exists = true;
        info = srv.second;
        break;
      }
    }

    if (!service_exists) {
      response.error = -1;
      send_response_message(conn, response);
      break;
    }

    response.srv_info = info;
    send_response_message(conn, response);
    break;
  }
  case OPCODE::PARAM_SET_REQUEST: {
    rix::msg::mediator::ParamInfo info;
    rix::msg::mediator::Status status;
    status.id = 0;
    status.error = 0;

    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      status.error = -1;
      rix::util::Log::warn << "Failed to deserialize ParamInfo message."
                           << std::endl;
      send_status_message(conn, status);
      break;
    }

    if (!set_parameter(info)) {
      status.error = -1;
      rix::util::Log::warn << "Param message type mismatch!" << std::endl;
      send_status_message(conn, status);
      break;
    }

    send_status_message(conn, status);
    break;
  }
  case OPCODE::PARAM_GET_REQUEST: {
    rix::msg::mediator::ParamInfo info;

    if (!info.deserialize(buffer.data(), bytes_read, offset)) {
      rix::util::Log::warn << "Failed to deserialize ParamInfo message."
                           << std::endl;
      send_response_message(conn, info);
      break;
    }

    if (!get_parameter(info)) {
      rix::util::Log::warn << "Param message type mismatch!" << std::endl;
      send_response_message(conn, info);
      break;
    }

    send_response_message(conn, info);
    break;
  }
  case OPCODE::SYSTEM_GET_REQUEST: {
    rix::msg::mediator::SystemInfo info;

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

    send_response_message(conn, info);
    break;
  }
  default: {
    rix::util::Log::warn << "Received invalid opcode: " << op.opcode
                         << std::endl;
    return;
  }
  }
}

void Mediator::notify_subscribers(
    const std::vector<rix::msg::mediator::SubInfo> &subscribers,
    const rix::msg::mediator::PubInfo &publisher) {
  if (subscribers.empty())
    return;
  rix::msg::mediator::SubNotify notify;
  notify.publishers.push_back(publisher);
  for (const auto &sub : subscribers) {
    rix::ipc::Endpoint endpoint(sub.endpoint.address, sub.endpoint.port);
    notify.id = sub.id;
    send_message_with_opcode_no_response(client_factory_(), notify,
                                         OPCODE::SUB_NOTIFY, endpoint);
  }
}

void Mediator::notify_subscribers(
    const rix::msg::mediator::SubInfo &subscriber,
    const std::vector<rix::msg::mediator::PubInfo> &publishers) {
  if (publishers.empty())
    return;
  rix::msg::mediator::SubNotify notify;
  notify.publishers = publishers;
  rix::ipc::Endpoint endpoint(subscriber.endpoint.address,
                              subscriber.endpoint.port);
  notify.id = subscriber.id;
  send_message_with_opcode_no_response(client_factory_(), notify,
                                       OPCODE::SUB_NOTIFY, endpoint);
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

void Mediator::send_status_message(std::shared_ptr<rix::ipc::Connection> conn,
                                   rix::msg::mediator::Status status) {
  std::vector<uint8_t> buffer(status.size());
  size_t offset = 0;
  status.serialize(buffer.data(), offset);

  ssize_t bytes = conn->write(buffer.data(), buffer.size());
  if (bytes != buffer.size()) {
    rix::util::Log::warn << "Failed to write to rixhub." << std::endl;
  }
}

void Mediator::send_response_message(std::shared_ptr<rix::ipc::Connection> conn,
                                     const rix::msg::Message &response) {
  rix::msg::standard::UInt32 size;
  size.data = response.size();
  std::vector<uint8_t> buffer(size.size() + size.data);
  size_t offset = 0;
  size.serialize(buffer.data(), offset);
  response.serialize(buffer.data(), offset);
  ssize_t bytes = conn->write(buffer.data(), buffer.size());
  if (bytes != buffer.size()) {
    rix::util::Log::warn << "Failed to write to rixhub." << std::endl;
  }
}

Mediator::Mediator(const rix::ipc::Endpoint &rixhub_endpoint,
                   ServerFactory server_factory, ClientFactory client_factory)
    : server_(server_factory(rixhub_endpoint)), client_factory_(client_factory),
      shutdown_flag_(false) {
  if (server_->is_exception()) {
    shutdown();
  }
  rix::util::Log::init("rixhub");

  rix::util::Log::info << "rixhub started on " << server_->local_endpoint()
                       << std::endl;
}

} // namespace rix::core