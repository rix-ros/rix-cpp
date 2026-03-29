#pragma once

#include <functional>
#include <type_traits>

// TODO: Seek an alternative to this file. The code here is confusing.

namespace rix {

// Type deduction helper for extracting message type from callback signature
template <typename T, typename = void> struct SubscriberCallbackTraits;

// Specialization for function pointers
template <typename TMsg> struct SubscriberCallbackTraits<void (*)(const TMsg&), void> {
  using MessageType = TMsg;
};

// Specialization for std::function
template <typename TMsg> struct SubscriberCallbackTraits<std::function<void(const TMsg&)>, void> {
  using MessageType = TMsg;
};

// Specialization for const member function (lambda/functor)
template <typename Class, typename TMsg> struct SubscriberCallbackTraits<void (Class::*)(const TMsg&) const, void> {
  using MessageType = TMsg;
};

// Specialization for non-const member function
template <typename Class, typename TMsg> struct SubscriberCallbackTraits<void (Class::*)(const TMsg&), void> {
  using MessageType = TMsg;
};

// Specialization for lambdas and functors - only if they have operator()
template <typename Functor>
struct SubscriberCallbackTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : SubscriberCallbackTraits<decltype(&Functor::operator())> {};

// Type deduction helper for service callbacks
template <typename T, typename = void> struct ServiceCallbackTraits;

// Specialization for function pointers
template <typename TRequest, typename TResponse>
struct ServiceCallbackTraits<void (*)(const TRequest&, TResponse&), void> {
  using RequestType = TRequest;
  using ResponseType = TResponse;
};

// Specialization for std::function
template <typename TRequest, typename TResponse>
struct ServiceCallbackTraits<std::function<void(const TRequest&, TResponse&)>, void> {
  using RequestType = TRequest;
  using ResponseType = TResponse;
};

// Specialization for const member function (lambda/functor)
template <typename Class, typename TRequest, typename TResponse>
struct ServiceCallbackTraits<void (Class::*)(const TRequest&, TResponse&) const, void> {
  using RequestType = TRequest;
  using ResponseType = TResponse;
};

// Specialization for non-const member function
template <typename Class, typename TRequest, typename TResponse>
struct ServiceCallbackTraits<void (Class::*)(const TRequest&, TResponse&), void> {
  using RequestType = TRequest;
  using ResponseType = TResponse;
};

// Specialization for lambdas and functors - only if they have operator()
template <typename Functor>
struct ServiceCallbackTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : ServiceCallbackTraits<decltype(&Functor::operator())> {};

// Type deduction helper for service callbacks
template <typename T, typename = void> struct ActionCallbackTraits;

// Specialization for function pointers
template <typename TGoal, typename TFeedback, typename TResult>
struct ActionCallbackTraits<bool (*)(const TGoal&, TFeedback&, TResult&), void> {
  using GoalType = TGoal;
  using FeedbackType = TFeedback;
  using ResultType = TResult;
};

// Specialization for std::function
template <typename TGoal, typename TFeedback, typename TResult>
struct ActionCallbackTraits<std::function<bool(const TGoal&, TFeedback&, TResult&)>, void> {
  using GoalType = TGoal;
  using FeedbackType = TFeedback;
  using ResultType = TResult;
};

// Specialization for const member function (lambda/functor)
template <typename Class, typename TGoal, typename TFeedback, typename TResult>
struct ActionCallbackTraits<bool (Class::*)(const TGoal&, TFeedback&, TResult&) const, void> {
  using GoalType = TGoal;
  using FeedbackType = TFeedback;
  using ResultType = TResult;
};

// Specialization for non-const member function
template <typename Class, typename TGoal, typename TFeedback, typename TResult>
struct ActionCallbackTraits<bool (Class::*)(const TGoal&, TFeedback&, TResult&), void> {
  using GoalType = TGoal;
  using FeedbackType = TFeedback;
  using ResultType = TResult;
};

// Specialization for lambdas and functors - only if they have operator()
template <typename Functor>
struct ActionCallbackTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : ActionCallbackTraits<decltype(&Functor::operator())> {};

// Action Client Callback Traits (for feedback callbacks)
template <typename T, typename = void> struct ActionClientCallbackTraits;

// Specialization for function pointers
template <typename TFeedback> struct ActionClientCallbackTraits<void (*)(const TFeedback&), void> {
  using FeedbackType = TFeedback;
  using ResultType = void;
  using GoalType = void;
};

// Specialization for std::function
template <typename TFeedback> struct ActionClientCallbackTraits<std::function<void(const TFeedback&)>, void> {
  using FeedbackType = TFeedback;
  using ResultType = void;
  using GoalType = void;
};

// Specialization for const member function (lambda/functor)
template <typename Class, typename TFeedback>
struct ActionClientCallbackTraits<void (Class::*)(const TFeedback&) const, void> {
  using FeedbackType = TFeedback;
  using ResultType = void;
  using GoalType = void;
};

// Specialization for non-const member function
template <typename Class, typename TFeedback>
struct ActionClientCallbackTraits<void (Class::*)(const TFeedback&), void> {
  using FeedbackType = TFeedback;
  using ResultType = void;
  using GoalType = void;
};

// Specialization for lambdas and functors - only if they have operator()
template <typename Functor>
struct ActionClientCallbackTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : ActionClientCallbackTraits<decltype(&Functor::operator())> {};

// Action Client Result Callback Traits
template <typename T, typename = void> struct ActionClientResultCallbackTraits;

// Specialization for function pointers
template <typename TResult> struct ActionClientResultCallbackTraits<void (*)(const TResult&), void> {
  using ResultType = TResult;
  using FeedbackType = void;
  using GoalType = void;
};

// Specialization for std::function
template <typename TResult> struct ActionClientResultCallbackTraits<std::function<void(const TResult&)>, void> {
  using ResultType = TResult;
  using FeedbackType = void;
  using GoalType = void;
};

// Specialization for const member function (lambda/functor)
template <typename Class, typename TResult>
struct ActionClientResultCallbackTraits<void (Class::*)(const TResult&) const, void> {
  using ResultType = TResult;
  using FeedbackType = void;
  using GoalType = void;
};

// Specialization for non-const member function
template <typename Class, typename TResult>
struct ActionClientResultCallbackTraits<void (Class::*)(const TResult&), void> {
  using ResultType = TResult;
  using FeedbackType = void;
  using GoalType = void;
};

// Specialization for lambdas and functors - only if they have operator()
template <typename Functor>
struct ActionClientResultCallbackTraits<Functor, std::void_t<decltype(&Functor::operator())>>
    : ActionClientResultCallbackTraits<decltype(&Functor::operator())> {};

} // namespace rix