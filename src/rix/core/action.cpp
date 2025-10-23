#include "rix/core/action.hpp"

namespace rix {

Action::~Action() {
  if (registered_flag_) {
    auto client = socket_factory_();
    if (!client) {
      return;
    }
    if (client->connect(rixhub_endpoint_)) {
      client->send_message(OPCODE::ACT_DEREGISTER, info_);
    }
  }
#ifdef RIX_MULTITHREADED
  acceptor_.shutdown();
  if (acceptor_.spin_thread.joinable()) {
    acceptor_.spin_thread.join();
  }
  shutdown();
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
#endif
}

void Action::set_goal_callback(std::function<void()> callback) {
  std::lock_guard<std::mutex> guard(mutex_);
  goal_callback_ = callback;
}

void Action::set_preempt_callback(std::function<void()> callback) {
  std::lock_guard<std::mutex> guard(mutex_);
  preempt_callback_ = callback;
}

Action::Action(const msg::mediator::ActInfo& info, SocketFactory socket_factory, const Endpoint& rixhub_endpoint)
    : info_(info), socket_factory_(socket_factory), rixhub_endpoint_(rixhub_endpoint) {
  server_ = socket_factory_();
  if (!server_) {
    shutdown();
    return;
  }

  server_->set_reuse_address(true);
  server_->bind(Endpoint(info_.endpoint.address, info_.endpoint.port));
  server_->listen(MAX_CONN);

  // Ensure server was intitialized properly
  if (server_->is_exception()) {
    shutdown();
    return;
  }

  auto server_endpoint = server_->local_endpoint();
  // Update the endpoint in case the port was set to 0 (ephemeral)
  info_.endpoint.address = server_endpoint.address;
  info_.endpoint.port = server_endpoint.port;

  // Register publisher with rixhub
  auto client = socket_factory_();
  if (!client->connect(rixhub_endpoint_)) {
    shutdown();
    return;
  }
  if (!client->send_message(OPCODE::ACT_REGISTER, info_)) {
    shutdown();
    return;
  }

  msg::mediator::Operation op;
  msg::mediator::Status status;
  if (!client->recv_message(op, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;

  Log::debug << "Action created for \"" << info_.name << "\"." << std::endl;

#ifdef RIX_MULTITHREADED
  acceptor_.spin_thread = std::thread([this]() { this->acceptor_.spin(); });
  spin_thread_ = std::thread([this]() { this->spin(); });
#endif
}

Action::ActAcceptor::ActAcceptor(Action& parent) : parent(parent) {}

void Action::ActAcceptor::on_spin() {
  std::lock_guard<std::mutex> guard(parent.mutex_);
  if (!parent.ok() || !parent.callback_) {
    return;
  }

  // Check if ActionClient has made a connection
  if (!parent.server_->wait_readable(Duration(0.0))) {
    return;
  }
  // Accept a connection from ActionClient
  auto conn = parent.server_->accept();
  if (!conn) {
    return;
  }

  msg::mediator::Operation op;
  if (!conn->recv_message(op, op.size())) {
    return;
  }

  msg::mediator::Status status;
  // Reject if already connected
  if (parent.connection_) {
    status.error = -1;
    conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    Log::debug << "Rejected ActionClient connection for \"" << parent.info_.name << "\" (already connected)."
               << std::endl;
    return;
  }

  // Reject if not a goal message
  if (op.opcode != OPCODE::ACT_GOAL_MESSAGE) {
    status.error = -1;
    conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    Log::debug << "Rejected ActionClient connection for \"" << parent.info_.name << "\" (invalid opcode)." << std::endl;
    return;
  }

  // Read the goal message
  if (!conn->recv_message(*parent.goal_instance_, op.len)) {
    status.error = -1;
    conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    Log::debug << "Rejected ActionClient connection for \"" << parent.info_.name << "\" (invalid goal message)."
               << std::endl;
    return;
  }

  // Accept the connection
  status.error = 0;
  if (!conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status)) {
    return;
  }

  parent.connection_ = conn;
  Log::debug << "Accepted ActionClient connection for \"" << parent.info_.name << "\"." << std::endl;

  if (parent.goal_callback_) {
    parent.goal_callback_();
  }
}

void Action::on_spin() {
#ifndef RIX_MULTITHREADED
  acceptor_.spin_once();
#endif
  std::lock_guard<std::mutex> guard(mutex_);

  if (!ok() || !connection_ || !callback_) {
    return;
  }

  // Check for incoming messages from ActionClient
  if (connection_->wait_readable(Duration(0.0))) {
    msg::mediator::Operation op;
    if (!connection_->recv_message(op, op.size())) {
      connection_ = nullptr;
      return;
    }

    msg::mediator::Status status;
    switch (op.opcode) {
    case OPCODE::ACT_CANCEL_MESSAGE: {
      // Handle cancel message
      connection_ = nullptr;
      Log::debug << "Received cancel for action \"" << info_.name << "\"." << std::endl;
      return;
    }
    case OPCODE::ACT_PREEMPT_MESSAGE: {
      // Handle preempt message
      if (!connection_->recv_message(*goal_instance_, op.len)) {
        connection_ = nullptr;
        return;
      }
      status.error = 0;
      if (!connection_->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status)) {
        connection_ = nullptr;
        return;
      }
      Log::debug << "Received preempt for action \"" << info_.name << "\"." << std::endl;
      if (preempt_callback_) {
        preempt_callback_();
      }
      break;
    }
    default: {
      status.error = -1;
      connection_->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
      // Invalid opcode, close connection
      connection_ = nullptr;
      Log::debug << "Received invalid opcode for action \"" << info_.name << "\"." << std::endl;
      return;
    }
    }
  }

  // Invoke the callback
  bool is_result = callback_(*goal_instance_, *feedback_instance_, *result_instance_);
  if (is_result) {
    // Send result message
    if (!connection_->send_message(OPCODE::ACT_RESULT_MESSAGE, *result_instance_)) {
      connection_ = nullptr;
      return;
    }
    Log::debug << "Sent result for action \"" << info_.name << "\"." << std::endl;
    // Close the connection after sending the result
    connection_ = nullptr;
  } else {
    // Send feedback message
    if (!connection_->send_message(OPCODE::ACT_FEEDBACK_MESSAGE, *feedback_instance_)) {
      connection_ = nullptr;
      return;
    }
    Log::debug << "Sent feedback for action \"" << info_.name << "\"." << std::endl;
  }
}

} // namespace rix