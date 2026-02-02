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

class Node; // Forward declaration

class Subscriber final : public Spinner {
  friend class Node;

public:
  /**
   * @brief Callback type definition for message handling.
   * @tparam TMsg The message type.
   */
  template <typename TMsg> using Callback = std::function<void(const TMsg&)>;

  // Disable copy and move semantics
  Subscriber(const Subscriber&) = delete;
  Subscriber& operator=(const Subscriber&) = delete;
  Subscriber(Subscriber&&) = delete;
  Subscriber& operator=(Subscriber&&) = delete;

  /**
   * @brief Destructor. Deregisters the subscriber from rixhub.
   */
  ~Subscriber() override;

  /**
   * @brief Sets the callback function to be invoked on message receipt.
   * @tparam TMsg The message type.
   * @param callback The callback function.
   */
  template <typename TMsg> void set_callback(Callback<TMsg> callback);

  /**
   * @brief Returns the number of connected publishers.
   * @return The number of connected publishers.
   */
  size_t get_publisher_count() const;

private:
  /**
   * @brief Callback type definition for untyped message handling.
   */
  using CallbackUntyped = std::function<void(const Message&)>;

  sys_msgs::SubInfo info_;                           ///< The subscriber information.
  std::shared_ptr<GenericSocket> server_;            ///< The server socket for incoming connections.
  SocketFactory socket_factory_;                     ///< The socket factory function.
  CallbackUntyped callback_;                         ///< The message callback function.
  mutable std::mutex callback_mutex_;                ///< Mutex for protecting the callback.
  std::set<std::shared_ptr<GenericSocket>> clients_; ///< The set of connected publisher sockets.
  Endpoint rixhub_endpoint_;                         ///< The RIXHub endpoint.
  std::atomic<bool> registered_flag_;                ///< Flag indicating if the subscriber is registered.
  std::shared_ptr<Message> msg_instance_;            ///< Prototype message instance for deserialization.
  std::thread spin_thread_;                          ///< The thread running the spin loop.

  /**
   * @brief Constructs a Subscriber with the given SubInfo, socket factory, and RIXHub endpoint.
   * @param info The SubInfo message containing subscriber details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  Subscriber(const sys_msgs::SubInfo& info, SocketFactory factory, const Endpoint& rixhub_endpoint);

  /**
   * @brief Internal class to accept new subscriber notifications.
   */
  class SubNotifyAcceptor : public Spinner {
  public:
    /**
     * @brief Constructs a SubNotifyAcceptor with the given parent Subscriber.
     * @param parent The parent Subscriber.
     */
    explicit SubNotifyAcceptor(Subscriber& parent);

    ~SubNotifyAcceptor() override = default;

    // Disable copy and move semantics
    SubNotifyAcceptor(const SubNotifyAcceptor&) = delete;
    SubNotifyAcceptor& operator=(const SubNotifyAcceptor&) = delete;
    SubNotifyAcceptor(SubNotifyAcceptor&&) = delete;
    SubNotifyAcceptor& operator=(SubNotifyAcceptor&&) = delete;

    Subscriber& parent;
    std::thread spin_thread{};

  private:
    /**
     * @brief Internal spin implementation for the SubNotifyAcceptor.
     * @details Accepts new connections and adds them to the parent Subscriber's client set.
     */
    void on_spin() override;
  };

  SubNotifyAcceptor sub_notify_acceptor_{*this};

  // Disable public spin methods (only Node can spin the Subscriber)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the Subscriber.
   * @details Receives messages from connected publishers and invokes the callback.
   */
  void on_spin() override;
};

template <typename TMsg> void Subscriber::set_callback(Callback<TMsg> callback) {
  static_assert(std::is_base_of_v<Message, TMsg>, "TMsg must be a subclass of Message.");

  if (TMsg().hash() != info_.topic_info.message_hash) {
    Log::warn << "Message type mismatch in set_callback." << std::endl;
    return;
  }
  std::lock_guard guard(callback_mutex_);
  msg_instance_ = std::make_shared<TMsg>();
  callback_ = [callback](const Message& msg) {
    // Safe to static cast because we checked the hash above
    const auto& typed_msg = static_cast<const TMsg&>(msg);
    callback(typed_msg);
  };
}

} // namespace rix