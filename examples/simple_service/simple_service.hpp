#include "rix/msg/standard/String.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/rix.hpp"

using namespace rix;

const std::string NAME = "simple_service";

class SimpleService final : public rix::Node {
public:
  SimpleService(int port);

private:
  void callback(const rix::msg::standard::UInt32& request, rix::msg::standard::String& response);
};
