#pragma once

#include "rix/geometry_msgs/Inertia.hpp"
#include "rix/geometry_msgs/Transform.hpp"
#include "rix/geometry_msgs/Vector3.hpp"

namespace rix {

inline geometry_msgs::Vector3 vector3_zeros() {
  geometry_msgs::Vector3 vec;
  vec.x = 0.0;
  vec.y = 0.0;
  vec.z = 0.0;
  return vec;
}

inline geometry_msgs::Transform transform_identity() {
  geometry_msgs::Transform t;
  t.translation = vector3_zeros();
  t.rotation.w = 1.0;
  t.rotation.x = 0.0;
  t.rotation.y = 0.0;
  t.rotation.z = 0.0;
  return t;
}

inline geometry_msgs::Inertia default_inertia() {
  geometry_msgs::Inertia i;
  i.mass = 0;
  i.center_of_mass = vector3_zeros();
  i.ixx = 1;
  i.iyy = 1;
  i.izz = 1;
  i.ixy = 0;
  i.ixz = 0;
  i.iyz = 0;
  return i;
}

} // namespace rix