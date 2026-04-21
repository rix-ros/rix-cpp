#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/PubInfo.hpp"
#include "rix/sys_msgs/SubInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Subscriber : public Spinner {
public:
  template <typename TMsg>
  using Callback = std::function<void(const TMsg&)>;
  virtual ~Subscriber() = default;
  virtual size_t get_publisher_count() const = 0;
  template <typename TMsg>
  void set_callback(Callback<TMsg> callback);

protected:
  using CallbackUntyped = std::function<void(const Message&)>;

private:
  virtual void set_callback(CallbackUntyped callback, std::shared_ptr<Message> message) = 0;
};

namespace detail {

class SubscriberImpl final : public Subscriber {
public:
  /**
   * @brief Constructs a SubscriberImpl with the given SubInfo, socket factory, and RIXHub endpoint.
   * @param info The SubInfo message containing subscriber details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  SubscriberImpl(const sys_msgs::SubInfo& info, const Endpoint& rixhub_endpoint);

  // Disable copy and move semantics
  SubscriberImpl(const SubscriberImpl&) = delete;
  SubscriberImpl& operator=(const SubscriberImpl&) = delete;
  SubscriberImpl(SubscriberImpl&&) = delete;
  SubscriberImpl& operator=(SubscriberImpl&&) = delete;

  /**
   * @brief Destructor. Deregisters the subscriber from rixhub.
   */
  ~SubscriberImpl() override;

  /**
   * @brief Returns the number of connected publishers.
   * @return The number of connected publishers.
   */
  size_t get_publisher_count() const override;

private:
  sys_msgs::SubInfo info_;                    ///< The subscriber information.
  std::shared_ptr<Acceptor> server_;          ///< The server socket for incoming connections.
  TransportFactory factory_;                  ///< The socket factory function.
  TransportFactory tcp_factory_;              ///< TCP factory captured at construction for rixhub comms.
  CallbackUntyped callback_;                  ///< The message callback function.
  mutable std::mutex callback_mutex_;         ///< Mutex for protecting the callback.
  std::set<std::shared_ptr<Stream>> clients_; ///< The set of connected publisher sockets.
  Endpoint rixhub_endpoint_;                  ///< The RIXHub endpoint.
  std::atomic<bool> registered_flag_;         ///< Flag indicating if the subscriber is registered.
  std::shared_ptr<Message> msg_instance_;     ///< Prototype message instance for deserialization.
  std::thread spin_thread_;                   ///< The thread running the spin loop.

  /**
   * @brief Internal class to accept new subscriber notifications.
   */
  class SubNotifyAcceptor : public Spinner {
  public:
    /**
     * @brief Constructs a SubNotifyAcceptor with the given parent SubscriberImpl.
     * @param parent The parent SubscriberImpl.
     */
    explicit SubNotifyAcceptor(SubscriberImpl& parent);

    ~SubNotifyAcceptor() override = default;

    // Disable copy and move semantics
    SubNotifyAcceptor(const SubNotifyAcceptor&) = delete;
    SubNotifyAcceptor& operator=(const SubNotifyAcceptor&) = delete;
    SubNotifyAcceptor(SubNotifyAcceptor&&) = delete;
    SubNotifyAcceptor& operator=(SubNotifyAcceptor&&) = delete;

    SubscriberImpl& parent;
    std::thread spin_thread{};

  private:
    /**
     * @brief Internal spin implementation for the SubNotifyAcceptor.
     * @details Accepts new connections and adds them to the parent SubscriberImpl's client set.
     */
    void on_spin() override;
  };

  SubNotifyAcceptor sub_notify_acceptor_{*this};

  // Disable public spin methods (only Node can spin the SubscriberImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  void set_callback(CallbackUntyped callback, std::shared_ptr<Message> message) override;

  /**
   * @brief Internal spin implementation for the SubscriberImpl.
   * @details Receives messages from connected publishers and invokes the callback.
   */
  void on_spin() override;
};
} // namespace detail

template <typename TMsg>
void Subscriber::set_callback(Callback<TMsg> callback) {
  static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be a subclass of Message.");

  auto msg_instance = std::make_shared<TMsg>();
  CallbackUntyped untyped = [callback](const Message& msg) {
    // Safe to static_cast because the hash is verified in the override
    callback(static_cast<const TMsg&>(msg));
  };

  set_callback(untyped, msg_instance);
}

} // namespace rix