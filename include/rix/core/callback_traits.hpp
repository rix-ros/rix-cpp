#include <functional>
#include <type_traits>

namespace rix {

// Type deduction helper for extracting message type from callback signature
template <typename T, typename = void> struct SubscriberCallbackTraits;

// Specialization for function pointers
template <typename TMsg> struct SubscriberCallbackTraits<void (*)(const TMsg&), void> {
  using MessageType = TMsg;
};

// Specialization for std::function
template <typename TMsg>
struct SubscriberCallbackTraits<std::function<void(const TMsg&)>, void> {
  using MessageType = TMsg;
};

// Specialization for const member function (lambda/functor)
template <typename Class, typename TMsg>
struct SubscriberCallbackTraits<void (Class::*)(const TMsg&) const, void> {
  using MessageType = TMsg;
};

// Specialization for non-const member function
template <typename Class, typename TMsg>
struct SubscriberCallbackTraits<void (Class::*)(const TMsg&), void> {
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

} // namespace rix