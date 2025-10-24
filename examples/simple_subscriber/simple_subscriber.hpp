#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"

#include <sstream>

using namespace rix;

const std::string NAME = "simple_subscriber";

class SimpleSubscriber final : public Node {
public:
  // Initialize the Node with a name and the RixHub endpoint
  SimpleSubscriber(int port);

private:
  void callback(const rix::msg::standard::Header& msg);
};