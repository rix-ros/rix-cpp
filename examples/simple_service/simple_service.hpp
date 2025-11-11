#include "rix/std_msgs/String.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix;

const std::string NAME = "simple_service";

class SimpleService final : public rix::Node {
public:
  SimpleService(int port);

private:
  void callback(const rix::std_msgs::UInt32& request, rix::std_msgs::String& response);
};
