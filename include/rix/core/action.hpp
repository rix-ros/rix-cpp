#pragma once

#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/ActInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class Action final : public Spinner {
  friend class Node;

public:
  /**
   * @brief Callback type definition for action goals.
   * @tparam TGoal The goal message type.
   * @tparam TFeedback The feedback message type.
   * @tparam TResult The result message type.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  using Callback = std::function<bool(const TGoal&, TFeedback&, TResult&)>;

  // Disable copy and move semantics
  Action(const Action&) = delete;
  Action& operator=(const Action&) = delete;
  Action(Action&&) = delete;
  Action& operator=(Action&&) = delete;

  /**
   * @brief Destructor. Deregisters the action from rixhub.
   */
  ~Action() override;

  /**
   * @brief Sets the callback to be invoked when a new goal is received.
   * @param callback The goal callback function.
   */
  void set_goal_callback(std::function<void()> callback);

  /**
   * @brief Sets the callback to be invoked when a preempt request is received.
   * @param callback The preempt callback function.
   */
  void set_preempt_callback(std::function<void()> callback);

  /**
   * @brief Sets the callback to be invoked for action goals.
   * @tparam TGoal The goal message type.
   * @tparam TFeedback The feedback message type.
   * @tparam TResult The result message type.
   * @param callback The callback function.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  void set_callback(Callback<TGoal, TFeedback, TResult> callback);

private:
  /**
   * @brief Callback type definition for untyped action goals.
   */
  using CallbackUntyped = std::function<bool(const Message&, Message&, Message&)>;

  CallbackUntyped callback_{};                   ///< Untyped action callback.
  std::function<void()> goal_callback_{};        ///< Goal callback.
  std::function<void()> preempt_callback_{};     ///< Preempt callback.
  sys_msgs::ActInfo info_{};                     ///< Action information.
  SocketFactory socket_factory_{};               ///< Socket factory function.
  std::shared_ptr<GenericSocket> server_{};      ///< Server socket.
  std::shared_ptr<GenericSocket> connection_{};  ///< Active connection socket.
  mutable std::mutex mutex_{};                   ///< Mutex for protecting shared data.
  Endpoint rixhub_endpoint_{};                   ///< RIXHub endpoint
  std::atomic<bool> registered_flag_{};          ///< Flag indicating if the action is registered.
  std::shared_ptr<Message> goal_instance_{};     ///< Prototype goal message.
  std::shared_ptr<Message> feedback_instance_{}; ///< Prototype feedback message.
  std::shared_ptr<Message> result_instance_{};   ///< Prototype result message.
  std::thread spin_thread_{};                    ///< Thread running the spin loop.

  /**
   * @brief Internal class to accept new action goal notifications.
   */
  class ActAcceptor : public Spinner {
  public:
    /**
     * @brief Constructs a ActAcceptor with the given parent Action.
     * @param parent The parent Action.
     */
    explicit ActAcceptor(Action& parent);
    ~ActAcceptor() override = default;

    // Disable copy and move semantics
    ActAcceptor(const ActAcceptor&) = delete;
    ActAcceptor& operator=(const ActAcceptor&) = delete;
    ActAcceptor(ActAcceptor&&) = delete;
    ActAcceptor& operator=(ActAcceptor&&) = delete;

    Action& parent;            ///< Reference to the parent Action.
    std::thread spin_thread{}; ///< Thread running the spin loop.

  private:
    void on_spin() override;
  };

  ActAcceptor acceptor_{*this};

  /**
   * @brief Constructs an Action with the given ActInfo, socket factory, and RIXHub endpoint.
   * @param info The ActInfo message containing action details.
   * @param socket_factory The socket factory function.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  Action(const sys_msgs::ActInfo& info, SocketFactory socket_factory, const Endpoint& rixhub_endpoint);

  // Disable public spin methods (only Node can spin the Action)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the Action.
   */
  void on_spin() override;
};

template <typename TGoal, typename TFeedback, typename TResult>
void Action::set_callback(Callback<TGoal, TFeedback, TResult> callback) {
  static_assert(std::is_base_of_v<Message, TGoal>, "TGoal must be a subclass of Message.");
  static_assert(std::is_base_of_v<Message, TFeedback>, "TFeedback must be a subclass of Message.");
  static_assert(std::is_base_of_v<Message, TResult>, "TResult must be a subclass of Message.");

  std::lock_guard<std::mutex> guard(mutex_);
  if (TGoal().hash() != info_.goal_hash || TFeedback().hash() != info_.feedback_hash ||
      TResult().hash() != info_.result_hash) {
    Log::warn << "Message type mismatch in Action::set_callback." << std::endl;
    return;
  }
  goal_instance_ = std::make_shared<TGoal>();
  feedback_instance_ = std::make_shared<TFeedback>();
  result_instance_ = std::make_shared<TResult>();
  callback_ = [callback](const Message& goal, Message& feedback, Message& result) -> bool {
    // Safe to static cast because we checked the hash above
    const auto& typed_goal = static_cast<const TGoal&>(goal);
    auto& typed_feedback = static_cast<TFeedback&>(feedback);
    auto& typed_result = static_cast<TResult&>(result);
    return callback(typed_goal, typed_feedback, typed_result);
  };
}

} // namespace rix