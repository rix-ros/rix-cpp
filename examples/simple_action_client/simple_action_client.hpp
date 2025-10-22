#include "rix/msg/standard/Double.hpp"
#include "rix/msg/standard/Float.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix;

const std::string NAME = "simple_action_client";

class SimpleActionClient : public Node {
public:
  SimpleActionClient(double rate);
  ~SimpleActionClient();

private:
  void timer_callback(const TimerCallback::Event& event);
  double i_{0};
  std::shared_ptr<ActionClient> act_cli_;
};
