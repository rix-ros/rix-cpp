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

namespace rix::core {

class Mediator : public Spinner {
public:
  Mediator(const rix::ipc::Endpoint &rixhub_endpoint, SocketFactory socket_factory = rix::ipc::create_socket);
  ~Mediator();

  virtual bool ok() const override;
  virtual void shutdown() override;
  virtual void spin_once() override;

  size_t get_node_count() const { return nodes_.size(); }
  size_t get_publisher_count() const { return publishers_.size(); }
  size_t get_subscriber_count() const { return subscribers_.size(); }
  size_t get_service_count() const { return services_.size(); }

private:
  std::shared_ptr<rix::ipc::GenericSocket> server_;
  SocketFactory socket_factory_;
  std::map<uint64_t, rix::msg::mediator::NodeInfo> nodes_;
  std::map<uint64_t, rix::msg::mediator::PubInfo> publishers_;
  std::map<uint64_t, rix::msg::mediator::SubInfo> subscribers_;
  std::map<uint64_t, rix::msg::mediator::SrvInfo> services_;
  std::map<std::string, std::array<uint64_t, 2>> topic_hashes_;
  std::map<std::string, std::pair<std::array<uint64_t, 2>, std::vector<uint8_t>>> parameters_;
  std::atomic<bool> shutdown_flag_;

  void notify_subscribers(const std::vector<rix::msg::mediator::SubInfo> &subscribers,
                          const rix::msg::mediator::PubInfo &publisher);
  void notify_subscribers(const rix::msg::mediator::SubInfo &subscriber,
                          const std::vector<rix::msg::mediator::PubInfo> &publishers);

  bool validate_topic_info(const rix::msg::mediator::TopicInfo &info);
  bool set_parameter(const rix::msg::mediator::ParamInfo &info);
  bool get_parameter(rix::msg::mediator::ParamInfo &info);
};

} // namespace rix::core