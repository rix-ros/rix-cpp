#include "simple_action_client.hpp"

SimpleActionClient::SimpleActionClient(double rate) : Node(NAME) {
  if (!ok()) {
    Log::error << "Failed to create node.";
    return;
  }

  act_cli_ = create_action_client<std_msgs::Double, std_msgs::Float, std_msgs::Double>(
      "/exponent",
      [](const std_msgs::Float& feedback) { Log::info << "Received feedback: " << feedback.data << "\%"; },
      [](const std_msgs::Double& result) { Log::info << "Received result: " << result.data; });
  if (!act_cli_->ok()) {
    shutdown();
    Log::error << "Failed to create action client.";
    return;
  }

  auto timer = create_timer(Duration(1.0 / rate), &SimpleActionClient::timer_callback, this);
  if (!timer->ok()) {
    shutdown();
    Log::error << "Failed to create timer.";
    return;
  }
}

void SimpleActionClient::timer_callback(const TimerCallback::Event& event) {
  std_msgs::Double goal;
  goal.data = i_;
  i_ += 0.1;
  if (!act_cli_->dispatch(goal)) {
    return;
  }
  Log::info << "Sent request: " << goal.data;
}