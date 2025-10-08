#pragma once

#include "rix/core/node.hpp"
#include "rix/core/publisher.hpp"
#include "rix/msg/geometry/TF.hpp"
#include "rix/msg/geometry/Transform.hpp"
#include "rix/msg/geometry/TransformStamped.hpp"
#include "rix/util/log.hpp"

namespace rix {

class TransformBroadcaster {
public:
  TransformBroadcaster(std::shared_ptr<Node> node, const std::string &topic = "/tf",
                       const Endpoint &endpoint = Endpoint(DEFAULT_IP, 0));
  TransformBroadcaster(Node &node, const std::string &topic = "/tf",
                       const Endpoint &endpoint = Endpoint(DEFAULT_IP, 0));
  TransformBroadcaster(const TransformBroadcaster &other);
  TransformBroadcaster &operator=(const TransformBroadcaster &other);

  void send(const msg::geometry::TransformStamped &transform);
  void send(const msg::geometry::TF &tf);

  bool ok() const;

private:
  std::shared_ptr<Publisher> publisher_;
};

} // namespace rix