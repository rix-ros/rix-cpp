#pragma once

#include <memory>
#include <mutex>
#include <map> 

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/ipc.hpp"
#include "rix/sys_msgs/NodeInfo.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include "rix/sys_msgs/ParamInfo.hpp"
#include "rix/sys_msgs/PubInfo.hpp"
#include "rix/sys_msgs/SrvInfo.hpp"
#include "rix/sys_msgs/SubInfo.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"

namespace rix {

class Mediator final : public Spinner {
public:
  explicit Mediator(const Endpoint& endpoint = Endpoint(DEFAULT_IP, RIXHUB_PORT));
  ~Mediator() override;

  Mediator(const Mediator&) = delete;
  Mediator& operator=(const Mediator&) = delete;
  Mediator(Mediator&&) = delete;
  Mediator& operator=(Mediator&&) = delete;

  void on_spin() override;

  size_t get_node_count() const { return nodes_.size(); }
  size_t get_publisher_count() const { return publishers_.size(); }
  size_t get_subscriber_count() const { return subscribers_.size(); }
  size_t get_service_count() const { return services_.size(); }
  size_t get_action_count() const { return actions_.size(); }

private:
  std::shared_ptr<Acceptor> server_{};
  TransportFactory socket_factory_{};
  std::map<uint64_t, sys_msgs::NodeInfo> nodes_{};
  std::map<uint64_t, sys_msgs::PubInfo> publishers_{};
  std::map<uint64_t, sys_msgs::SubInfo> subscribers_{};
  std::map<uint64_t, sys_msgs::SrvInfo> services_{};
  std::map<uint64_t, sys_msgs::ActInfo> actions_{};
  std::map<std::string, std::array<uint64_t, 2>> topic_hashes_{};
  std::map<std::string, std::pair<std::array<uint64_t, 2>, std::vector<uint8_t>>> parameters_{};

  void handle_ping(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_node_register(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_pub_register(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_sub_register(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_srv_register(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_act_register(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_node_deregister(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_pub_deregister(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_sub_deregister(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_srv_deregister(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_act_deregister(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_srv_request(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_act_request(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_param_set_request(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_param_get_request(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);
  void handle_system_get_request(const sys_msgs::Operation& operation, std::shared_ptr<Stream> conn);

  void notify_subscribers(const std::vector<sys_msgs::SubInfo>& subscribers,
                          const sys_msgs::PubInfo& publisher);
  void notify_subscribers(const sys_msgs::SubInfo& subscriber,
                          const std::vector<sys_msgs::PubInfo>& publishers);

  bool validate_topic_info(const sys_msgs::TopicInfo& info);
  bool validate_service_info(const sys_msgs::SrvInfo& info);
  bool validate_action_info(const sys_msgs::ActInfo& info);
  bool set_parameter(const sys_msgs::ParamInfo& info);
  bool get_parameter(sys_msgs::ParamInfo& info);
};

} // namespace rix