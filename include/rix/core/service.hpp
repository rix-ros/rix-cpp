#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/SrvInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Service : public Spinner {
public:
  template <typename TRequest, typename TResponse> using Callback = std::function<void(const TRequest&, TResponse&)>;
  virtual ~Service() = default;
  template <typename TRequest, typename TResponse> void set_callback(Callback<TRequest, TResponse> callback);

protected:
  using CallbackUntyped = std::function<void(const Message&, Message&)>;

private:
  virtual void set_callback(CallbackUntyped callback,
                            std::shared_ptr<Message> request_instance,
                            std::shared_ptr<Message> response_instance) = 0;
};

class Node; // Forward declaration

class ServiceImpl final : public Service {
  friend class Node;

public:
  // Disable copy and move semantics
  ServiceImpl(const ServiceImpl&) = delete;
  ServiceImpl& operator=(const ServiceImpl&) = delete;
  ServiceImpl(ServiceImpl&&) = delete;
  ServiceImpl& operator=(ServiceImpl&&) = delete;

  /**
   * @brief Destructor. Deregisters the service from rixhub.
   */
  ~ServiceImpl() override;

private:
  sys_msgs::SrvInfo info_;                     ///< ServiceImpl information
  std::shared_ptr<Acceptor> server_;           ///< Server socket
  TransportFactory factory_;                   ///< Socket factory function
  CallbackUntyped callback_;                   ///< Callback function for service requests
  mutable std::mutex callback_mutex_;          ///< Mutex for protecting the callback
  Endpoint rixhub_endpoint_;                   ///< RIXHub endpoint
  std::atomic<bool> registered_flag_;          ///< Registration flag
  std::shared_ptr<Message> request_instance_;  ///< Prototype request message
  std::shared_ptr<Message> response_instance_; ///< Prototype response message
  std::thread spin_thread_{};                  ///< Thread running the spin loop

  /**
   * @brief Constructs a ServiceImpl with the given SrvInfo, socket factory, and RIXHub endpoint.
   * @param info The SrvInfo message containing service details.
   * @param socket_factory The socket factory function.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  ServiceImpl(const sys_msgs::SrvInfo& info, const Endpoint& rixhub_endpoint);

  // Disable public spin methods (only Node can spin the ServiceImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  void set_callback(CallbackUntyped callback,
                    std::shared_ptr<Message> request_instance,
                    std::shared_ptr<Message> response_instance) override;

  /**
   * @brief Internal spin implementation for the ServiceImpl.
   */
  void on_spin() override;
};

template <typename TRequest, typename TResponse> void Service::set_callback(Callback<TRequest, TResponse> callback) {
  static_assert(std::is_base_of_v<Message, TRequest>, "TRequest must be a subclass of Message.");
  static_assert(std::is_base_of_v<Message, TResponse>, "TResponse must be a subclass of Message.");

  auto request_instance = std::make_shared<TRequest>();
  auto response_instance = std::make_shared<TResponse>();
  auto untyped = [callback](const Message& request, Message& response) {
    // Safe to static cast because we checked the hash above
    const auto& typed_request = static_cast<const TRequest&>(request);
    auto& typed_response = static_cast<TResponse&>(response);
    callback(typed_request, typed_response);
  };

  set_callback(untyped, request_instance, response_instance);
}

} // namespace rix