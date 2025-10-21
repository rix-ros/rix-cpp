#pragma once

#include <memory>
#include <mutex>
#include <set>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/ipc/socket.hpp"
#include "rix/msg/mediator/NodeInfo.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/ParamInfo.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/SrvInfo.hpp"
#include "rix/msg/mediator/SrvRequest.hpp"
#include "rix/msg/mediator/SrvResponse.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/mediator/SubInfo.hpp"
#include "rix/msg/mediator/SubNotify.hpp"
#include "rix/msg/mediator/SystemInfo.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/msg/standard/UInt64.hpp"

namespace rix {

class Mediator : public Spinner {
public:
  Mediator(const Endpoint& endpoint = Endpoint(DEFAULT_IP, RIXHUB_PORT), SocketFactory socket_factory = create_socket);
  ~Mediator();

  Mediator(const Mediator&) = delete;
  Mediator& operator=(const Mediator&) = delete;
  Mediator(Mediator&&) = delete;
  Mediator& operator=(Mediator&&) = delete;

  void on_spin() override;

  size_t get_node_count() const { return nodes_.size(); }
  size_t get_publisher_count() const { return publishers_.size(); }
  size_t get_subscriber_count() const { return subscribers_.size(); }
  size_t get_service_count() const { return services_.size(); }

private:
  std::shared_ptr<GenericSocket> server_{};
  SocketFactory socket_factory_{};
  std::map<uint64_t, msg::mediator::NodeInfo> nodes_{};
  std::map<uint64_t, msg::mediator::PubInfo> publishers_{};
  std::map<uint64_t, msg::mediator::SubInfo> subscribers_{};
  std::map<uint64_t, msg::mediator::SrvInfo> services_{};
  std::map<std::string, std::array<uint64_t, 2>> topic_hashes_{};
  std::map<std::string, std::pair<std::array<uint64_t, 2>, std::vector<uint8_t>>> parameters_{};

  void handle_ping(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_node_register(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_pub_register(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_sub_register(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_srv_register(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_node_deregister(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_pub_deregister(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_sub_deregister(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_srv_deregister(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_srv_request(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_param_set_request(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_param_get_request(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);
  void handle_system_get_request(const msg::mediator::Operation& operation, std::shared_ptr<GenericSocket> conn);

  void notify_subscribers(const std::vector<msg::mediator::SubInfo>& subscribers,
                          const msg::mediator::PubInfo& publisher);
  void notify_subscribers(const msg::mediator::SubInfo& subscriber,
                          const std::vector<msg::mediator::PubInfo>& publishers);

  bool validate_topic_info(const msg::mediator::TopicInfo& info);
  bool validate_service_info(const msg::mediator::SrvInfo& info);
  bool set_parameter(const msg::mediator::ParamInfo& info);
  bool get_parameter(msg::mediator::ParamInfo& info);
};

} // namespace rix