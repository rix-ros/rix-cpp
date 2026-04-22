#pragma once

#include "rix/ipc/acceptor.hpp"
#include "rix/ipc/shm_acceptor.hpp"
#include "rix/ipc/shm_stream.hpp"
#include "rix/ipc/stream.hpp"
#include "rix/ipc/tcp_acceptor.hpp"
#include "rix/ipc/tcp_stream.hpp"

namespace rix {

enum Protocol : uint8_t { TCP = 0, SHM = 1 };

/**
 * @brief Type alias for socket factory function.
 */
using AcceptorFactory = std::function<std::shared_ptr<Acceptor>(const Endpoint&)>;
using StreamFactory = std::function<std::shared_ptr<Stream>(const Endpoint&, bool blocking)>;

struct TransportFactory {
  AcceptorFactory create_acceptor;
  StreamFactory create_stream;
};

namespace detail {
inline std::array<TransportFactory, 2> default_transport_factories = {
    {
     {[](const Endpoint& endpoint) -> std::shared_ptr<Acceptor> { return std::make_shared<TCPAcceptor>(endpoint); },
         [](const Endpoint& endpoint, bool blocking) -> std::shared_ptr<Stream> {
           return std::make_shared<TCPStream>(endpoint, blocking);
         }},
     {[](const Endpoint& endpoint) -> std::shared_ptr<Acceptor> { return std::make_shared<ShmAcceptor>(endpoint); },
         [](const Endpoint& endpoint, bool blocking) -> std::shared_ptr<Stream> {
           return std::make_shared<ShmStream>(endpoint, blocking);
         }},
     }
};

inline std::array<TransportFactory*, 2> transport_overrides = {nullptr, nullptr};
} // namespace detail

/**
 * @brief Returns the effective transport factory for the given protocol.
 *        Returns the test override if set, otherwise the default.
 */
inline const TransportFactory& get_transport_factory(Protocol protocol) {
  if (detail::transport_overrides[protocol]) {
    return *detail::transport_overrides[protocol];
  }
  return detail::default_transport_factories[protocol];
}

/**
 * @brief Override the transport factory for a given protocol (for testing).
 */
inline void set_transport_factory(Protocol protocol, TransportFactory* factory) {
  detail::transport_overrides[protocol] = factory;
}

/**
 * @brief Reset the transport factory override for a given protocol.
 */
inline void reset_transport_factory(Protocol protocol) { detail::transport_overrides[protocol] = nullptr; }

// Backward compatibility alias
inline const std::array<TransportFactory, 2>& transport_factories = detail::default_transport_factories;

/**
 * @brief Per-component transport configuration.
 *
 * Passed to Node::create_publisher, create_subscriber, etc. to select the
 * protocol and any protocol-specific options for that component.
 */
struct TransportOptions {
  Protocol protocol = Protocol::TCP;
  size_t shm_buffer_size = 4 * 1024 * 1024; ///< Used only when protocol == SHM.

  static TransportOptions tcp() { return {Protocol::TCP}; }
  static TransportOptions shm(size_t buffer_size = 4 * 1024 * 1024) { return {Protocol::SHM, buffer_size}; }

  /**
   * @brief Builds the TransportFactory for this component, applying any options.
   */
  TransportFactory build_factory() const {
    TransportFactory f = get_transport_factory(protocol);
    if (protocol == Protocol::SHM) {
      size_t sz = shm_buffer_size;
      f.create_acceptor = [sz](const Endpoint& ep) -> std::shared_ptr<Acceptor> {
        return std::make_shared<ShmAcceptor>(ep, 64, sz);
      };
    }
    return f;
  }
};

/**
 * @brief RAII guard that overrides the transport factory for a protocol for the
 * duration of a scope, then restores the previous state on destruction.
 *
 * Used by Node::create_* methods so that protocol-specific options (e.g. SHM
 * buffer size) reach the component impl without modifying any impl constructor.
 */
struct ScopedTransportOverride {
  ScopedTransportOverride(Protocol protocol, TransportFactory factory)
      : protocol_(protocol), owned_(std::move(factory)), previous_(detail::transport_overrides[protocol]) {
    set_transport_factory(protocol_, &owned_);
  }
  // Restore the previous override (not just null) so outer overrides survive.
  ~ScopedTransportOverride() { detail::transport_overrides[protocol_] = previous_; }

  ScopedTransportOverride(const ScopedTransportOverride&) = delete;
  ScopedTransportOverride& operator=(const ScopedTransportOverride&) = delete;

private:
  Protocol protocol_;
  TransportFactory owned_;
  TransportFactory* previous_;
};

} // namespace rix