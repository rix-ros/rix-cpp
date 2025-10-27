#pragma once

#include "rix/msg/geometry/Point.hpp"
#include "rix/msg/geometry/Transform.hpp"
#include "rix/msg/geometry/Vector3.hpp"
#include <eigen3/Eigen/Geometry>

namespace rix {

msg::geometry::Transform eigen_to_msg(const Eigen::Affine3d& T);
msg::geometry::Vector3 eigen_to_msg(const Eigen::Vector3d& v);
msg::geometry::Quaternion eigen_to_msg(const Eigen::Quaterniond& v);

Eigen::Affine3d msg_to_eigen(const msg::geometry::Transform& T);
Eigen::Vector3d msg_to_eigen(const msg::geometry::Vector3& v);
Eigen::Vector3d msg_to_eigen(const msg::geometry::Point& p);
Eigen::Quaterniond msg_to_eigen(const msg::geometry::Quaternion& v);

Eigen::Affine3d interpolate(const Eigen::Affine3d& t1, const Eigen::Affine3d& t2, double ratio);
Eigen::Quaterniond interpolate(const Eigen::Quaterniond& q1, const Eigen::Quaterniond& q2, double ratio);
Eigen::Vector3d interpolate(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, double ratio);

msg::geometry::Transform
interpolate(const msg::geometry::Transform& t1, const msg::geometry::Transform& t2, double ratio);
msg::geometry::Quaternion
interpolate(const msg::geometry::Quaternion& q1, const msg::geometry::Quaternion& q2, double ratio);
msg::geometry::Vector3 interpolate(const msg::geometry::Vector3& v1, const msg::geometry::Vector3& v2, double ratio);

} // namespace rix