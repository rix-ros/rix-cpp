#pragma once

#include <iostream>
#include <memory>

#include "rix/geometry_msgs/Transform.hpp"
#include "rix/geometry_msgs/Vector3.hpp"
#include "rix/sensor_msgs/JointState.hpp"
#include "rix/rob/msg_util.hpp"

namespace rix {

class JointDynamics {
public:
  JointDynamics() {};
  double damping{};
  double friction{};
};

class JointLimits {
public:
  JointLimits() {};
  double lower{};
  double upper{};
  double effort{};
  double velocity{};
};

class Joint; // Forward declaration

class JointMimic {
public:
  JointMimic() {};
  double offset{};
  double multiplier{};
  std::string name{};
  mutable std::shared_ptr<Joint> joint{};
};

class Joint {
public:
  enum Type { UNKNOWN, FIXED, CONTINUOUS, REVOLUTE, PRISMATIC };

  explicit Joint(const geometry_msgs::Vector3& axis = vector3_zeros(),
                 const geometry_msgs::Transform& origin = transform_identity(),
                 const Type& type = FIXED,
                 const JointLimits& limits = {},
                 const JointDynamics& dynamics = {},
                 JointMimic mimic = {},
                 std::string name = "",
                 std::string parent = "",
                 std::string child = "");
  Joint(const Joint& j) = default;
  Joint& operator=(const Joint& j) = default;

  bool in_bounds(double position) const;
  double clamp(double position) const;

  const std::string& name() const;
  const std::string& parent() const;
  const std::string& child() const;
  Type type() const;
  const JointLimits& limits() const;
  const JointDynamics& dynamics() const;

  bool is_mimic() const;
  const JointMimic& mimic() const;

  double position() const; // rad or m
  double velocity() const; // rad/s or m/s
  double effort() const;   // Nm or N
  sensor_msgs::JointState get_state() const;
  void set_state(double position, double velocity, double effort);
  void set_state(const sensor_msgs::JointState& js);

  const geometry_msgs::Vector3& axis() const;     // Axis of rotation or translation
  const geometry_msgs::Transform& origin() const; // Transform from parent link frame to joint frame
  geometry_msgs::Transform transform() const;     // Transform based from joint frame to child link frame

private:
  double position_; // The current position (rad or m)
  double velocity_; // The current velocity (rad/s or m/s)
  double effort_;   // The current effort (Nm or N)

  Type type_;                       // The type of the joint (Fixed, Continuous, Revolute, or Prismatic)
  JointLimits limits_;              // The limits on the joint
  JointDynamics dynamics_;          // The dynamics of the joint
  JointMimic mimic_;                // The mimic parameters of the joint
  std::string name_;                // The name of the joint
  std::string parent_;              // The name of the joint's parent link
  std::string child_;               // The name of the joint's child link
  geometry_msgs::Vector3 axis_;     // The unit vector along the actuation axis in the parent link frame
  geometry_msgs::Transform origin_; // The transform from parent frame to child frame when the position is 0
};

} // namespace rix