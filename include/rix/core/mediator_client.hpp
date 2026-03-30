#pragma once

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/ipc.hpp"
#include "rix/std_msgs/UInt64.hpp"
#include "rix/sys_msgs/NodeInfo.hpp"
#include "rix/sys_msgs/ParamInfo.hpp"
#include "rix/sys_msgs/Status.hpp"
#include "rix/sys_msgs/SystemInfo.hpp"
#include "rix/util.hpp"

namespace rix {

class MediatorClient : public Spinner {
public:
  virtual ~MediatorClient() = default;

  virtual bool set_parameter(const std::string& name, const Message& parameter) const = 0;
  virtual bool get_parameter(const std::string& name, Message& parameter) const = 0;
  virtual bool get_system_info(sys_msgs::SystemInfo& info) = 0;
};

namespace detail {

class MediatorClientImpl final : public MediatorClient {
public:
  MediatorClientImpl(sys_msgs::NodeInfo& node_info, const Endpoint& endpoint, const Endpoint& rixhub_endpoint);

  ~MediatorClientImpl() override;

  /**
   * @brief Sets a parameter on the RIXHub parameter server.
   * @param name The name of the parameter.
   * @param parameter The parameter value to set.
   * @return true if the parameter was set successfully, false otherwise.
   */
  bool set_parameter(const std::string& name, const Message& parameter) const override;

  /**
   * @brief Gets a parameter from the RIXHub parameter server.
   * @param name The name of the parameter.
   * @param parameter The variable to store the retrieved parameter value.
   * @return true if the parameter was retrieved successfully, false otherwise.
   */
  bool get_parameter(const std::string& name, Message& parameter) const override;

  /**
   * @brief Retrieves system information from RIXHub.
   * @param info The SystemInfo message to populate with system information.
   * @return true if the system information was retrieved successfully, false otherwise.
   */
  bool get_system_info(sys_msgs::SystemInfo& info) override;

private:
  sys_msgs::NodeInfo& info_;          ///< Node information.
  Endpoint rixhub_endpoint_;          ///< The RIXHub endpoint.
  std::shared_ptr<Acceptor> server_;  ///< Server socket for the node.
  std::atomic<bool> registered_flag_; ///< Flag indicating if the node is registered with RIXHub.
  std::thread spin_thread_;           ///< Thread for spinning the MediatorClient.

  /**
   * @brief Internal spin implementation for the MediatorClientImpl.
   */
  void on_spin() override;
};

} // namespace detail
} // namespace rix