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
  template <typename TMsg> using Callback = std::function<void(const TMsg&)>;

  Subscriber(const Subscriber&) = delete;
  Subscriber& operator=(const Subscriber&) = delete;
  ~Subscriber() override;

  template <typename TMsg> void set_callback(Callback<TMsg> callback);

  size_t get_publisher_count() const;

private:
  using CallbackUntyped = std::function<void(const Message&)>;
  sys_msgs::SubInfo info_;
  std::shared_ptr<GenericSocket> server_;
  SocketFactory socket_factory_;
  CallbackUntyped callback_;
  mutable std::mutex callback_mutex_;
  std::set<std::shared_ptr<GenericSocket>> clients_;
  Endpoint rixhub_endpoint_;
  std::atomic<bool> registered_flag_;
  std::shared_ptr<Message> msg_instance_;
  std::thread spin_thread_;

  Subscriber(const sys_msgs::SubInfo& info, SocketFactory factory, const Endpoint& rixhub_endpoint);

  // Internal class to handle accepting new connections from rixhub
  class SubNotifyAcceptor : public Spinner {
  public:
    explicit SubNotifyAcceptor(Subscriber& parent);
    ~SubNotifyAcceptor() override = default;

    SubNotifyAcceptor(const SubNotifyAcceptor&) = delete;
    SubNotifyAcceptor& operator=(const SubNotifyAcceptor&) = delete;
    SubNotifyAcceptor(SubNotifyAcceptor&&) = delete;
    SubNotifyAcceptor& operator=(SubNotifyAcceptor&&) = delete;

    void on_spin() override;

    Subscriber& parent;
    std::thread spin_thread{};
  };

  SubNotifyAcceptor sub_notify_acceptor_{*this};

  using Spinner::spin;
  using Spinner::spin_once;
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