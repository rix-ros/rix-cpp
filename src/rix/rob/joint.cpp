#include "rix/rob/joint.hpp"

#include <eigen3/Eigen/Geometry>
#include <utility>

#include "rix/rob/eigen_util.hpp"
#include "rix/util/log.hpp"

namespace rix {

Joint::Joint(const geometry_msgs::Vector3& axis,
             const geometry_msgs::Transform& origin,
             const Joint::Type& type,
             const JointLimits& limits,
             const JointDynamics& dynamics,
             JointMimic mimic,
             std::string name,
             std::string parent,
             std::string child)
    : position_(0.0), velocity_(0.0), effort_(0.0), type_(type), limits_(limits), dynamics_(dynamics),
      mimic_(std::move(mimic)), name_(std::move(name)), parent_(std::move(parent)), child_(std::move(child)),
      axis_(axis), origin_(origin) {}

bool Joint::in_bounds(double position) const { return position >= limits_.lower && position <= limits_.upper; }

double Joint::clamp(double position) const {
  if (type_ == Type::CONTINUOUS) {
    // Wrap to (-pi, pi] without clamping to joint limits
    position = std::fmod(position + M_PI, 2 * M_PI);
    if (position < 0) {
      position += 2 * M_PI;
    }
    position -= M_PI;
    return position;
  }
  // Clamp for revolute or prismatic joints
  if (position < limits_.lower) {
    return limits_.lower;
  } else if (position > limits_.upper) {
    return limits_.upper;
  }
  return position;
}

const std::string& Joint::name() const { return name_; }
const std::string& Joint::parent() const { return parent_; }
const std::string& Joint::child() const { return child_; }
Joint::Type Joint::type() const { return type_; }
const JointLimits& Joint::limits() const { return limits_; }
const JointDynamics& Joint::dynamics() const { return dynamics_; }

bool Joint::is_mimic() const { return !mimic_.name.empty(); }
const JointMimic& Joint::mimic() const { return mimic_; }

double Joint::position() const {
  if (is_mimic()) {
    auto parent_state = mimic_.joint->get_state();
    return parent_state.position * mimic_.multiplier + mimic_.offset;
  }
  return position_;
}
double Joint::velocity() const {
  if (is_mimic()) {
    auto parent_state = mimic_.joint->get_state();
    return parent_state.velocity * mimic_.multiplier + mimic_.offset;
  }
  return velocity_;
}
double Joint::effort() const {
  if (is_mimic()) {
    auto parent_state = mimic_.joint->get_state();
    return parent_state.velocity * mimic_.multiplier + mimic_.offset;
  }
  return velocity_;
}

const geometry_msgs::Vector3& Joint::axis() const { return axis_; }
const geometry_msgs::Transform& Joint::origin() const { return origin_; }

geometry_msgs::Transform Joint::transform() const {
  Eigen::Affine3d T = Eigen::Affine3d::Identity();
  Eigen::Vector3d axis = msg_to_eigen(axis_);

  switch (type_) {
  case Joint::Type::PRISMATIC:
    T.translate(position_ * axis);
    break;
  case Joint::Type::CONTINUOUS:
  case Joint::Type::REVOLUTE:
    T = T * Eigen::AngleAxisd(position_, axis);
    break;
  case Joint::Type::FIXED:
  case Joint::Type::UNKNOWN:
    break;
  }
  return eigen_to_msg(T);
}

sensor_msgs::JointState Joint::get_state() const {
  sensor_msgs::JointState js;
  js.name = name_;
  if (is_mimic()) {
    auto parent_state = mimic_.joint->get_state();
    js.position = parent_state.position * mimic_.multiplier + mimic_.offset;
    js.velocity = parent_state.velocity * mimic_.multiplier + mimic_.offset;
    js.effort = parent_state.effort * mimic_.multiplier + mimic_.offset;
    return js;
  }
  js.position = position_;
  js.velocity = velocity_;
  js.effort = effort_;
  return js;
}

void Joint::set_state(double position, double velocity, double effort) {
  if (is_mimic()) {
    Log::warn << "Cannot set state of mimic joint: \"" << name_ << "\"!";
    return;
  }
  position_ = position;
  velocity_ = velocity;
  effort_ = effort;
}

void Joint::set_state(const sensor_msgs::JointState& joint_state) {
  if (is_mimic()) {
    Log::warn << "Cannot set state of mimic joint: \"" << name_ << "\"!";
    return;
  }
  position_ = joint_state.position;
  velocity_ = joint_state.velocity;
  effort_ = joint_state.effort;
}

} // namespace rix