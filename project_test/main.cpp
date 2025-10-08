#include "rix/rix.hpp"
#include "rix/tf.hpp"

int main() {
  auto node = std::make_shared<rix::Node>("test_node", rix::Endpoint("127.0.0.1", 48104));
  auto tfb = std::make_shared<rix::TransformBroadcaster>(node);
}