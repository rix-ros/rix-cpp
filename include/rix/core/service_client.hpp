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

namespace rix::core {

class Node; // Forward declaration

class ServiceClient : Spinner {
  friend class Node;

public:
  ServiceClient(const ServiceClient &) = delete;
  ServiceClient &operator=(const ServiceClient &) = delete;
  ~ServiceClient();

  virtual bool ok() const override;
  virtual void shutdown() override;
  bool call(const rix::msg::Message &request, rix::msg::Message &response);

private:
  rix::msg::mediator::SrvRequest request_;
  SocketFactory socket_factory_;
  std::atomic<bool> shutdown_flag_;
  rix::ipc::Endpoint endpoint_;

  using Spinner::spin;
  virtual void spin_once() override;

  ServiceClient(const rix::msg::mediator::SrvRequest &request, SocketFactory factory,
                const rix::ipc::Endpoint &rixhub_endpoint);
};

} // namespace rix::core