#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/socket.hpp"
#include "rix/util/environment.hpp"
#include "rix/util/log.hpp"

#ifdef RIX_MULTITHREADED
#include <thread>
#endif

namespace rix::core {

// Default RIXHub IP will first check RIX_RIXHUB_IP, then RIX_DEFAULT_IP, then fallback to loopback address.
static inline std::string RIXHUB_IP{
    rix::util::get_env("RIX_RIXHUB_IP", rix::util::get_env("RIX_DEFAULT_IP", "127.0.0.1"))};

// Default RIXHub port is 48104, can be overridden by RIX_RIXHUB_PORT environment variable
static inline uint16_t RIXHUB_PORT{static_cast<uint16_t>(std::stoi(rix::util::get_env("RIX_RIXHUB_PORT", "48104")))};

// Default IP is loopback address, can be overridden by RIX_DEFAULT_IP environment variable
static inline std::string DEFAULT_IP{rix::util::get_env("RIX_DEFAULT_IP", "127.0.0.1")};

enum OPCODE : uint8_t {
  STATUS_RESPONSE = 0,

  NODE_REGISTER = 80,
  SUB_REGISTER,
  PUB_REGISTER,
  SRV_REGISTER,
  ACT_REGISTER,

  SUB_NOTIFY = 90,

  NODE_DEREGISTER = 100,
  SUB_DEREGISTER,
  PUB_DEREGISTER,
  SRV_DEREGISTER,
  ACT_DEREGISTER,

  PUB_MESSAGE = 120,
  SRV_REQUEST_MESSAGE,
  SRV_RESPONSE_MESSAGE,
  ACT_COMMAND_MESSAGE,
  ACT_FEEDBACK_MESSAGE,
  ACT_RESULT_MESSAGE,

  SRV_REQUEST = 140,
  ACT_REQUEST,
  PARAM_SET_REQUEST,
  PARAM_GET_REQUEST,
  SYSTEM_GET_REQUEST,

  SRV_RESPONSE = 160,
  ACT_RESPONSE,
  PARAM_GET_RESPONSE,
  SYSTEM_GET_RESPONSE,
};

using SocketFactory = std::function<std::shared_ptr<rix::ipc::GenericSocket>(void)>;

static inline bool parse_endpoint(const std::string &str, std::any &value) {
  rix::ipc::Endpoint endpoint(str);
  if (endpoint.port < 0) {
    return false;
  }
  value = endpoint;
  return true;
}

} // namespace rix::core