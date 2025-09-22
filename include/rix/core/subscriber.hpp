#pragma once

#include <functional>
#include <memory>
#include <set>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/mediator/SubInfo.hpp"
#include "rix/msg/mediator/SubNotify.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix::core {

class Node; // Forward declaration

class Subscriber : public Spinner {
  friend class Node;

public:
  using Callback = std::function<void(const rix::msg::Message &)>;

  Subscriber(const Subscriber &) = delete;
  Subscriber &operator=(const Subscriber &) = delete;
  ~Subscriber();

  virtual bool ok() const override;
  virtual void shutdown() override;

  template <typename TMsg> void set_callback(std::function<void(const TMsg &)> callback);

  size_t get_publisher_count() const;

private:
  rix::msg::mediator::SubInfo info_;
  std::shared_ptr<rix::ipc::GenericSocket> server_;
  SocketFactory socket_factory_;
  Callback callback_;
  mutable std::mutex callback_mutex_;
  std::set<std::shared_ptr<rix::ipc::GenericSocket>> clients_;
  rix::ipc::Endpoint rixhub_endpoint_;
  std::atomic<bool> shutdown_flag_;
  std::atomic<bool> registered_flag_;
  std::shared_ptr<rix::msg::Message> msg_instance_;

  Subscriber(const rix::msg::mediator::SubInfo &info, SocketFactory factory, const rix::ipc::Endpoint &rixhub_endpoint);

  using Spinner::spin;
  virtual void spin_once() override;
};

template <typename TMsg> void Subscriber::set_callback(std::function<void(const TMsg &)> callback) {
  static_assert(std::is_base_of<rix::msg::Message, TMsg>::value, "TMsg must be a subclass of rix::msg::Message.");

  if (TMsg().hash() != info_.topic_info.message_hash) {
    rix::util::Log::warn << "Message type mismatch in set_callback." << std::endl;
    return;
  }
  std::lock_guard<std::mutex> guard(callback_mutex_);
  msg_instance_ = std::make_shared<TMsg>();
  callback_ = [callback](const rix::msg::Message &msg) {
    // Safe to static cast because we checked the hash above
    const TMsg &typed_msg = static_cast<const TMsg &>(msg);
    callback(typed_msg);
  };
}

} // namespace rix::core