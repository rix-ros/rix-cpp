#include "rix/core/node.hpp"
#include "rix/std_msgs/UInt64.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include "rix/sys_msgs/Status.hpp"

namespace rix {

Node::Node(const std::string& name, const Endpoint& endpoint)
    : rixhub_endpoint_(Endpoint(RIXHUB_IP, RIXHUB_PORT)), registered_flag_(false) {

  const TransportFactory& transport_factory = transport_factories[Protocol::TCP];

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

  info_.id = id_factory_();
  info_.name = name;

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

  // Create timer to handle pings at 1Hz
  create_timer(Duration(1.0), [this](const TimerCallback::Event&) {
    // Check for ping
    // std::cout << "Checking for ping..." << std::endl;
    if (server_->is_readable()) {
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
  });
}

Node::~Node() {
  const TransportFactory& transport_factory = transport_factories[Protocol::TCP];
  if (registered_flag_) {
    const auto client = transport_factory.create_stream(rixhub_endpoint_, true);
    if (!client) {
      return;
    }
    client->send_message(OPCODE::NODE_DEREGISTER, info_);
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
    if (!MULTITHREADED) {
      component->spin_once();
    }
    ++it;
  }

  if (MULTITHREADED) {
    // Sleep to prevent busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

std::shared_ptr<Publisher> Node::create_publisher(const sys_msgs::TopicInfo& topic_info,
                                                  const Endpoint& rixhub_endpoint,
                                                  const Endpoint& endpoint,
                                                  Protocol protocol) {
  sys_msgs::PubInfo pub_info;
  pub_info.id = id_factory_();
  pub_info.node_id = info_.id;
  pub_info.topic_info = topic_info;
  pub_info.endpoint.address = endpoint.address;
  pub_info.endpoint.port = endpoint.port;
  const TransportFactory& transport_factory = transport_factories[protocol];
  auto pub = std::shared_ptr<Publisher>(new Publisher(pub_info, transport_factory, rixhub_endpoint_));
  components_.push_back(pub);
  return pub;
}

std::shared_ptr<Subscriber> Node::create_subscriber(const sys_msgs::TopicInfo& topic_info,
                                                    const Endpoint& rixhub_endpoint,
                                                    const Endpoint& endpoint,
                                                    Protocol protocol) {
  sys_msgs::SubInfo sub_info;
  sub_info.id = id_factory_();
  sub_info.node_id = info_.id;
  sub_info.topic_info = topic_info;
  sub_info.endpoint.address = endpoint.address;
  sub_info.endpoint.port = endpoint.port;
  auto sub = std::shared_ptr<Subscriber>(new Subscriber(sub_info, transport_factories[protocol], rixhub_endpoint_));
  components_.push_back(sub);
  return sub;
}

std::shared_ptr<Service> Node::create_service(sys_msgs::SrvInfo& service_info,
                                              const Endpoint& rixhub_endpoint,
                                              const Endpoint& endpoint,
                                              Protocol protocol) {
  service_info.id = id_factory_();
  service_info.node_id = info_.id;
  service_info.endpoint.address = endpoint.address;
  service_info.endpoint.port = endpoint.port;
  auto srv = std::shared_ptr<Service>(new Service(service_info, transport_factories[protocol], rixhub_endpoint_));
  components_.push_back(srv);
  return srv;
}

std::shared_ptr<Action> Node::create_action(sys_msgs::ActInfo& action_info,
                                            const Endpoint& rixhub_endpoint,
                                            const Endpoint& endpoint,
                                            Protocol protocol) {
  action_info.id = id_factory_();
  action_info.node_id = info_.id;
  action_info.endpoint.address = endpoint.address;
  action_info.endpoint.port = endpoint.port;
  auto act = std::shared_ptr<Action>(new Action(action_info, transport_factories[protocol], rixhub_endpoint_));
  components_.push_back(act);
  return act;
}

bool Node::get_system_info(sys_msgs::SystemInfo& info) {
  auto client = transport_factories[Protocol::TCP].create_stream(rixhub_endpoint_, true);

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
  Log::debug << "Retrieved system info from RIXHub." << std::endl;
  return true;
}

std::shared_ptr<ServiceClient> Node::create_service_client(const sys_msgs::SrvRequest& service_request,
                                                           const Endpoint& rixhub_endpoint,
                                                           Protocol protocol) {
  auto srv_cli = std::shared_ptr<ServiceClient>(
      new ServiceClient(service_request, transport_factories[protocol], rixhub_endpoint));
  components_.push_back(srv_cli);
  return srv_cli;
}

std::shared_ptr<ActionClient> Node::create_action_client(const sys_msgs::ActRequest& action_request,
                                                         const Endpoint& rixhub_endpoint,
                                                         Protocol protocol) {
  auto act_cli =
      std::shared_ptr<ActionClient>(new ActionClient(action_request, transport_factories[protocol], rixhub_endpoint));
  components_.push_back(act_cli);
  return act_cli;
}

} // namespace rix