#include "simple_action.hpp"

SimpleAction::SimpleAction(int max_iters, int port) : Node(NAME), i_(0), max_iters_(max_iters) {
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  // Pass member function pointer and 'this' - no lambda or std::bind needed!
  auto act = create_action("/exponent", &SimpleAction::callback, this, Endpoint(DEFAULT_IP, port));
  if (!act->ok()) {
    Log::error << "Failed to create service." << std::endl;
    shutdown();
    return;
  }
  act->set_goal_callback([this]() {
    Log::info << "New goal received." << std::endl;
    i_ = 0;
    value_ = 0.0;
    new_goal_ = true;
  });
  act->set_preempt_callback([this]() {
    Log::info << "Goal preempted." << std::endl;
    i_ = 0;
    value_ = 0.0;
    new_goal_ = true;
  });
}

bool SimpleAction::callback(const rix::std_msgs::Double& goal,
                            rix::std_msgs::Float& feedback,
                            rix::std_msgs::Double& result) {
  // Perform a taylor series expansion to approximate e^(goal.data) store intermediate data in value_ and return the
  // percent complete in feedback
  if (new_goal_) {
    Log::info << "Starting new goal: e^" << goal.data << " with max iters: " << max_iters_ << std::endl;
    new_goal_ = false;
  }
  if (i_ >= max_iters_) {
    result.data = value_;
    Log::info << "Goal complete: e^" << goal.data << " ~= " << result.data << " after " << i_ << " iterations."
              << std::endl;
    return true;
  }
  value_ += pow(goal.data, i_) / tgamma(static_cast<double>(i_ + 1));
  feedback.data = (static_cast<float>(i_) / static_cast<float>(max_iters_)) * 100.0f;
  Log::info << "Goal progress: " << feedback.data << "\% after " << i_ << " iterations." << std::endl;
  i_++;
  Time::sleep_for(Duration(0.001)); // Simulate work being done
  return false;
}
