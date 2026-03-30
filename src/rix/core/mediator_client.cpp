#include "rix/core/mediator_client.hpp"

namespace rix {
namespace detail {

MediatorClientImpl::MediatorClientImpl(sys_msgs::NodeInfo& node_info,
                                       const Endpoint& endpoint,
                                       const Endpoint& rixhub_endpoint)
    : info_(node_info), rixhub_endpoint_(rixhub_endpoint) {
  const TransportFactory& transport_factory = get_transport_factory(Protocol::TCP);

  server_ = transport_factory.create_acceptor(endpoint);
  if (!server_) {
    shutdown();
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

  const auto client = transport_factory.create_stream(rixhub_endpoint_, true);
  if (!client) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::NODE_REGISTER, info_)) {
    shutdown();
    return;
  }

  sys_msgs::Operation operation;
  sys_msgs::Status status;
  if (!client->recv_message(operation, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;

  if (MULTITHREADED) {
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

MediatorClientImpl::~MediatorClientImpl() {
  const TransportFactory& transport_factory = get_transport_factory(Protocol::TCP);
  if (registered_flag_) {
    const auto client = transport_factory.create_stream(rixhub_endpoint_, true);
    if (!client) {
      return;
    }
    client->send_message(OPCODE::NODE_DEREGISTER, info_);
  }
  if (MULTITHREADED && spin_thread_.joinable()) {
    shutdown();
    spin_thread_.join();
  }
}

bool MediatorClientImpl::set_parameter(const std::string& name, const Message& parameter) const {
  const TransportFactory& transport_factory = get_transport_factory(Protocol::TCP);
  sys_msgs::ParamInfo info;
  info.id = info_.id;
  info.name = name;
  info.message_hash = parameter.hash();
  info.data.resize(parameter.size());
  size_t offset = 0;
  parameter.serialize(info.data.data(), offset);

  auto client = transport_factory.create_stream(rixhub_endpoint_, true);
  if (!client->send_message(OPCODE::PARAM_SET_REQUEST, info)) {
    return false;
  }

  sys_msgs::Operation operation;
  sys_msgs::Status status;
  if (!client->recv_message(operation, status)) {
    return false;
  }

  if (operation.opcode != OPCODE::STATUS_RESPONSE) {
    return false;
  }

  return status.error == 0;
}

bool MediatorClientImpl::get_parameter(const std::string& name, Message& parameter) const {
  const TransportFactory& transport_factory = get_transport_factory(Protocol::TCP);
  sys_msgs::ParamInfo info;
  info.id = info_.id;
  info.name = name;
  info.message_hash = parameter.hash();
  sys_msgs::ParamInfo info_received;

  auto client = transport_factory.create_stream(rixhub_endpoint_, true);
  if (!client->send_message(OPCODE::PARAM_GET_REQUEST, info)) {
    return false;
  }
  sys_msgs::Operation operation;
  if (!client->recv_message(operation, info_received)) {
    return false;
  }
  if (operation.opcode != OPCODE::PARAM_GET_RESPONSE) {
    return false;
  }
  size_t offset = 0;
  return parameter.deserialize(info_received.data.data(), info_received.data.size(), offset);
}

bool MediatorClientImpl::get_system_info(sys_msgs::SystemInfo& info) {
  const TransportFactory& transport_factory = get_transport_factory(Protocol::TCP);
  auto client = transport_factory.create_stream(rixhub_endpoint_, true);

  std_msgs::UInt64 node_id;
  node_id.data = info_.id;
  if (!client->send_message(OPCODE::SYSTEM_GET_REQUEST, node_id)) {
    return false;
  }

  sys_msgs::Operation operation;
  if (!client->recv_message(operation, info)) {
    return false;
  }
  if (operation.opcode != OPCODE::SYSTEM_GET_RESPONSE) {
    return false;
  }
  Log::debug << "Retrieved system info from RIXHub.";
  return true;
}

void MediatorClientImpl::on_spin() {
  // Check for ping
  Duration timeout(MULTITHREADED ? 5.0 : 0.0);
  if (server_->wait_readable(timeout)) {
    const auto conn = server_->accept();
    if (conn) {
      sys_msgs::Operation operation;
      conn->recv_message(operation, operation.get_prefix_len());
      if (operation.opcode == OPCODE::PING) {
        sys_msgs::Status status;
        status.id = info_.id;
        status.error = 0;
        conn->send_message(OPCODE::STATUS_RESPONSE, status);
      }
    }
  }
}

} // namespace detail
} // namespace rix