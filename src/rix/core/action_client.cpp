#include "rix/core/action_client.hpp"

namespace rix {

ActionClient::~ActionClient() {
#ifdef RIX_MULTITHREADED
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif
}

bool ActionClient::dispatch(const msg::Message& goal) {
  std::lock_guard<std::mutex> guard(mutex_);
  uint8_t opcode;
  if (client_) {
    // If a goal is already active, send a preempt message
    opcode = OPCODE::ACT_PREEMPT_MESSAGE;
  } else {
    // Otherwise, create a new client and send a goal message
    opcode = OPCODE::ACT_GOAL_MESSAGE;
    client_ = socket_factory_();

    if (!client_->connect(endpoint_)) {
      client_ = nullptr;
      return false;
    }
  }

  if (!client_->send_message(opcode, goal)) {
    client_ = nullptr;
    return false;
  }

  // Clear the recv buffer to avoid stale messages (look for response)
  msg::mediator::Operation op;
  while (true) {
    client_->recv_message(op, op.size());
    if (op.opcode == OPCODE::ACT_RESPONSE_MESSAGE) {
      // Stop if we reach a response message
      break;
    }
    client_->ignore_message(op.len);
  }

  // Read the response message
  msg::mediator::Status status;
  if (!client_->recv_message(status, op.len)) {
    client_ = nullptr;
    return false;
  }

  if (op.opcode != OPCODE::ACT_RESPONSE_MESSAGE) {
    client_ = nullptr;
    return false;
  }

  if (status.error) {
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
  msg::standard::Void void_msg;
  if (!client_->send_message(opcode, void_msg)) {
    client_ = nullptr;
    return false;
  }
  client_ = nullptr;
  result_received_ = false;
  return true;
}

bool ActionClient::wait_for_result(const Duration& d) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_condition_.wait_for(lock, d.raw(), [this]() { return result_received_; });
}

ActionClient::ActionClient(const msg::mediator::ActRequest& request,
                           SocketFactory factory,
                           const Endpoint& rixhub_endpoint)
    : request_(request), socket_factory_(factory) {
  auto client = socket_factory_();
  if (!client) {
    shutdown();
    return;
  }

  if (!client->connect(rixhub_endpoint)) {
    shutdown();
    return;
  }

  if (!client->send_message(OPCODE::ACT_REQUEST, request)) {
    shutdown();
    return;
  }

  msg::mediator::ActResponse response;
  msg::mediator::Operation op;
  if (!client->recv_message(op, response)) {
    shutdown();
    return;
  }

  if (op.opcode != OPCODE::ACT_RESPONSE) {
    shutdown();
    return;
  }

  if (response.error) {
    shutdown();
    return;
  }

  endpoint_.address = response.act_info.endpoint.address;
  endpoint_.port = response.act_info.endpoint.port;

#ifdef RIX_MULTITHREADED
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

void ActionClient::on_spin() {
  std::lock_guard<std::mutex> guard(mutex_);
  // If not ok, no active client, or no callbacks, do nothing
  if (!ok() || !client_ || !feedback_instance_ || !result_instance_) {
    return;
  }
  if (client_->is_readable()) {
    msg::mediator::Operation op;
    if (!client_->recv_message(op, op.size())) {
      client_ = nullptr;
      return;
    }
    switch (op.opcode) {
    case OPCODE::ACT_FEEDBACK_MESSAGE: {
      if (!client_->recv_message(*feedback_instance_, op.len)) {
        client_ = nullptr;
        return;
      }
      feedback_callback_(*feedback_instance_);
      break;
    }
    case OPCODE::ACT_RESULT_MESSAGE: {
      if (!client_->recv_message(*result_instance_, op.len)) {
        client_ = nullptr;
        return;
      }
      result_callback_(*result_instance_);
      // Action is complete, close the client
      client_ = nullptr;
      result_received_ = true;
      result_condition_.notify_all();
      break;
    }
    default:
      // Unknown opcode, close the client
      client_ = nullptr;
      return;
    }
  }
}

} // namespace rix