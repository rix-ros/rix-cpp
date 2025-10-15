#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix;

const std::string NAME = "simple_service_client";

class SimpleServiceClient : public rix::Node {
public:
  SimpleServiceClient(int rate);

private:
  void timer_callback(const rix::TimerCallback::Event& event);
  uint32_t i_{0};
  std::shared_ptr<rix::ServiceClient> srv_cli_;
};
