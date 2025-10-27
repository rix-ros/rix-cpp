#pragma once

#include <memory>
#include <mutex>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/msg/mediator/ActInfo.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class Action final : public Spinner {
  friend class Node;

public:
  template <typename TGoal, typename TFeedback, typename TResult>
  using Callback = std::function<bool(const TGoal&, TFeedback&, TResult&)>;

  Action(const Action&) = delete;
  Action& operator=(const Action&) = delete;
  Action(Action&&) = delete;
  Action& operator=(Action&&) = delete;
  ~Action() override;

  void set_goal_callback(std::function<void()> callback);
  void set_preempt_callback(std::function<void()> callback);

  template <typename TGoal, typename TFeedback, typename TResult>
  void set_callback(Callback<TGoal, TFeedback, TResult> callback);

private:
  using CallbackUntyped = std::function<bool(const msg::Message&, msg::Message&, msg::Message&)>;
  CallbackUntyped callback_{};
  std::function<void()> goal_callback_{};
  std::function<void()> preempt_callback_{};
  msg::mediator::ActInfo info_{};
  SocketFactory socket_factory_{};
  std::shared_ptr<GenericSocket> server_{};
  std::shared_ptr<GenericSocket> connection_{};
  mutable std::mutex mutex_{};
  Endpoint rixhub_endpoint_{};
  std::atomic<bool> registered_flag_{};
  std::shared_ptr<msg::Message> goal_instance_{};
  std::shared_ptr<msg::Message> feedback_instance_{};
  std::shared_ptr<msg::Message> result_instance_{};

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_{};
#endif

  // Internal class to handle accepting new connections from rixhub
  class ActAcceptor : public Spinner {
  public:
    explicit ActAcceptor(Action& parent);
    ~ActAcceptor() override = default;

    ActAcceptor(const ActAcceptor&) = delete;
    ActAcceptor& operator=(const ActAcceptor&) = delete;
    ActAcceptor(ActAcceptor&&) = delete;
    ActAcceptor& operator=(ActAcceptor&&) = delete;

    void on_spin() override;

    Action& parent;
#ifdef RIX_MULTITHREADED
    std::thread spin_thread{};
#endif
  };

  ActAcceptor acceptor_{*this};

  Action(const msg::mediator::ActInfo& info, SocketFactory socket_factory, const Endpoint& rixhub_endpoint);

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;
};

template <typename TGoal, typename TFeedback, typename TResult>
void Action::set_callback(Callback<TGoal, TFeedback, TResult> callback) {
  static_assert(std::is_base_of_v<msg::Message, TGoal>, "TGoal must be a subclass of msg::Message.");
  static_assert(std::is_base_of_v<msg::Message, TFeedback>, "TFeedback must be a subclass of msg::Message.");
  static_assert(std::is_base_of_v<msg::Message, TResult>, "TResult must be a subclass of msg::Message.");

  std::lock_guard<std::mutex> guard(mutex_);
  if (TGoal().hash() != info_.goal_hash || TFeedback().hash() != info_.feedback_hash ||
      TResult().hash() != info_.result_hash) {
    Log::warn << "Message type mismatch in Action::set_callback." << std::endl;
    return;
  }
  goal_instance_ = std::make_shared<TGoal>();
  feedback_instance_ = std::make_shared<TFeedback>();
  result_instance_ = std::make_shared<TResult>();
  callback_ = [callback](const msg::Message& goal, msg::Message& feedback, msg::Message& result) -> bool {
    // Safe to static cast because we checked the hash above
    const auto& typed_goal = static_cast<const TGoal&>(goal);
    auto& typed_feedback = static_cast<TFeedback&>(feedback);
    auto& typed_result = static_cast<TResult&>(result);
    return callback(typed_goal, typed_feedback, typed_result);
  };
}

} // namespace rix