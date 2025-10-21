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

namespace rix {

class Node; // Forward declaration

class Service : public Spinner {
  friend class Node;

public:
  template <typename TRequest, typename TResponse>
  using Callback = std::function<void(const TRequest&, TResponse&)>;

  Service(const Service&) = delete;
  Service& operator=(const Service&) = delete;
  Service(Service&&) = delete;
  Service& operator=(Service&&) = delete;
  ~Service();

  template <typename TRequest, typename TResponse>
  void set_callback(Callback<TRequest, TResponse> callback);

private:
  using CallbackUntyped = std::function<void(const msg::Message&, msg::Message&)>;
  msg::mediator::SrvInfo         info_;
  std::shared_ptr<GenericSocket> server_;
  SocketFactory                  socket_factory_;
  CallbackUntyped                callback_;
  mutable std::mutex             callback_mutex_;
  Endpoint                       rixhub_endpoint_;
  std::atomic<bool>              registered_flag_;
  std::shared_ptr<msg::Message>  request_instance_;
  std::shared_ptr<msg::Message>  response_instance_;

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_{};
#endif

  Service(const msg::mediator::SrvInfo& info,
          SocketFactory                 socket_factory,
          const Endpoint&               rixhub_endpoint);

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;
};

template <typename TRequest, typename TResponse>
void Service::set_callback(Callback<TRequest, TResponse> callback) {
  static_assert(std::is_base_of<msg::Message, TRequest>::value,
                "TRequest must be a subclass of msg::Message.");
  static_assert(std::is_base_of<msg::Message, TResponse>::value,
                "TResponse must be a subclass of msg::Message.");

  if (TRequest().hash() != info_.request_hash ||
      TResponse().hash() != info_.response_hash) {
    Log::warn << "Message type mismatch in Service::set_callback." << std::endl;
    return;
  }

  std::lock_guard<std::mutex> guard(callback_mutex_);
  callback_ = [callback](const msg::Message& request, msg::Message& response) {
    // Safe to static cast because we checked the hash above
    const TRequest& typed_request = static_cast<const TRequest&>(request);
    TResponse&      typed_response = static_cast<TResponse&>(response);
    callback(typed_request, typed_response);
  };
  request_instance_ = std::make_shared<TRequest>();
  response_instance_ = std::make_shared<TResponse>();
}

} // namespace rix