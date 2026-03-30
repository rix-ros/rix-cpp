#include "rix/core/action.hpp"
#include "rix/sys_msgs/Status.hpp"

namespace rix {
namespace detail {

ActionImpl::~ActionImpl() {
  if (registered_flag_) {
    auto client = factory_.create_stream(rixhub_endpoint_, true);
    if (!client) {
      return;
    }
    client->send_message(OPCODE::ACT_DEREGISTER, info_);
  }
  if (MULTITHREADED) {
    acceptor_.shutdown();
    if (acceptor_.spin_thread.joinable()) {
      acceptor_.spin_thread.join();
    }
    shutdown();
    if (spin_thread_.joinable()) {
      spin_thread_.join();
    }
  }
}

void ActionImpl::set_goal_callback(std::function<void()> callback) {
  std::lock_guard<std::mutex> guard(mutex_);
  goal_callback_ = callback;
}

void ActionImpl::set_preempt_callback(std::function<void()> callback) {
  std::lock_guard<std::mutex> guard(mutex_);
  preempt_callback_ = callback;
}

ActionImpl::ActionImpl(const sys_msgs::ActInfo& info, const Endpoint& rixhub_endpoint)
    : info_(info), factory_(get_transport_factory(static_cast<Protocol>(info.protocol))),
      rixhub_endpoint_(rixhub_endpoint) {
  server_ = factory_.create_acceptor(Endpoint(info_.endpoint.address, info_.endpoint.port));
  if (!server_) {
    shutdown();
    return;
  }

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
  auto client = factory_.create_stream(rixhub_endpoint_, true);
  if (!client->send_message(OPCODE::ACT_REGISTER, info_)) {
    shutdown();
    return;
  }

  sys_msgs::Operation operation;
  sys_msgs::Status status;
  if (!client->recv_message(operation, status)) {
    shutdown();
    return;
  }
  if (status.error) {
    shutdown();
    return;
  }

  registered_flag_ = true;

  Log::debug << "Action created for \"" << info_.name << "\".";

  if (MULTITHREADED) {
    acceptor_.spin_thread = std::thread([this]() { this->acceptor_.spin(); });
    spin_thread_ = std::thread([this]() { this->spin(); });
  }
}

ActionImpl::ActAcceptor::ActAcceptor(ActionImpl& parent) : parent(parent) {}

void ActionImpl::ActAcceptor::on_spin() {
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

  sys_msgs::Operation operation;
  if (!conn->recv_message(operation, operation.get_prefix_len())) {
    return;
  }

  sys_msgs::Status status;
  // Reject if already connected
  if (parent.connection_) {
    status.error = -1;
    conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    Log::debug << "Rejected ActionClient connection for \"" << parent.info_.name << "\" (already connected)."
              ;
    return;
  }

  // Reject if not a goal message
  if (operation.opcode != OPCODE::ACT_GOAL_MESSAGE) {
    status.error = -1;
    conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    Log::debug << "Rejected ActionClient connection for \"" << parent.info_.name << "\" (invalid opcode).";
    return;
  }

  // Read the goal message
  if (!conn->recv_message(*parent.goal_instance_, operation.len)) {
    status.error = -1;
    conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status);
    Log::debug << "Rejected ActionClient connection for \"" << parent.info_.name << "\" (invalid goal message)."
              ;
    return;
  }

  // Accept the connection
  status.error = 0;
  if (!conn->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status)) {
    return;
  }

  parent.connection_ = conn;
  Log::debug << "Accepted ActionClient connection for \"" << parent.info_.name << "\".";

  if (parent.goal_callback_) {
    parent.goal_callback_();
  }
}

void ActionImpl::on_spin() {
  if (!MULTITHREADED) {
    acceptor_.spin_once();
  }
  std::lock_guard<std::mutex> guard(mutex_);

  if (!ok() || !connection_ || !callback_) {
    return;
  }

  // Check for incoming messages from ActionClient
  if (connection_->wait_readable(Duration(0.0))) {
    sys_msgs::Operation operation;
    if (!connection_->recv_message(operation, operation.get_prefix_len())) {
      connection_ = nullptr;
      return;
    }

    sys_msgs::Status status;
    switch (operation.opcode) {
    case OPCODE::ACT_CANCEL_MESSAGE: {
      // Handle cancel message
      connection_ = nullptr;
      Log::debug << "Received cancel for action \"" << info_.name << "\".";
      return;
    }
    case OPCODE::ACT_PREEMPT_MESSAGE: {
      // Handle preempt message
      if (!connection_->recv_message(*goal_instance_, operation.len)) {
        connection_ = nullptr;
        return;
      }
      status.error = 0;
      if (!connection_->send_message(OPCODE::ACT_RESPONSE_MESSAGE, status)) {
        connection_ = nullptr;
        return;
      }
      Log::debug << "Received preempt for action \"" << info_.name << "\".";
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
      Log::debug << "Received invalid opcode for action \"" << info_.name << "\".";
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
    Log::debug << "Sent result for action \"" << info_.name << "\".";
    // Close the connection after sending the result
    connection_ = nullptr;
  } else {
    // Send feedback message
    if (!connection_->send_message(OPCODE::ACT_FEEDBACK_MESSAGE, *feedback_instance_)) {
      connection_ = nullptr;
      return;
    }
    Log::debug << "Sent feedback for action \"" << info_.name << "\".";
  }
}

void ActionImpl::set_callback(CallbackUntyped callback,
                              std::shared_ptr<Message> goal_instance,
                              std::shared_ptr<Message> feedback_instance,
                              std::shared_ptr<Message> result_instance) {
  if (goal_instance->hash() != info_.goal_hash || feedback_instance->hash() != info_.feedback_hash ||
      result_instance->hash() != info_.result_hash) {
    Log::warn << "Message type mismatch in set_callback.";
    return;
  }
  std::lock_guard<std::mutex> guard(mutex_);
  callback_ = std::move(callback);
  goal_instance_ = std::move(goal_instance);
  feedback_instance_ = std::move(feedback_instance);
  result_instance_ = std::move(result_instance);
}

} // namespace detail
} // namespace rix