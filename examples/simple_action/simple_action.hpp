#include "rix/std_msgs/Double.hpp"
#include "rix/std_msgs/Float.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/rix.hpp"
#include <cmath>

using namespace rix;

const std::string NAME = "simple_action";

class SimpleAction final : public rix::Node {
public:
  SimpleAction(int max_iters, int port);

private:
  bool callback(const rix::std_msgs::Double& goal,
                rix::std_msgs::Float& feedback,
                rix::std_msgs::Double& result);
  int i_;
  int max_iters_;
  double value_;
  bool new_goal_;
};
