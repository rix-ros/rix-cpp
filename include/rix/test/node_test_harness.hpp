#pragma once

#include "rix/core/node.hpp"
#include "rix/test/core/mock_component_factory.hpp"
#include "rix/test/mock_clock.hpp"
#include "rix/util/time.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace rix {
namespace test {

/**
 * @brief Returned by expect_publisher().
 *        Automatically captures every message the publisher sends.
 */
template <typename TMsg> class PublisherCapture {
public:
  size_t message_count() const { return messages_.size(); }
  const TMsg& message(size_t i) const { return messages_.at(i); }
  const TMsg& last_message() const { return messages_.back(); }
  const std::vector<TMsg>& messages() const { return messages_; }

private:
  friend class NodeTestHarness;
  std::vector<TMsg> messages_;
};

/**
 * @brief Returned by expect_subscriber().
 *        Call inject() to push messages into the subscriber's queue; they are
 *        delivered on the next spin cycle.
 */
template <typename TMsg> class SubscriberInjector {
public:
  void inject(const TMsg& msg) {
    if (auto sub = sub_.lock()) {
      sub->enqueue(std::make_shared<TMsg>(msg));
    }
  }

  void inject(const std::vector<TMsg>& msgs) {
    for (const auto& m : msgs)
      inject(m);
  }

private:
  friend class NodeTestHarness;
  std::weak_ptr<MockSubscriber> sub_;
};

/**
 * @brief Returned by expect_service().
 *        Call call() to exercise the service callback; request and response are
 *        recorded and retrievable afterwards.
 */
template <typename TReq, typename TRes> class ServiceCapture {
public:
  size_t call_count() const { return requests_.size(); }
  const TReq& request(size_t i) const { return requests_.at(i); }
  const TRes& response(size_t i) const { return responses_.at(i); }

  /** Invoke the service callback synchronously and record req + res. */
  bool call(const TReq& req) {
    TRes res{};
    auto srv = srv_.lock();
    if (!srv || !srv->process(req, res))
      return false;
    requests_.push_back(req);
    responses_.push_back(res);
    return true;
  }

private:
  friend class NodeTestHarness;
  std::weak_ptr<MockService> srv_;
  std::vector<TReq> requests_;
  std::vector<TRes> responses_;
};

/**
 * @brief Returned by expect_service_client().
 *        Queue responses the mock will return and inspect captured requests.
 */
template <typename TReq, typename TRes> class ServiceClientCapture {
public:
  /** Pre-load a response to be returned on the next call(). */
  void queue_response(const TRes& res) { queued_.push_back(res); }

  size_t call_count() const { return requests_.size(); }
  const TReq& request(size_t i) const { return requests_.at(i); }
  const TRes& response(size_t i) const { return responses_.at(i); }

private:
  friend class NodeTestHarness;
  std::deque<TRes> queued_;
  std::vector<TReq> requests_;
  std::vector<TRes> responses_;
};

/**
 * @brief Returned by expect_action().
 *        Call call() to exercise the action callback; goal, feedback, and
 *        result are all recorded.
 */
template <typename TGoal, typename TFeedback, typename TResult> class ActionCapture {
public:
  size_t call_count() const { return feedbacks_.size() + results_.size(); }
  const TGoal& goal(size_t i) const { return goals_.at(i); }
  const TFeedback& feedback(size_t i) const { return feedbacks_.at(i); }
  const TResult& result(size_t i) const { return results_.at(i); }

  void clear() {
    feedbacks_.clear();
    results_.clear();
  }

  bool set_goal(const TGoal& goal) {
    auto act = act_.lock();
    if (!act)
      return false;

    if (processing_) {
      act->trigger_preempt();
    } else {
      act->trigger_goal();
    }
    current_goal_ = goal;
    processing_ = true;
    goals_.push_back(goal);
    return true;
  }

  /** Invoke the action callback synchronously and record goal/feedback/result. */
  bool call() {
    TFeedback fb{};
    TResult res{};
    auto act = act_.lock();
    if (!act)
      return false;
    bool done = act->process(current_goal_, fb, res);
    if (done) {
      processing_ = false;
      results_.push_back(res);
    } else {
      feedbacks_.push_back(fb);
    }
    return done;
  }

private:
  friend class NodeTestHarness;
  std::weak_ptr<MockAction> act_;
  TGoal current_goal_;
  std::vector<TGoal> goals_;
  std::vector<TFeedback> feedbacks_;
  std::vector<TResult> results_;
  bool processing_;
};

/**
 * @brief Returned by expect_action_client().
 *        Queue feedback/results to inject and inspect dispatched goals.
 */
template <typename TGoal, typename TFeedback, typename TResult> class ActionClientCapture {
public:
  size_t dispatch_count() const { return goals_.size(); }
  const TGoal& goal(size_t i) const { return goals_.at(i); }

  /** Inject a feedback message into the action client's feedback callback. */
  void inject_feedback(const TFeedback& fb) {
    if (auto cli = cli_.lock()) {
      cli->inject_feedback(std::make_shared<TFeedback>(fb));
    }
  }

  /** Inject a result message into the action client's result callback. */
  void inject_result(const TResult& res) {
    if (auto cli = cli_.lock()) {
      cli->inject_result(std::make_shared<TResult>(res));
    }
  }

private:
  friend class NodeTestHarness;
  std::weak_ptr<MockActionClient> cli_;
  std::vector<TGoal> goals_;
};

// ============================================================================
// NodeTestHarness
// ============================================================================

/**
 * @brief High-level test harness for testing custom RIX Node subclasses.
 *
 * Designed for educators building autograding pipelines.  The harness uses
 * lightweight mock objects (MockPublisher, MockSubscriber, …) instead of
 * real IPC infrastructure — no sockets, no network, no gmock expectations.
 *
 * Typical usage:
 * @code
 *   rix::test::NodeTestHarness harness;
 *
 *   // Phase 1 — declare what the node is expected to create
 *   auto& pub = harness.expect_publisher<std_msgs::Header>("/chatter");
 *
 *   // Phase 2 — construct the node
 *   auto& node = harness.create<SimplePublisher<Node>>(1.0);
 *   EXPECT_TRUE(node.ok());
 *
 *   // Phase 3 — drive execution
 *   harness.advance_time(1.0);
 *   harness.spin(1);
 *
 *   // Phase 4 — assert
 *   EXPECT_EQ(pub.message_count(), 1u);
 * @endcode
 */
class NodeTestHarness {
public:
  NodeTestHarness() : factory_(std::make_shared<MockComponentFactoryImpl>()) {
    if (MULTITHREADED) {
      throw std::runtime_error(
          "NodeTestHarness does not support multithreaded tests. Run 'export RIX_MULTITHREADED=0' and try again.");
    }
    clock_ = std::make_shared<MockClock>();
    rix::Time::set_clock(clock_);
    Node::set_component_factory(factory_);
  }

