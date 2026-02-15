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
  /**
   * @brief Callback type definitions for feedback and result handling.
   */
  template <typename TFeedback> using FeedbackCallback = std::function<void(const TFeedback&)>;

  /**
   * @brief Callback type definition for result handling.
   */
  template <typename TResult> using ResultCallback = std::function<void(const TResult&)>;

  // Disable copy and move semantics
  ActionClient(const ActionClient&) = delete;
  ActionClient& operator=(const ActionClient&) = delete;
  ActionClient(ActionClient&&) = delete;
  ActionClient& operator=(ActionClient&&) = delete;

  /**
   * @brief Destructor. Cleans up the ActionClient.
   */
  ~ActionClient() override;

  /**
   * @brief Dispatches a goal to the action server.
   * @param goal The goal message.
   * @return true if the goal was successfully dispatched, false otherwise.
   */
  bool dispatch(const Message& goal);

  /**
   * @brief Cancels the current goal.
   * @return true if the cancel request was successfully sent, false otherwise.
   */
  bool cancel();

  /**
   * @brief Waits for the result to be received or until the timeout expires.
   * @param timeout The maximum duration to wait for the result.
   * @return true if the result was received before the timeout, false otherwise.
   */
  bool wait_for_result(const Duration& timeout);

  /**
   * @brief Sets the callback to be invoked for feedback messages.
   * @tparam TFeedback The feedback message type.
   * @param callback The feedback callback function.
   */
  template <typename TFeedback> void set_feedback_callback(FeedbackCallback<TFeedback> callback);

  /**
   * @brief Sets the callback to be invoked for result messages.
   * @tparam TResult The result message type.
   * @param callback The result callback function.
   */
  template <typename TResult> void set_result_callback(ResultCallback<TResult> callback);

private:
  /**
   * @brief Callback type definition for untyped feedback and result handling.
   */
  using CallbackUntyped = std::function<void(const Message&)>;

  CallbackUntyped feedback_callback_{};          ///< Untyped feedback callback.
  CallbackUntyped result_callback_{};            ///< Untyped result callback.
  sys_msgs::ActRequest request_{};               ///< Action request information.
  TransportFactory socket_factory_{};               ///< Socket factory function.
  std::shared_ptr<Stream> client_{};      ///< Client socket.
  Endpoint endpoint_{};                          ///< Action server endpoint.
  std::shared_ptr<Message> feedback_instance_{}; ///< Prototype feedback message.
  std::shared_ptr<Message> result_instance_{};   ///< Prototype result message.
  mutable std::mutex mutex_{};                   ///< Mutex to protect shared data.
  std::condition_variable result_condition_{};   ///< Condition variable to notify result receipt.
  bool result_received_{false};                  ///< Flag indicating if the result has been received.
  std::thread spin_thread_{};                    ///< Thread for spinning the client.

  // Disable public spin methods (only Node can spin the ActionClient)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the ActionClient.
   */
  void on_spin() override;

  /**
   * @brief Constructs an ActionClient with the given ActRequest, socket factory, and RIXHub endpoint.
   * @param request The ActRequest message containing action client details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  ActionClient(const sys_msgs::ActRequest& request, TransportFactory factory, const Endpoint& rixhub_endpoint);
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