#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/SrvInfo.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix::core {

class Node; // Forward declaration

class Service : public Spinner {
  friend class Node;

public:
  using Callback = std::function<void(const rix::msg::Message &request, rix::msg::Message &response)>;

  Service(const Service &) = delete;
  Service &operator=(const Service &) = delete;
  ~Service();

  virtual bool ok() const override;
  virtual void shutdown() override;

  template <typename TRequest, typename TResponse>
  void set_callback(std::function<void(const TRequest &, TResponse &)> callback);

private:
  rix::msg::mediator::SrvInfo info_;
  std::shared_ptr<rix::ipc::GenericSocket> server_;
  SocketFactory socket_factory_;
  Callback callback_;
  mutable std::mutex callback_mutex_;
  rix::ipc::Endpoint rixhub_endpoint_;
  std::atomic<bool> shutdown_flag_;
  std::atomic<bool> registered_flag_;
  std::shared_ptr<rix::msg::Message> request_instance_;
  std::shared_ptr<rix::msg::Message> response_instance_;

  Service(const rix::msg::mediator::SrvInfo &info, SocketFactory socket_factory,
          const rix::ipc::Endpoint &rixhub_endpoint);

  using Spinner::spin;
  virtual void spin_once() override;
};

template <typename TRequest, typename TResponse>
void Service::set_callback(std::function<void(const TRequest &, TResponse &)> callback) {
  static_assert(std::is_base_of<rix::msg::Message, TRequest>::value,
                "TRequest must be a subclass of rix::msg::Message.");
  static_assert(std::is_base_of<rix::msg::Message, TResponse>::value,
                "TResponse must be a subclass of rix::msg::Message.");

  if (TRequest().hash() != info_.request_hash || TResponse().hash() != info_.response_hash) {
    rix::util::Log::warn << "Message type mismatch in Service::set_callback." << std::endl;
    return;
  }

  std::lock_guard<std::mutex> guard(callback_mutex_);
  callback_ = [callback](const rix::msg::Message &request, rix::msg::Message &response) {
    // Safe to static cast because we checked the hash above
    const TRequest &typed_request = static_cast<const TRequest &>(request);
    TResponse &typed_response = static_cast<TResponse &>(response);
    callback(typed_request, typed_response);
  };
  request_instance_ = std::make_shared<TRequest>();
  response_instance_ = std::make_shared<TResponse>();
}

} // namespace rix::core