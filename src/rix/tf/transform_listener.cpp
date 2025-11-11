
#include "rix/tf/transform_listener.hpp"

#include <eigen3/Eigen/Geometry>

#include "rix/rob/eigen_util.hpp"

namespace rix {

TransformListener::TransformListener(const std::shared_ptr<Node>& node,
                                     const Duration& duration,
                                     const std::string& topic,
                                     const Endpoint& endpoint)
    : TransformListener(*node, duration, topic, endpoint) {}

TransformListener::TransformListener(Node& node,
                                     const Duration& duration,
                                     const std::string& topic,
                                     const Endpoint& endpoint)
    : subscriber_(node.create_subscriber<geometry_msgs::TF>(
          topic,
          [this](const geometry_msgs::TF& msg) {
            if (!graph_.update(msg)) {
              Log::warn << "Failed to update transform from TF message." << std::endl;
            }
          },
          endpoint)),
      graph_("world", duration), duration_(duration) {}

TransformListener::TransformListener(const TransformListener& other)
    : subscriber_(other.subscriber_), graph_(other.graph_), duration_(other.duration_) {}

TransformListener& TransformListener::operator=(const TransformListener& other) {
  if (this != &other) {
    subscriber_ = other.subscriber_;
    graph_ = other.graph_;
    duration_ = other.duration_;
  }
  return *this;
}

bool TransformListener::get_transform(const std::string& target_frame,
                                      const std::string& source_frame,
                                      const Time& time,
                                      geometry_msgs::TransformStamped& transform) const {
  return graph_.get_transform(target_frame, source_frame, time, transform);
}

bool TransformListener::transform_point(const std::string& target_frame,
                                        const std::string& source_frame,
                                        const Time& time,
                                        const geometry_msgs::Point& point,
                                        geometry_msgs::Point& transformed_point) const {
  geometry_msgs::TransformStamped transform;
  if (!graph_.get_transform(target_frame, source_frame, time, transform)) {
    return false;
  }
  Eigen::Affine3d eigen_transform = msg_to_eigen(transform.transform);

  Eigen::Vector3d eigen_point = msg_to_eigen(point);
  Eigen::Vector3d transformed_eigen_point = eigen_transform * eigen_point;
  transformed_point.x = transformed_eigen_point.x();
  transformed_point.y = transformed_eigen_point.y();
  transformed_point.z = transformed_eigen_point.z();
  return true;
}

bool TransformListener::transform_point_cloud(const std::string& target_frame,
                                              const std::string& source_frame,
                                              const Time& time,
                                              const sensor_msgs::PointCloud& point_cloud,
                                              sensor_msgs::PointCloud& transformed_point_cloud) const {
  transformed_point_cloud.header = point_cloud.header;
  transformed_point_cloud.header.frame_id = target_frame;
  transformed_point_cloud.points.resize(point_cloud.points.size());
  transformed_point_cloud.channels.resize(point_cloud.channels.size());

  geometry_msgs::TransformStamped transform;
  if (!graph_.get_transform(target_frame, source_frame, time, transform)) {
    return false;
  }
  Eigen::Affine3d eigen_transform = msg_to_eigen(transform.transform);

  for (size_t i = 0; i < point_cloud.points.size(); ++i) {
    const auto& point = point_cloud.points[i];
    auto& t_point = transformed_point_cloud.points[i];

    Eigen::Vector3d eigen_point = msg_to_eigen(point);
    Eigen::Vector3d transformed_eigen_point = eigen_transform * eigen_point;
    t_point.x = transformed_eigen_point.x();
    t_point.y = transformed_eigen_point.y();
    t_point.z = transformed_eigen_point.z();
  }
  return true;
}

bool TransformListener::transform_pose(const std::string& target_frame,
                                       const std::string& source_frame,
                                       const Time& time,
                                       const geometry_msgs::Pose& pose,
                                       geometry_msgs::Pose& transformed_pose) const {
  geometry_msgs::TransformStamped transform;
  if (!graph_.get_transform(target_frame, source_frame, time, transform)) {
    return false;
  }
  Eigen::Affine3d eigen_transform = msg_to_eigen(transform.transform);

  Eigen::Affine3d pose_transform = Eigen::Translation3d(msg_to_eigen(pose.position)) * msg_to_eigen(pose.orientation);
  Eigen::Affine3d transformed_pose_affine = eigen_transform * pose_transform;

  const Eigen::Vector3d& transformed_position = transformed_pose_affine.translation();
  Eigen::Quaterniond transformed_orientation(transformed_pose_affine.rotation());

  transformed_pose.position.x = transformed_position.x();
  transformed_pose.position.y = transformed_position.y();
  transformed_pose.position.z = transformed_position.z();
  transformed_pose.orientation = eigen_to_msg(transformed_orientation);

  return true;
}

bool TransformListener::transform_quaternion(const std::string& target_frame,
                                             const std::string& source_frame,
                                             const Time& time,
                                             const geometry_msgs::Quaternion& quaternion,
                                             geometry_msgs::Quaternion& transformed_quaternion) const {
  geometry_msgs::TransformStamped transform;
  if (!graph_.get_transform(target_frame, source_frame, time, transform)) {
    return false;
  }
  Eigen::Affine3d eigen_transform = msg_to_eigen(transform.transform);

  Eigen::Quaterniond eigen_quaternion = msg_to_eigen(quaternion);
  Eigen::Quaterniond transformed_eigen_quaternion(eigen_transform.rotation() * eigen_quaternion);
  transformed_quaternion = eigen_to_msg(transformed_eigen_quaternion);
  return true;
}

bool TransformListener::transform_vector(const std::string& target_frame,
                                         const std::string& source_frame,
                                         const Time& time,
                                         const geometry_msgs::Vector3& vector,
                                         geometry_msgs::Vector3& transformed_vector) const {
  geometry_msgs::TransformStamped transform;
  if (!graph_.get_transform(target_frame, source_frame, time, transform)) {
    return false;
  }
  Eigen::Affine3d eigen_transform = msg_to_eigen(transform.transform);

  Eigen::Vector3d eigen_vector = msg_to_eigen(vector);
  Eigen::Vector3d transformed_eigen_vector = eigen_transform.linear() * eigen_vector;
  transformed_vector = eigen_to_msg(transformed_eigen_vector);
  return true;
}

bool TransformListener::ok() const { return subscriber_->ok(); }

const FrameGraph& TransformListener::graph() const { return graph_; }

Duration TransformListener::duration() const { return duration_; }

void TransformListener::set_duration(const Duration& duration) { duration_ = duration; }

} // namespace rix