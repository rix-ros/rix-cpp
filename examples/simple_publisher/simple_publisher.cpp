#include "simple_publisher.hpp"

SimplePublisher::SimplePublisher(double rate, int port) : rix::Node(NAME) {
  if (!this->ok()) {
    rix::Log::error << "Failed to create node." << std::endl;
    return;
  }

  pub_ = this->template create_publisher<rix::std_msgs::Header>("/chatter", rix::Endpoint(rix::DEFAULT_IP, port));

  if (!pub_ || !pub_->ok()) {
    rix::Log::error << "Failed to create publisher." << std::endl;
    this->shutdown();
    return;
  }

  message_.frame_id = "Hello!";
  message_.seq = 0;

  timer_ = this->create_timer(rix::Duration(1.0 / rate), &SimplePublisher::timer_callback_, this);
}

void SimplePublisher::timer_callback_(const rix::TimerCallback::Event& /*event*/) {
  message_.seq += 1;
  message_.stamp = rix::Time::now().to_msg();
  pub_->publish(message_);
}