#pragma once

#include "rix/ipc/acceptor.hpp"
#include "rix/ipc/stream.hpp"
#include "rix/ipc/tcp_acceptor.hpp"
#include "rix/ipc/tcp_stream.hpp"

namespace rix {

enum Protocol : uint8_t { TCP = 0 };

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
inline std::array<TransportFactory, 1> default_transport_factories = {
    {{[](const Endpoint& endpoint) -> std::shared_ptr<Acceptor> { return std::make_shared<TCPAcceptor>(endpoint); },
      [](const Endpoint& endpoint, bool blocking) -> std::shared_ptr<Stream> { return std::make_shared<TCPStream>(endpoint, blocking); }}}};

inline std::array<TransportFactory*, 1> transport_overrides = {nullptr};
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
inline void set_transport_factory(Protocol protocol, TransportFactory* factory) { detail::transport_overrides[protocol] = factory; }

/**
 * @brief Reset the transport factory override for a given protocol.
 */
inline void reset_transport_factory(Protocol protocol) { detail::transport_overrides[protocol] = nullptr; }

// Backward compatibility alias
inline const std::array<TransportFactory, 1>& transport_factories = detail::default_transport_factories;

} // namespace rix