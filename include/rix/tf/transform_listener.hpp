#pragma once

#include "rix/core/node.hpp"
#include "rix/core/subscriber.hpp"
#include "rix/geometry_msgs/Point.hpp"
#include "rix/geometry_msgs/Pose.hpp"
#include "rix/geometry_msgs/Quaternion.hpp"
#include "rix/geometry_msgs/Vector3.hpp"
#include "rix/sensor_msgs/PointCloud.hpp"
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
                     geometry_msgs::TransformStamped& transform) const;

  bool transform_point(const std::string& target_frame,
                       const std::string& source_frame,
                       const Time& time,
                       const geometry_msgs::Point& point,
                       geometry_msgs::Point& transformed_point) const;
  bool transform_point_cloud(const std::string& target_frame,
                             const std::string& source_frame,
                             const Time& time,
                             const sensor_msgs::PointCloud& point_cloud,
                             sensor_msgs::PointCloud& transformed_point_cloud) const;
  bool transform_pose(const std::string& target_frame,
                      const std::string& source_frame,
                      const Time& time,
                      const geometry_msgs::Pose& pose,
                      geometry_msgs::Pose& transformed_pose) const;
  bool transform_quaternion(const std::string& target_frame,
                            const std::string& source_frame,
                            const Time& time,
                            const geometry_msgs::Quaternion& quaternion,
                            geometry_msgs::Quaternion& transformed_quaternion) const;
  bool transform_vector(const std::string& target_frame,
                        const std::string& source_frame,
                        const Time& time,
                        const geometry_msgs::Vector3& vector,
                        geometry_msgs::Vector3& transformed_vector) const;

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