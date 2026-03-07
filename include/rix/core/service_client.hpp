#pragma once

#include <functional>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/SrvRequest.hpp"

namespace rix {

class ServiceClient : public Spinner {
public:
  virtual ~ServiceClient() = default;
  virtual bool call(const Message& request, Message& response) = 0;
};

class Node; // Forward declaration

class ServiceClientImpl final : public ServiceClient {
  friend class Node;

public:
  // Disable copy and move semantics
  ServiceClientImpl(const ServiceClientImpl&) = delete;
  ServiceClientImpl& operator=(const ServiceClientImpl&) = delete;
  ServiceClientImpl(ServiceClientImpl&&) = delete;
  ServiceClientImpl& operator=(ServiceClientImpl&&) = delete;

  /**
   * @brief Destructor. Cleans up the ServiceClientImpl.
   */
  ~ServiceClientImpl() override;

  /**
   * @brief Calls the service with the given request and fills the response.
   * @param request The request message.
   * @param response The response message to be filled.
   * @return true if the call was successful, false otherwise.
   */
  bool call(const Message& request, Message& response) override;

private:
  sys_msgs::SrvRequest request_; ///< The service request information.
  TransportFactory factory_;     ///< Socket factory function.
  Endpoint endpoint_;            ///< Endpoint of the service.
  std::thread spin_thread_{};    ///< Thread running the spin loop.

  // Disable public spin methods (only Node can spin the ServiceClientImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the ServiceClientImpl.
   */
  void on_spin() override;

  /**
   * @brief Constructs a ServiceClientImpl with the given SrvRequest, socket factory, and RIXHub endpoint.
   * @param request The SrvRequest message containing service request details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  ServiceClientImpl(const sys_msgs::SrvRequest& request, const Endpoint& rixhub_endpoint);
};

} // namespace rix