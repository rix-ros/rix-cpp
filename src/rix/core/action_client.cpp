#include "rix/core/action_client.hpp"
#include "rix/std_msgs/Void.hpp"
#include "rix/sys_msgs/ActResponse.hpp"
#include "rix/sys_msgs/Status.hpp"

namespace rix {

ActionClient::~ActionClient() {
  if (MULTITHREADED) {
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

bool ActionClient::dispatch(const Message& goal) {
  std::lock_guard<std::mutex> guard(mutex_);
  uint8_t opcode;
  if (client_) {
    // If a goal is already active, send a preempt message
    opcode = OPCODE::ACT_PREEMPT_MESSAGE;
  } else {
    // Otherwise, create a new client and send a goal message
    opcode = OPCODE::ACT_GOAL_MESSAGE;
    client_ = socket_factory_.create_stream(endpoint_, true);
    if (!client_) {
      return false;
    }
  }

  if (!client_->send_message(opcode, goal)) {
    client_ = nullptr;
    return false;
  }

  // Clear the recv buffer to avoid stale messages (look for response)
  sys_msgs::Operation operation;
  while (true) {
    client_->recv_message(operation, operation.get_prefix_len());
    if (operation.opcode == OPCODE::ACT_RESPONSE_MESSAGE) {
      // Stop if we reach a response message
      break;
    }
    client_->ignore_message(operation.len);
  }

  // Read the response message
  sys_msgs::Status status;
  if (!client_->recv_message(status, operation.len)) {
    client_ = nullptr;
    return false;
  }

  if (operation.opcode != OPCODE::ACT_RESPONSE_MESSAGE) {
    client_ = nullptr;
    return false;
  }

  if (status.error != 0) {
    client_ = nullptr;
    return false;
  }

  result_received_ = false;
  return true;
}

bool ActionClient::cancel() {
  std::lock_guard<std::mutex> guard(mutex_);
  if (!client_) {
    return false;
  }
  uint8_t opcode = OPCODE::ACT_CANCEL_MESSAGE;
  std_msgs::Void void_msg;
  if (!client_->send_message(opcode, void_msg)) {
    client_ = nullptr;
    return false;
  }
  client_ = nullptr;
  result_received_ = false;
  return true;
}

bool ActionClient::wait_for_result(const Duration& timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_condition_.wait_for(lock, timeout.raw(), [this]() { return result_received_; });
}

ActionClient::ActionClient(const sys_msgs::ActRequest& request, TransportFactory factory, const Endpoint& rixhub_endpoint)
    : request_(request), socket_factory_(factory) {
  auto client = socket_factory_.create_stream(rixhub_endpoint, true);
  if (!client) {
    shutdown();
    return;
  }

  if (!client->send_message(OPCODE::ACT_REQUEST, request)) {
    shutdown();
    return;
  }

  sys_msgs::ActResponse response;
  sys_msgs::Operation operation;
  if (!client->recv_message(operation, response)) {
    shutdown();
    return;
  }

  if (operation.opcode != OPCODE::ACT_RESPONSE) {
    shutdown();
    return;
  }

  if (response.error != 0) {
    shutdown();
    return;
  }

  endpoint_.address = response.act_info.endpoint.address;
  endpoint_.port = response.act_info.endpoint.port;

  if (MULTITHREADED) {
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

void ActionClient::on_spin() {
  std::unique_lock<std::mutex> guard(mutex_);
  // If not ok, no active client, or no callbacks, do nothing
  if (!ok() || !client_ || !feedback_instance_ || !result_instance_) {
    return;
  }
  if (client_->is_readable()) {
    sys_msgs::Operation operation;
    if (!client_->recv_message(operation, operation.get_prefix_len())) {
      client_ = nullptr;
      return;
    }
    switch (operation.opcode) {
    case OPCODE::ACT_FEEDBACK_MESSAGE: {
      if (!client_->recv_message(*feedback_instance_, operation.len)) {
        client_ = nullptr;
        return;
      }
      guard.unlock();
      feedback_callback_(*feedback_instance_);
      return;
    }
    case OPCODE::ACT_RESULT_MESSAGE: {
      if (!client_->recv_message(*result_instance_, operation.len)) {
        client_ = nullptr;
        return;
      }
      // Action is complete, close the client
      client_ = nullptr;
      result_received_ = true;
      result_condition_.notify_all();
      guard.unlock();
      result_callback_(*result_instance_);
      return;
    }
    default:
      // Unknown opcode, close the client
      client_ = nullptr;
      return;
    }
  }
}

} // namespace rix