#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/ipc/client.hpp"
#include "rix/ipc/server.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/SrvInfo.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix::core {

class Node; // Forward declaration

class Service : public Spinner {
  /**
   * @brief We declare the Node as a friend class so that its factory method
   * create_service has access to the private constructor.
   */
  friend class Node;

public:
  using SerializedCallback = std::function<void(
      const std::vector<uint8_t> &request, std::vector<uint8_t> &response)>;

  Service(const Service &) = delete;
  Service &operator=(const Service &) = delete;
  ~Service();

  virtual bool ok() const override;
  virtual void shutdown() override;

  template <typename TRequest, typename TResponse>
  void
  set_callback(std::function<void(const TRequest &, TResponse &)> callback);
  SerializedCallback get_callback() const;

private:
  rix::msg::mediator::SrvInfo info_;
  std::shared_ptr<rix::ipc::Server> server_;
  ClientFactory factory_;
  SerializedCallback callback_;
  mutable std::mutex callback_mutex_;
  rix::ipc::Endpoint rixhub_endpoint_;
  std::atomic<bool> shutdown_flag_;

  Service(const rix::msg::mediator::SrvInfo &info,
          std::shared_ptr<rix::ipc::Server> server, ClientFactory factory,
          const rix::ipc::Endpoint &rixhub_endpoint);

  using Spinner::spin;
  virtual void spin_once() override;
};

template <typename TRequest, typename TResponse>
void Service::set_callback(
    std::function<void(const TRequest &, TResponse &)> callback) {
  static_assert(std::is_base_of<rix::msg::Message, TRequest>::value,
                "TRequest must be a subclass of rix::msg::Message.");
  static_assert(std::is_base_of<rix::msg::Message, TResponse>::value,
                "TResponse must be a subclass of rix::msg::Message.");

  /**
   * Ensure that the function's message type matches the message type of the
   * service's topic.
   */
  if (TRequest().hash() != info_.request_hash ||
      TResponse().hash() != info_.response_hash) {
    rix::util::Log::warn << "Message type mismatch in Service::set_callback."
                         << std::endl;
    return;
  }

  /**
   * Protect access to the callback (we do not want to change the callback if
   * clients are actively processing data)
   */
  std::lock_guard<std::mutex> guard(callback_mutex_);

  callback_ = [callback](const std::vector<uint8_t> &request,
                         std::vector<uint8_t> &response) {
    TRequest req_obj;
    size_t offset = 0;
    if (!req_obj.deserialize(request.data(), request.size(), offset)) {
      rix::util::Log::warn << "Failed to deserialize message from publisher."
                           << std::endl;
      return;
    }
    TResponse res_obj;
    callback(req_obj, res_obj);

    rix::msg::standard::UInt32 size;
    size.data = res_obj.size();
    response.resize(size.size() + size.data);
    offset = 0;
    size.serialize(response.data(), offset);
    res_obj.serialize(response.data(), offset);
  };
}

} // namespace rix::core