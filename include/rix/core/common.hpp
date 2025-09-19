#pragma once

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
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix::core {

const uint16_t RIXHUB_PORT = 48104;

enum OPCODE {
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

} // namespace rix::core