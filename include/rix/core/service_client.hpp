#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/ipc/client.hpp"
#include "rix/ipc/server.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/SrvRequest.hpp"
#include "rix/msg/mediator/SrvResponse.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix::core {

class Node; // Forward declaration

class ServiceClient : Spinner {
  /**
   * @brief We declare the Node as a friend class so that its factory method
   * create_service has access to the private constructor.
   */
  friend class Node;

public:
  ServiceClient(const ServiceClient &) = delete;
  ServiceClient &operator=(const ServiceClient &) = delete;
  ~ServiceClient();

  virtual bool ok() const override;
  virtual void shutdown() override;

  template <typename TRequest, typename TResponse>
  bool call(const TRequest &request, TResponse &response);

private:
  rix::msg::mediator::SrvRequest request_;
  ClientFactory factory_;
  std::atomic<bool> shutdown_flag_;
  rix::ipc::Endpoint endpoint_;

  using Spinner::spin;
  virtual void spin_once() override;

  bool call(const std::vector<uint8_t> &request,
            std::vector<uint8_t> &response);

  ServiceClient(const rix::msg::mediator::SrvRequest &request,
                ClientFactory factory,
                const rix::ipc::Endpoint &rixhub_endpoint);
};

template <typename TRequest, typename TResponse>
bool ServiceClient::call(const TRequest &request, TResponse &response) {
  static_assert(std::is_base_of<rix::msg::Message, TRequest>::value,
                "TRequest must be a subclass of rix::msg::Message.");
  static_assert(std::is_base_of<rix::msg::Message, TResponse>::value,
                "TResponse must be a subclass of rix::msg::Message.");

  /**
   * Ensure that the function's message type matches the message type of the
   * service's topic.
   */
  if (TRequest().hash() != request_.request_hash ||
      TResponse().hash() != request_.response_hash) {
    rix::util::Log::warn << "Message type mismatch in Service::call."
                         << std::endl;
    return false;
  }

  rix::msg::standard::UInt32 size;
  size.data = request.size();
  std::vector<uint8_t> req(size.size() + size.data);
  size_t offset = 0;
  size.serialize(req.data(), offset);
  request.serialize(req.data(), offset);

  std::vector<uint8_t> res;
  if (!call(req, res))
    return false;

  offset = 0;
  return response.deserialize(res.data(), res.size(), offset);
}

} // namespace rix::core