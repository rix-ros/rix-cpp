#include "simple_service_client.hpp"

SimpleServiceClient::SimpleServiceClient(int rate) : rix::Node(NAME) {
  if (!ok()) {
    Log::error << "Failed to create node." << std::endl;
    return;
  }

  srv_cli_ = create_service_client<rix::msg::standard::UInt32, rix::msg::standard::String>("/alphabet");
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

SimpleServiceClient::~SimpleServiceClient() {}

void SimpleServiceClient::timer_callback(const rix::TimerCallback::Event& event) {
  rix::msg::standard::UInt32 req;
  req.data = i_++;
  rix::msg::standard::String res;
  if (!srv_cli_->call(req, res)) {
    return;
  }
  Log::info << "Sent request: " << req.data << ", received response: " << res.data << std::endl;
}