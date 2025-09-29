#include "rix/rix.hpp"
#include "rix/tf.hpp"

int main() {
  auto node = std::make_shared<rix::core::Node>("test_node", rix::ipc::Endpoint("127.0.0.1", 48104));
  auto tfb = std::make_shared<rix::tf::TransformBroadcaster>(node);
}