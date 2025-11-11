#pragma once

#include "rix/geometry_msgs/Point.hpp"
#include "rix/geometry_msgs/Transform.hpp"
#include "rix/geometry_msgs/Vector3.hpp"
#include <eigen3/Eigen/Geometry>

namespace rix {

geometry_msgs::Transform eigen_to_msg(const Eigen::Affine3d& T);
geometry_msgs::Vector3 eigen_to_msg(const Eigen::Vector3d& v);
geometry_msgs::Quaternion eigen_to_msg(const Eigen::Quaterniond& v);

Eigen::Affine3d msg_to_eigen(const geometry_msgs::Transform& T);
Eigen::Vector3d msg_to_eigen(const geometry_msgs::Vector3& v);
Eigen::Vector3d msg_to_eigen(const geometry_msgs::Point& p);
Eigen::Quaterniond msg_to_eigen(const geometry_msgs::Quaternion& v);

Eigen::Affine3d interpolate(const Eigen::Affine3d& t1, const Eigen::Affine3d& t2, double ratio);
Eigen::Quaterniond interpolate(const Eigen::Quaterniond& q1, const Eigen::Quaterniond& q2, double ratio);
Eigen::Vector3d interpolate(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, double ratio);

geometry_msgs::Transform
interpolate(const geometry_msgs::Transform& t1, const geometry_msgs::Transform& t2, double ratio);
geometry_msgs::Quaternion
interpolate(const geometry_msgs::Quaternion& q1, const geometry_msgs::Quaternion& q2, double ratio);
geometry_msgs::Vector3 interpolate(const geometry_msgs::Vector3& v1, const geometry_msgs::Vector3& v2, double ratio);

} // namespace rix