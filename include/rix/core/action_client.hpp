#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/ActRequest.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class ActionClient final : public Spinner {
  friend class Node;

public:
  template <typename TFeedback> using FeedbackCallback = std::function<void(const TFeedback&)>;
  template <typename TResult> using ResultCallback = std::function<void(const TResult&)>;

  ActionClient(const ActionClient&) = delete;
  ActionClient& operator=(const ActionClient&) = delete;
  ActionClient(ActionClient&&) = delete;
  ActionClient& operator=(ActionClient&&) = delete;

  ~ActionClient() override;

  bool dispatch(const Message& goal);
  bool cancel();

  /**
   * @brief Wait for the result of the action. Returns true if the most recent result has
   * been received.
   *
   * @param timeout Duration to wait for the result
   */
  bool wait_for_result(const Duration& timeout);

  template <typename TFeedback> void set_feedback_callback(FeedbackCallback<TFeedback> callback);
  template <typename TResult> void set_result_callback(ResultCallback<TResult> callback);

private:
  using CallbackUntyped = std::function<void(const Message&)>;
  CallbackUntyped feedback_callback_{};
  CallbackUntyped result_callback_{};
  sys_msgs::ActRequest request_{};
  SocketFactory socket_factory_{};
  std::shared_ptr<GenericSocket> client_{};
  Endpoint endpoint_{};
  std::shared_ptr<Message> feedback_instance_{};
  std::shared_ptr<Message> result_instance_{};
  mutable std::mutex mutex_{};
  std::condition_variable result_condition_{};
  bool result_received_{false};
  std::thread spin_thread_{};

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;

  ActionClient(const sys_msgs::ActRequest& request, SocketFactory factory, const Endpoint& rixhub_endpoint);
};

template <typename TFeedback> void ActionClient::set_feedback_callback(FeedbackCallback<TFeedback> callback) {
  static_assert(std::is_base_of_v<Message, TFeedback>, "TFeedback must be a subclass of Message.");
  std::lock_guard<std::mutex> guard(mutex_);
  if (TFeedback().hash() != request_.feedback_hash) {
    Log::warn << "Message type mismatch in ActionClient::set_feedback_callback." << std::endl;
    return;
  }
  feedback_instance_ = std::make_shared<TFeedback>();
  feedback_callback_ = [callback](const Message& msg) {
    // Safe to static cast because we checked the hash above
    const auto& typed_msg = static_cast<const TFeedback&>(msg);
    callback(typed_msg);
  };
}

template <typename TResult> void ActionClient::set_result_callback(ResultCallback<TResult> callback) {
  static_assert(std::is_base_of_v<Message, TResult>, "TResult must be a subclass of Message.");
  std::lock_guard<std::mutex> guard(mutex_);
  if (TResult().hash() != request_.result_hash) {
    Log::warn << "Message type mismatch in ActionClient::set_result_callback." << std::endl;
    return;
  }
  result_instance_ = std::make_shared<TResult>();
  result_callback_ = [callback](const Message& msg) {
    // Safe to static cast because we checked the hash above
    const auto& typed_msg = static_cast<const TResult&>(msg);
    callback(typed_msg);
  };
}

} // namespace rix