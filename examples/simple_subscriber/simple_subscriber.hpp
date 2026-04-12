#include <sstream>

#include "rix/rix.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;

const std::string NAME = "simple_subscriber";

class SimpleSubscriber final : public Node {
public:
  SimpleSubscriber(int port);

private:
  void callback(const rix::std_msgs::Header& msg);
};