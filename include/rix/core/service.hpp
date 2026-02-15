#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/SrvInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class Service final : public Spinner {
  friend class Node;

public:
  /**
   * @brief Callback type definition for service requests.
   * @tparam TRequest The request message type.
   * @tparam TResponse The response message type.
   */
  template <typename TRequest, typename TResponse> using Callback = std::function<void(const TRequest&, TResponse&)>;

  // Disable copy and move semantics
  Service(const Service&) = delete;
  Service& operator=(const Service&) = delete;
  Service(Service&&) = delete;
  Service& operator=(Service&&) = delete;

  /**
   * @brief Destructor. Deregisters the service from rixhub.
   */
  ~Service() override;

  /**
   * @brief Sets the callback function to be invoked on service requests.
   * @tparam TRequest The request message type.
   * @tparam TResponse The response message type.
   * @param callback The callback function.
   */
  template <typename TRequest, typename TResponse> void set_callback(Callback<TRequest, TResponse> callback);

private:
  /**
   * @brief Callback type definition for untyped service requests.
   */
  using CallbackUntyped = std::function<void(const Message&, Message&)>;

  sys_msgs::SrvInfo info_;                     ///< Service information
  std::shared_ptr<Acceptor> server_;      ///< Server socket
  TransportFactory socket_factory_;               ///< Socket factory function
  CallbackUntyped callback_;                   ///< Callback function for service requests
  mutable std::mutex callback_mutex_;          ///< Mutex for protecting the callback
  Endpoint rixhub_endpoint_;                   ///< RIXHub endpoint
  std::atomic<bool> registered_flag_;          ///< Registration flag
  std::shared_ptr<Message> request_instance_;  ///< Prototype request message
  std::shared_ptr<Message> response_instance_; ///< Prototype response message
  std::thread spin_thread_{};                  ///< Thread running the spin loop

  /**
   * @brief Constructs a Service with the given SrvInfo, socket factory, and RIXHub endpoint.
   * @param info The SrvInfo message containing service details.
   * @param socket_factory The socket factory function.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  Service(const sys_msgs::SrvInfo& info, TransportFactory socket_factory, const Endpoint& rixhub_endpoint);

  // Disable public spin methods (only Node can spin the Service)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the Service.
   */
  void on_spin() override;
};

template <typename TRequest, typename TResponse> void Service::set_callback(Callback<TRequest, TResponse> callback) {
  static_assert(std::is_base_of_v<Message, TRequest>, "TRequest must be a subclass of Message.");
  static_assert(std::is_base_of_v<Message, TResponse>, "TResponse must be a subclass of Message.");

  if (TRequest().hash() != info_.request_hash || TResponse().hash() != info_.response_hash) {
    Log::warn << "Message type mismatch in Service::set_callback." << std::endl;
    return;
  }

  std::lock_guard<std::mutex> guard(callback_mutex_);
  callback_ = [callback](const Message& request, Message& response) {
    // Safe to static cast because we checked the hash above
    const auto& typed_request = static_cast<const TRequest&>(request);
    auto& typed_response = static_cast<TResponse&>(response);
    callback(typed_request, typed_response);
  };
  request_instance_ = std::make_shared<TRequest>();
  response_instance_ = std::make_shared<TResponse>();
}

} // namespace rix