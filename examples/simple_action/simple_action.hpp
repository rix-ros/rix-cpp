#include "rix/msg/standard/Double.hpp"
#include "rix/msg/standard/Float.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"
#include <cmath>

using namespace rix;

const std::string NAME = "simple_action";

class SimpleAction final : public rix::Node {
public:
  SimpleAction(int max_iters, int port);

private:
  bool callback(const rix::msg::standard::Double& goal,
                rix::msg::standard::Float& feedback,
                rix::msg::standard::Double& result);
  int i_;
  int max_iters_;
  double value_;
  bool new_goal_;
};
