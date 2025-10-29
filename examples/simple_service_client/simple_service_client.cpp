#include "simple_service_client.hpp"

SimpleServiceClient::SimpleServiceClient(int rate) : rix::Node(NAME) {
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  srv_cli_ = create_service_client<rix::std_msgs::UInt32, rix::std_msgs::String>("/alphabet");
  if (!srv_cli_->ok()) {
    shutdown();
    Log::error << "Failed to create service client." << std::endl;
    return;
  }

  uint32_t i = 0;
  auto timer = create_timer(Duration(1.0 / rate), &SimpleServiceClient::timer_callback, this);
  if (!timer->ok()) {
    shutdown();
    Log::error << "Failed to create timer." << std::endl;
    return;
  }
}

void SimpleServiceClient::timer_callback(const rix::TimerCallback::Event& event) {
  rix::std_msgs::UInt32 req;
  req.data = i_++;
  rix::std_msgs::String res;
  if (!srv_cli_->call(req, res)) {
    return;
  }
  Log::info << "Sent request: " << req.data << ", received response: " << res.data << std::endl;
}