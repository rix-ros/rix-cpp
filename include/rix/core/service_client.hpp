#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/SrvRequest.hpp"
#include "rix/msg/mediator/SrvResponse.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class ServiceClient : public Spinner {
  friend class Node;

public:
  ServiceClient(const ServiceClient&) = delete;
  ServiceClient& operator=(const ServiceClient&) = delete;
  ServiceClient(ServiceClient&&) = delete;
  ServiceClient& operator=(ServiceClient&&) = delete;

  ~ServiceClient();

  bool call(const msg::Message& request, msg::Message& response);

private:
  msg::mediator::SrvRequest request_;
  SocketFactory socket_factory_;
  Endpoint endpoint_;

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_{};
#endif

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;

  ServiceClient(const msg::mediator::SrvRequest& request, SocketFactory factory, const Endpoint& rixhub_endpoint);
};

} // namespace rix