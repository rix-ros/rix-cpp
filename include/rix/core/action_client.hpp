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

class ActionClient : public Spinner {
public:
  template <typename TFeedback> using FeedbackCallback = std::function<void(const TFeedback&)>;
  template <typename TResult> using ResultCallback = std::function<void(const TResult&)>;

  virtual ~ActionClient() = default;

  virtual bool dispatch(const Message& goal) = 0;
  virtual bool cancel() = 0;
  virtual bool wait_for_result(const Duration& timeout) = 0;

  template <typename TFeedback> void set_feedback_callback(FeedbackCallback<TFeedback> callback);
  template <typename TResult> void set_result_callback(ResultCallback<TResult> callback);

protected:
  using CallbackUntyped = std::function<void(const Message&)>;

private:
  virtual void set_feedback_callback(CallbackUntyped callback, std::shared_ptr<Message> feedback_instance) = 0;
  virtual void set_result_callback(CallbackUntyped callback, std::shared_ptr<Message> result_instance) = 0;
};

namespace detail {

class ActionClientImpl final : public ActionClient {
public:
  /**
   * @brief Constructs an ActionClientImpl with the given ActRequest, socket factory, and RIXHub endpoint.
   * @param request The ActRequest message containing action client details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  ActionClientImpl(const sys_msgs::ActRequest& request, const Endpoint& rixhub_endpoint);

  // Disable copy and move semantics
  ActionClientImpl(const ActionClientImpl&) = delete;
  ActionClientImpl& operator=(const ActionClientImpl&) = delete;
  ActionClientImpl(ActionClientImpl&&) = delete;
  ActionClientImpl& operator=(ActionClientImpl&&) = delete;

  /**
   * @brief Destructor. Cleans up the ActionClientImpl.
   */
  ~ActionClientImpl() override;

  /**
   * @brief Dispatches a goal to the action server.
   * @param goal The goal message.
   * @return true if the goal was successfully dispatched, false otherwise.
   */
  bool dispatch(const Message& goal) override;

  /**
   * @brief Cancels the current goal.
   * @return true if the cancel request was successfully sent, false otherwise.
   */
  bool cancel() override;

  /**
   * @brief Waits for the result to be received or until the timeout expires.
   * @param timeout The maximum duration to wait for the result.
   * @return true if the result was received before the timeout, false otherwise.
   */
  bool wait_for_result(const Duration& timeout) override;

private:
  CallbackUntyped feedback_callback_{};          ///< Untyped feedback callback.
  CallbackUntyped result_callback_{};            ///< Untyped result callback.
  sys_msgs::ActRequest request_{};               ///< Action request information.
  TransportFactory factory_{};                   ///< Socket factory function.
  std::shared_ptr<Stream> client_{};             ///< Client socket.
  Endpoint endpoint_{};                          ///< Action server endpoint.
  std::shared_ptr<Message> feedback_instance_{}; ///< Prototype feedback message.
  std::shared_ptr<Message> result_instance_{};   ///< Prototype result message.
  mutable std::mutex mutex_{};                   ///< Mutex to protect shared data.
  std::condition_variable result_condition_{};   ///< Condition variable to notify result receipt.
  bool result_received_{false};                  ///< Flag indicating if the result has been received.
  std::thread spin_thread_{};                    ///< Thread for spinning the client.

  // Disable public spin methods (only Node can spin the ActionClientImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  void set_feedback_callback(CallbackUntyped callback, std::shared_ptr<Message> feedback_instance) override;
  void set_result_callback(CallbackUntyped callback, std::shared_ptr<Message> result_instance) override;

  /**
   * @brief Internal spin implementation for the ActionClientImpl.
   */
  void on_spin() override;
};

} // namespace detail

template <typename TFeedback> void ActionClient::set_feedback_callback(FeedbackCallback<TFeedback> callback) {
  static_assert(std::is_base_of_v<Message, TFeedback>, "TFeedback must be a subclass of Message.");
  auto feedback_instance = std::make_shared<TFeedback>();
  auto untyped = [callback](const Message& msg) {
    // Safe to static cast because we checked the hash above
    const auto& typed_msg = static_cast<const TFeedback&>(msg);
    callback(typed_msg);
  };
  set_feedback_callback(untyped, feedback_instance);
}

template <typename TResult> void ActionClient::set_result_callback(ResultCallback<TResult> callback) {
  static_assert(std::is_base_of_v<Message, TResult>, "TResult must be a subclass of Message.");
  auto result_instance = std::make_shared<TResult>();
  auto untyped = [callback](const Message& msg) {
    // Safe to static cast because we checked the hash above
    const auto& typed_msg = static_cast<const TResult&>(msg);
    callback(typed_msg);
  };
  set_result_callback(untyped, result_instance);
}

} // namespace rix