  ~NodeTestHarness() {
    node_.reset();
    MockComponentFactoryImpl::reset_fail();
    rix::Time::set_clock(std::make_shared<rix::Clock>());
    Node::set_component_factory(std::make_shared<detail::ComponentFactoryImpl>());
  }

  /**
   * @brief Declare that the node will create a publisher on @p topic.
   *
   * @param topic            The topic name.
   * @param subscriber_count Reported by get_subscriber_count() (informational).
   * @return A capture handle; inspect messages after spinning.
   */
  template <typename TMsg>
  PublisherCapture<TMsg>& expect_publisher(const std::string& topic, size_t subscriber_count = 1) {
    auto cap = std::make_shared<PublisherCapture<TMsg>>();
    capture_handles_.push_back(cap);

    wire_fns_.push_back([factory = factory_, cap, topic, subscriber_count](Node* mock) {
      auto pub = factory->get_publisher(topic);
      if (!pub)
        return;
      pub->set_subscriber_count(subscriber_count);
      pub->add_observer([cap](const Message& msg) { cap->messages_.push_back(static_cast<const TMsg&>(msg)); });
    });

    return *cap;
  }

  /**
   * @brief Declare that the node will create a subscriber on @p topic.
   *
   * @return An injector handle; call inject() before/during spin to push messages.
   */
  template <typename TMsg> SubscriberInjector<TMsg>& expect_subscriber(const std::string& topic) {
    auto inj = std::make_shared<SubscriberInjector<TMsg>>();
    capture_handles_.push_back(inj);

    wire_fns_.push_back([factory = factory_, inj, topic](Node* mock) {
      if (auto sub = factory->get_subscriber(topic)) {
        inj->sub_ = sub;
      }
    });

    return *inj;
  }

  /**
   * @brief Declare that the node will create a service named @p name.
   *
   * @return A capture handle; call capture.call(req) to invoke the service callback.
   */
  template <typename TReq, typename TRes> ServiceCapture<TReq, TRes>& expect_service(const std::string& name) {
    auto cap = std::make_shared<ServiceCapture<TReq, TRes>>();
    capture_handles_.push_back(cap);

    wire_fns_.push_back([factory = factory_, cap, name](Node* mock) {
      if (auto srv = factory->get_service(name)) {
        cap->srv_ = srv;
      }
    });

    return *cap;
  }

