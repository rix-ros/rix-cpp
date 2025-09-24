#include "rix/rix.hpp"

int main() {
  auto node = std::make_shared<rix::core::Node>(
      "test_node", rix::ipc::Endpoint("127.0.0.1", 48104));
}