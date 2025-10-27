#pragma once

#include "rix/core/node.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/msg/geometry/Point.hpp"
#include "rix/msg/geometry/Pose.hpp"
#include "rix/msg/geometry/Quaternion.hpp"
#include "rix/msg/geometry/Vector3.hpp"
#include "rix/msg/sensor/PointCloud.hpp"
#include "rix/tf/frame_graph.hpp"

namespace rix {

class TransformListener {
public:
  explicit TransformListener(const std::shared_ptr<Node>& node,
                             const Duration& duration = Duration(10.0),
                             const std::string& topic = "/tf",
                             const Endpoint& endpoint = Endpoint("127.0.0.1", 0));
  explicit TransformListener(Node& node,
                             const Duration& duration = Duration(10.0),
                             const std::string& topic = "/tf",
                             const Endpoint& endpoint = Endpoint("127.0.0.1", 0));
  TransformListener(const TransformListener& other);
  TransformListener& operator=(const TransformListener& other);

  bool get_transform(const std::string& target_frame,
                     const std::string& source_frame,
                     const Time& time,
                     msg::geometry::TransformStamped& transform) const;

  bool transform_point(const std::string& target_frame,
                       const std::string& source_frame,
                       const Time& time,
                       const msg::geometry::Point& point,
                       msg::geometry::Point& transformed_point) const;
  bool transform_point_cloud(const std::string& target_frame,
                             const std::string& source_frame,
                             const Time& time,
                             const msg::sensor::PointCloud& point_cloud,
                             msg::sensor::PointCloud& transformed_point_cloud) const;
  bool transform_pose(const std::string& target_frame,
                      const std::string& source_frame,
                      const Time& time,
                      const msg::geometry::Pose& pose,
                      msg::geometry::Pose& transformed_pose) const;
  bool transform_quaternion(const std::string& target_frame,
                            const std::string& source_frame,
                            const Time& time,
                            const msg::geometry::Quaternion& quaternion,
                            msg::geometry::Quaternion& transformed_quaternion) const;
  bool transform_vector(const std::string& target_frame,
                        const std::string& source_frame,
                        const Time& time,
                        const msg::geometry::Vector3& vector,
                        msg::geometry::Vector3& transformed_vector) const;

  bool ok() const;

  const FrameGraph& graph() const;
  Duration duration() const;
  void set_duration(const Duration& duration);

private:
  std::shared_ptr<Subscriber> subscriber_;
  FrameGraph graph_;
  Duration duration_;
};

} // namespace rix