#include "rix/rix.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;

const std::string NAME = "parameter_server";

class ParameterServer final : public rix::Node {
public:
  ParameterServer(int port);
};
