#pragma once

#include "rix/core/node.hpp"
#include "rix/core/publisher.hpp"
#include "rix/geometry_msgs/TF.hpp"
#include "rix/geometry_msgs/TransformStamped.hpp"

namespace rix {

class TransformBroadcaster {
public:
  explicit TransformBroadcaster(const std::shared_ptr<Node>& node,
                                const std::string& topic = "/tf",
                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));
  explicit TransformBroadcaster(Node& node,
                                const std::string& topic = "/tf",
                                const Endpoint& endpoint = Endpoint(DEFAULT_IP, 0));
  TransformBroadcaster(const TransformBroadcaster& other);
  TransformBroadcaster& operator=(const TransformBroadcaster& other);

  void send(const geometry_msgs::TransformStamped& transform) const;
  void send(const geometry_msgs::TF& tf) const;

  bool ok() const;

private:
  std::shared_ptr<Publisher> publisher_;
};

} // namespace rix