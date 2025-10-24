#include "simple_action_client.hpp"

SimpleActionClient::SimpleActionClient(double rate) : Node(NAME) {
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  act_cli_ = create_action_client<msg::standard::Double, msg::standard::Float, msg::standard::Double>("/exponent");
  if (!act_cli_->ok()) {
    shutdown();
    Log::error << "Failed to create action client." << std::endl;
    return;
  }

  act_cli_->set_feedback_callback<msg::standard::Float>([](const msg::standard::Float& feedback) {
    Log::info << "Received feedback: " << feedback.data << "\%" << std::endl;
  });
  act_cli_->set_result_callback<msg::standard::Double>(
      [](const msg::standard::Double& result) { Log::info << "Received result: " << result.data << std::endl; });

  std::shared_ptr<double> i_ptr = std::make_shared<double>(0.0);
  auto timer = create_timer(Duration(1.0 / rate), &SimpleActionClient::timer_callback, this);
  if (!timer->ok()) {
    shutdown();
    Log::error << "Failed to create timer." << std::endl;
    return;
  }
}

void SimpleActionClient::timer_callback(const TimerCallback::Event& event) {
  msg::standard::Double goal;
  goal.data = i_;
  i_ += 0.1;
  if (!act_cli_->dispatch(goal)) {
    return;
  }
  Log::info << "Sent request: " << goal.data << std::endl;
}