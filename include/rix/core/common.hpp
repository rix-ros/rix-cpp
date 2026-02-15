#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>

#include "rix/ipc/acceptor.hpp"
#include "rix/ipc/stream.hpp"
#include "rix/ipc/tcp_acceptor.hpp"
#include "rix/ipc/tcp_stream.hpp"
#include "rix/util/environment.hpp"
#include "rix/util/log.hpp"
#include <thread>

namespace rix {

// Default RIXHub IP will first check RIX_RIXHUB_IP, then RIX_DEFAULT_IP, then fallback to loopback address.
static inline const std::string RIXHUB_IP{get_env("RIX_RIXHUB_IP", get_env("RIX_DEFAULT_IP", "127.0.0.1"))};

// Default RIXHub port is 48104, can be overridden by RIX_RIXHUB_PORT environment variable
static inline const uint16_t RIXHUB_PORT{static_cast<uint16_t>(std::stoi(get_env("RIX_RIXHUB_PORT", "48104")))};

// Default IP is loopback address, can be overridden by RIX_DEFAULT_IP environment variable
static inline const std::string DEFAULT_IP{get_env("RIX_DEFAULT_IP", "127.0.0.1")};

// Multithreading is disabled by default, can be enabled by setting RIX_MULTITHREADED to "1"
static inline const bool MULTITHREADED{get_env("RIX_MULTITHREADED", "0") != "0"};

enum OPCODE : uint8_t {
  STATUS_RESPONSE = 0,   ///< Sent as response to various requests
  PING,                  ///< Sent to check connectivity
  NODE_REGISTER = 80,    ///< Sent to RIXHub to register a node
  SUB_REGISTER,          ///< Sent to RIXHub to register a subscriber
  PUB_REGISTER,          ///< Sent to RIXHub to register a publisher
  SRV_REGISTER,          ///< Sent to RIXHub to register a service
  ACT_REGISTER,          ///< Sent to RIXHub to register an action
  SUB_NOTIFY = 90,       ///< Sent to notify subscriber of new publishers
  NODE_DEREGISTER = 100, ///< Sent to RIXHub to deregister a node
  SUB_DEREGISTER,        ///< Sent to RIXHub to deregister a subscriber
  PUB_DEREGISTER,        ///< Sent to RIXHub to deregister a publisher
  SRV_DEREGISTER,        ///< Sent to RIXHub to deregister a service
  ACT_DEREGISTER,        ///< Sent to RIXHub to deregister an action
  PUB_MESSAGE = 120,     ///< Sent from Publisher to Subscriber
  SRV_REQUEST_MESSAGE,   ///< Sent from Service client to Service server
  SRV_RESPONSE_MESSAGE,  ///< Sent from Service server to Service client
  ACT_GOAL_MESSAGE,      ///< Sent to Action server to request action
  ACT_PREEMPT_MESSAGE,   ///< Sent to Action server to preempt current action
  ACT_CANCEL_MESSAGE,    ///< Sent to Action server to cancel current action
  ACT_RESPONSE_MESSAGE,  ///< Sent from Action server to Action client as response to goal/preempt/cancel
  ACT_FEEDBACK_MESSAGE,  ///< Sent from Action server to Action client as feedback during action execution
  ACT_RESULT_MESSAGE,    ///< Sent from Action server to Action client as result of action execution
  SRV_REQUEST = 140,     ///< Sent from ServiceClient to RIXHub to request information about a Service
  ACT_REQUEST,           ///< Sent from ActionClient to RIXHub to request information about an Action
  PARAM_SET_REQUEST,     ///< Sent from Node to RIXHub to set a parameter value
  PARAM_GET_REQUEST,     ///< Sent from Node to RIXHub to get a parameter value
  SYSTEM_GET_REQUEST,    ///< Sent from Node to RIXHub to get system information
  SRV_RESPONSE = 160,    ///< Sent from RIXHub as response to SRV_REQUEST
  ACT_RESPONSE,          ///< Sent from RIXHub as response to ACT_REQUEST
  PARAM_GET_RESPONSE,    ///< Sent from RIXHub as response to PARAM_GET_REQUEST
  SYSTEM_GET_RESPONSE,   ///< Sent from RIXHub as response to SYSTEM_GET_REQUEST
};

/**
 * @brief Type alias for socket factory function.
 */
using AcceptorFactory = std::function<std::shared_ptr<Acceptor>(const Endpoint&)>;
using StreamFactory = std::function<std::shared_ptr<Stream>(const Endpoint&, bool blocking)>;

struct TransportFactory {
  AcceptorFactory create_acceptor;
  StreamFactory create_stream;
};

enum Protocol : uint8_t { TCP = 0 };

const std::array<TransportFactory, 1> transport_factories = {
    {{[](const Endpoint& endpoint) { return std::make_shared<TCPAcceptor>(endpoint); },
      [](const Endpoint& endpoint, bool blocking) { return std::make_shared<TCPStream>(endpoint, blocking); }}}};

/**
 * @brief Type alias for ID factory function.
 */
using IDFactory = std::function<uint64_t(void)>;

/**
 * @brief Default ID generator using random number generation.
 * @return A randomly generated uint64_t ID.
 */
static inline uint64_t default_id_generator() {
  static std::mutex mutex;
  static std::random_device rd;
  static std::mt19937_64 eng(rd());
  static std::uniform_int_distribution<uint64_t> distr;

  std::lock_guard<std::mutex> lock(mutex);
  return distr(eng);
}

} // namespace rix