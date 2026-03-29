#include "rix/rix.hpp"
#include "rix/std_msgs/Double.hpp"
#include "rix/std_msgs/Float.hpp"
#include "rix/std_msgs/UInt32.hpp"

using namespace rix;

const std::string NAME = "simple_action_client";

class SimpleActionClient final : public Node {
public:
  SimpleActionClient(double rate);

private:
  void timer_callback(const TimerCallback::Event& event);
  double i_{0};
  std::shared_ptr<ActionClient> act_cli_;
};
