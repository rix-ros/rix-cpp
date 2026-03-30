#include "rix/tf/transform_broadcaster.hpp"

namespace rix {
TransformBroadcaster::TransformBroadcaster(const std::shared_ptr<Node>& node,
                                           const std::string& topic,
                                           const Endpoint& endpoint)
    : TransformBroadcaster(*node, topic, endpoint) {}

TransformBroadcaster::TransformBroadcaster(Node& node, const std::string& topic, const Endpoint& endpoint)
    : publisher_(node.create_publisher<geometry_msgs::TF>(topic, endpoint)) {
  if (!publisher_->ok()) {
    Log::error << "TransformBroadcaster: Failed to create publisher";
  }
}

TransformBroadcaster::TransformBroadcaster(const TransformBroadcaster& other) : publisher_(other.publisher_) {}

TransformBroadcaster& TransformBroadcaster::operator=(const TransformBroadcaster& other) {
  if (this != &other) {
    publisher_ = other.publisher_;
  }
  return *this;
}

void TransformBroadcaster::send(const geometry_msgs::TransformStamped& transform) const {
  geometry_msgs::TF tf;
  tf.transforms.push_back(transform);
  publisher_->publish(tf);
}

void TransformBroadcaster::send(const geometry_msgs::TF& tf) const { publisher_->publish(tf); }

bool TransformBroadcaster::ok() const { return publisher_->ok(); }

} // namespace rix