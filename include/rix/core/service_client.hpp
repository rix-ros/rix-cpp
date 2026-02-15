#pragma once

#include <functional>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/SrvRequest.hpp"

namespace rix {

class Node; // Forward declaration

class ServiceClient final : public Spinner {
  friend class Node;

public:
  // Disable copy and move semantics
  ServiceClient(const ServiceClient&) = delete;
  ServiceClient& operator=(const ServiceClient&) = delete;
  ServiceClient(ServiceClient&&) = delete;
  ServiceClient& operator=(ServiceClient&&) = delete;

  /**
   * @brief Destructor. Cleans up the ServiceClient.
   */
  ~ServiceClient() override;

  /**
   * @brief Calls the service with the given request and fills the response.
   * @param request The request message.
   * @param response The response message to be filled.
   * @return true if the call was successful, false otherwise.
   */
  bool call(const Message& request, Message& response);

private:
  sys_msgs::SrvRequest request_; ///< The service request information.
  TransportFactory socket_factory_; ///< Socket factory function.
  Endpoint endpoint_;            ///< Endpoint of the service.
  std::thread spin_thread_{};    ///< Thread running the spin loop.

  // Disable public spin methods (only Node can spin the ServiceClient)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the ServiceClient.
   */
  void on_spin() override;

  /**
   * @brief Constructs a ServiceClient with the given SrvRequest, socket factory, and RIXHub endpoint.
   * @param request The SrvRequest message containing service request details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  ServiceClient(const sys_msgs::SrvRequest& request, TransportFactory factory, const Endpoint& rixhub_endpoint);
};

} // namespace rix