#pragma once

#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/ActInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Action : public Spinner {
public:
  template <typename TGoal, typename TFeedback, typename TResult>
  using Callback = std::function<bool(const TGoal&, TFeedback&, TResult&)>;
  virtual ~Action() = default;

  virtual void set_goal_callback(std::function<void()> callback) = 0;

  virtual void set_preempt_callback(std::function<void()> callback) = 0;

  template <typename TGoal, typename TFeedback, typename TResult>
  void set_callback(Callback<TGoal, TFeedback, TResult> callback);

protected:
  using CallbackUntyped = std::function<bool(const Message&, Message&, Message&)>;

private:
  virtual void set_callback(CallbackUntyped callback,
                            std::shared_ptr<Message> goal_instance,
                            std::shared_ptr<Message> feedback_instance,
                            std::shared_ptr<Message> result_instance) = 0;
};

namespace detail {

class ActionImpl final : public Action {
public:
  /**
   * @brief Callback type definition for action goals.
   * @tparam TGoal The goal message type.
   * @tparam TFeedback The feedback message type.
   * @tparam TResult The result message type.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  using Callback = std::function<bool(const TGoal&, TFeedback&, TResult&)>;

  /**
   * @brief Constructs an ActionImpl with the given ActInfo, socket factory, and RIXHub endpoint.
   * @param info The ActInfo message containing action details.
   * @param socket_factory The socket factory function.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  ActionImpl(const sys_msgs::ActInfo& info, const Endpoint& rixhub_endpoint);

  // Disable copy and move semantics
  ActionImpl(const ActionImpl&) = delete;
  ActionImpl& operator=(const ActionImpl&) = delete;
  ActionImpl(ActionImpl&&) = delete;
  ActionImpl& operator=(ActionImpl&&) = delete;

  /**
   * @brief Destructor. Deregisters the action from rixhub.
   */
  ~ActionImpl() override;

  /**
   * @brief Sets the callback to be invoked when a new goal is received.
   * @param callback The goal callback function.
   */
  void set_goal_callback(std::function<void()> callback) override;

  /**
   * @brief Sets the callback to be invoked when a preempt request is received.
   * @param callback The preempt callback function.
   */
  void set_preempt_callback(std::function<void()> callback) override;

private:
  /**
   * @brief Callback type definition for untyped action goals.
   */
  using CallbackUntyped = std::function<bool(const Message&, Message&, Message&)>;

  CallbackUntyped callback_{};                   ///< Untyped action callback.
  std::function<void()> goal_callback_{};        ///< Goal callback.
  std::function<void()> preempt_callback_{};     ///< Preempt callback.
  sys_msgs::ActInfo info_{};                     ///< ActionImpl information.
  TransportFactory factory_{};                   ///< Socket factory function.
  std::shared_ptr<Acceptor> server_{};           ///< Server socket.
  std::shared_ptr<Stream> connection_{};         ///< Active connection socket.
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
     * @brief Constructs a ActAcceptor with the given parent ActionImpl.
     * @param parent The parent ActionImpl.
     */
    explicit ActAcceptor(ActionImpl& parent);
    ~ActAcceptor() override = default;

    // Disable copy and move semantics
    ActAcceptor(const ActAcceptor&) = delete;
    ActAcceptor& operator=(const ActAcceptor&) = delete;
    ActAcceptor(ActAcceptor&&) = delete;
    ActAcceptor& operator=(ActAcceptor&&) = delete;

    ActionImpl& parent;        ///< Reference to the parent ActionImpl.
    std::thread spin_thread{}; ///< Thread running the spin loop.

  private:
    void on_spin() override;
  };

  ActAcceptor acceptor_{*this};

  // Disable public spin methods (only Node can spin the ActionImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  void set_callback(CallbackUntyped callback,
                    std::shared_ptr<Message> goal_instance,
                    std::shared_ptr<Message> feedback_instance,
                    std::shared_ptr<Message> result_instance) override;

  /**
   * @brief Internal spin implementation for the ActionImpl.
   */
  void on_spin() override;
};

} // namespace detail

template <typename TGoal, typename TFeedback, typename TResult>
void Action::set_callback(Callback<TGoal, TFeedback, TResult> callback) {
  auto goal_instance = std::make_shared<TGoal>();
  auto feedback_instance = std::make_shared<TFeedback>();
  auto result_instance = std::make_shared<TResult>();
  auto untyped = [callback](const Message& goal, Message& feedback, Message& result) -> bool {
    // Safe to static cast because we checked the hash above
    const auto& typed_goal = static_cast<const TGoal&>(goal);
    auto& typed_feedback = static_cast<TFeedback&>(feedback);
    auto& typed_result = static_cast<TResult&>(result);
    return callback(typed_goal, typed_feedback, typed_result);
  };

  set_callback(untyped, goal_instance, feedback_instance, result_instance);
}

} // namespace rix