  /**
   * @brief Declare that the node will create a service client for @p name.
   *
   * @return A capture handle; use queue_response() to pre-load responses and
   *         inspect captured requests after spinning.
   */
  template <typename TReq, typename TRes>
  ServiceClientCapture<TReq, TRes>& expect_service_client(const std::string& name) {
    auto cap = std::make_shared<ServiceClientCapture<TReq, TRes>>();
    capture_handles_.push_back(cap);

    wire_fns_.push_back([factory = factory_, cap, name](Node* mock) {
      auto cli = factory->get_service_client(name);
      if (!cli)
        return;
      cli->set_call_handler([cap](const Message& req, Message& res) -> bool {
        cap->requests_.push_back(static_cast<const TReq&>(req));
        auto& typed_res = static_cast<TRes&>(res);
        if (!cap->queued_.empty()) {
          typed_res = cap->queued_.front();
          cap->queued_.pop_front();
          cap->responses_.push_back(typed_res);
          return true;
        }
        return false;
      });
    });

    return *cap;
  }

  /**
   * @brief Declare that the node will create an action server named @p name.
   *
   * @return A capture handle; call capture.call(goal) to invoke the action callback.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  ActionCapture<TGoal, TFeedback, TResult>& expect_action(const std::string& name) {
    auto cap = std::make_shared<ActionCapture<TGoal, TFeedback, TResult>>();
    capture_handles_.push_back(cap);

    wire_fns_.push_back([factory = factory_, cap, name](Node* mock) {
      if (auto act = factory->get_action(name)) {
        cap->act_ = act;
      }
    });

    return *cap;
  }

  /**
   * @brief Declare that the node will create an action client for @p name.
   *
   * @return A capture handle; use inject_feedback() / inject_result() to
   *         simulate server responses and inspect dispatched goals.
   */
  template <typename TGoal, typename TFeedback, typename TResult>
  ActionClientCapture<TGoal, TFeedback, TResult>& expect_action_client(const std::string& name) {
    auto cap = std::make_shared<ActionClientCapture<TGoal, TFeedback, TResult>>();
    capture_handles_.push_back(cap);

    wire_fns_.push_back([factory = factory_, cap, name](Node* mock) {
      auto cli = factory->get_action_client(name);
      if (!cli)
        return;
      // Capture goals dispatched by the student node
      cli->set_dispatch_handler([cap](const Message& goal) -> bool {
        cap->goals_.push_back(static_cast<const TGoal&>(goal));
        return true;
      });
      // Weak ref so tests can inject feedback/results
      cap->cli_ = cli;
    });

    return *cap;
  }

  void preset_parameter(const std::string& name, const Message& parameter) {
    factory_->preset_parameter(name, parameter);
  }

  /**
   * @brief Make the next Node constructor call shutdown(), so ok() == false.
   *
   * Call this before create() to test how the student node handles init failure.
   */
  NodeTestHarness& node_registration_fails() {
    MockComponentFactoryImpl::set_should_fail();
    return *this;
  }

  /**
   * @brief Construct the node-under-test and wire up all declared handles.
   *
   * @tparam TNode  Must derive from Node (and thus NodeBase).
   * @param args    Forwarded to TNode's constructor.
   * @return Reference to the constructed node.
   */
  template <typename TNode, typename... Args> TNode& create(Args&&... args) {
    static_assert(std::is_base_of_v<Node, TNode>, "TNode must derive from rix::NodeBase (use Node as base)");

    auto node = std::make_unique<TNode>(std::forward<Args>(args)...);
    auto* ptr = node.get();
    node_ = std::move(node);

    // Wire captures / injectors to the mock objects created during construction
    if (auto* mock = dynamic_cast<Node*>(node_.get())) {
      for (auto& fn : wire_fns_) {
        fn(mock);
      }
    }

    return *ptr;
  }

  /**
   * @brief Spin the node @p n times, processing one cycle per call.
   */
  void spin(int n = 1) {
    for (int i = 0; i < n; ++i) {
      node_->spin_once();
    }
  }

  /**
   * @brief Advance the mock clock by @p seconds.
   *
   * This is the primary mechanism for triggering timer callbacks: call
   * advance_time(duration) then spin(1).
   */
  void advance_time(double seconds) { clock_->sleep_for(rix::Duration(seconds)); }

  /** Direct access to the mock clock (for fine-grained time control). */
  std::shared_ptr<MockClock> clock() { return clock_; }

  /** True if the node exists and ok() returns true. */
  bool node_ok() const { return node_ && node_->ok(); }

private:
  std::shared_ptr<MockClock> clock_;
  std::unique_ptr<Node> node_;
  std::shared_ptr<MockComponentFactoryImpl> factory_;

  // Wire-up closures executed after create()
  std::vector<std::function<void(Node*)>> wire_fns_;

  // Heterogeneous ownership of all capture/injector handles
  std::vector<std::shared_ptr<void>> capture_handles_;
};

} // namespace test
} // namespace rix
