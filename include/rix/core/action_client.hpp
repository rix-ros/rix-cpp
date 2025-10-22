#pragma once

#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/msg/mediator/ActRequest.hpp"
#include "rix/msg/mediator/ActResponse.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/msg/standard/Void.hpp"
#include "rix/msg/standard/UInt8Array.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class ActionClient : public Spinner {
  friend class Node;

public:
  template <typename TFeedback> using FeedbackCallback = std::function<void(const TFeedback&)>;
  template <typename TResult> using ResultCallback = std::function<void(const TResult&)>;

  ActionClient(const ActionClient&) = delete;
  ActionClient& operator=(const ActionClient&) = delete;
  ActionClient(ActionClient&&) = delete;
  ActionClient& operator=(ActionClient&&) = delete;

  ~ActionClient();

  bool dispatch(const msg::Message& goal);
  bool cancel();

  /**
   * @brief Wait for the result of the action. Returns true if the most recent result has
   * been received.
   *
   * @param d Duration to wait for the result
   */
  bool wait_for_result(const Duration& d);

  template <typename TFeedback> void set_feedback_callback(FeedbackCallback<TFeedback> callback);
  template <typename TResult> void set_result_callback(ResultCallback<TResult> callback);

private:
  using CallbackUntyped = std::function<void(const msg::Message&)>;
  CallbackUntyped feedback_callback_{};
  CallbackUntyped result_callback_{};
  msg::mediator::ActRequest request_{};
  SocketFactory socket_factory_{};
  std::shared_ptr<GenericSocket> client_{};
  Endpoint endpoint_{};
  std::shared_ptr<msg::Message> feedback_instance_{};
  std::shared_ptr<msg::Message> result_instance_{};
  mutable std::mutex mutex_{};
  std::condition_variable result_condition_{};
  bool result_received_{false};

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_{};
#endif

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;

  ActionClient(const msg::mediator::ActRequest& request, SocketFactory factory, const Endpoint& rixhub_endpoint);
};

template <typename TFeedback> void ActionClient::set_feedback_callback(FeedbackCallback<TFeedback> callback) {
  static_assert(std::is_base_of<msg::Message, TFeedback>::value, "TFeedback must be a subclass of msg::Message.");
  std::lock_guard<std::mutex> guard(mutex_);
  if (TFeedback().hash() != request_.feedback_hash) {
    Log::warn << "Message type mismatch in ActionClient::set_feedback_callback." << std::endl;
    return;
  }
  feedback_instance_ = std::make_shared<TFeedback>();
  feedback_callback_ = [callback](const msg::Message& msg) {
    // Safe to static cast because we checked the hash above
    const TFeedback& typed_msg = static_cast<const TFeedback&>(msg);
    callback(typed_msg);
  };
}

template <typename TResult> void ActionClient::set_result_callback(ResultCallback<TResult> callback) {
  static_assert(std::is_base_of<msg::Message, TResult>::value, "TResult must be a subclass of msg::Message.");
  std::lock_guard<std::mutex> guard(mutex_);
  if (TResult().hash() != request_.result_hash) {
    Log::warn << "Message type mismatch in ActionClient::set_result_callback." << std::endl;
    return;
  }
  result_instance_ = std::make_shared<TResult>();
  result_callback_ = [callback](const msg::Message& msg) {
    // Safe to static cast because we checked the hash above
    const TResult& typed_msg = static_cast<const TResult&>(msg);
    callback(typed_msg);
  };
}

} // namespace rix