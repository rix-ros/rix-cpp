#include "rix/rob/eigen_util.hpp"

namespace rix {

msg::geometry::Transform eigen_to_msg(const Eigen::Affine3d& T) {
  msg::geometry::Transform t;
  const Eigen::Vector3d&   translation = T.translation();
  t.translation.x = translation.x();
  t.translation.y = translation.y();
  t.translation.z = translation.z();

  Eigen::Quaterniond q(T.linear());
  t.rotation.w = q.w();
  t.rotation.x = q.x();
  t.rotation.y = q.y();
  t.rotation.z = q.z();
  return t;
}

msg::geometry::Vector3 eigen_to_msg(const Eigen::Vector3d& v) {
  msg::geometry::Vector3 vec;
  vec.x = v.x();
  vec.y = v.y();
  vec.z = v.z();
  return vec;
}

msg::geometry::Quaternion eigen_to_msg(const Eigen::Quaterniond& q) {
  msg::geometry::Quaternion quat;
  quat.w = q.w();
  quat.x = q.x();
  quat.y = q.y();
  quat.z = q.z();
  return quat;
}

Eigen::Affine3d msg_to_eigen(const msg::geometry::Transform& t) {
  Eigen::Affine3d T(Eigen::Affine3d::Identity());
  T.translation() = Eigen::Vector3d(t.translation.x, t.translation.y, t.translation.z);
  Eigen::Quaterniond q(t.rotation.w, t.rotation.x, t.rotation.y, t.rotation.z);
  T.linear() = q.toRotationMatrix();
  return T;
}

Eigen::Vector3d msg_to_eigen(const msg::geometry::Vector3& v) {
  return Eigen::Vector3d(v.x, v.y, v.z);
}

Eigen::Vector3d msg_to_eigen(const msg::geometry::Point& p) {
  return Eigen::Vector3d(p.x, p.y, p.z);
}

Eigen::Quaterniond msg_to_eigen(const msg::geometry::Quaternion& q) {
  return Eigen::Quaterniond(q.w, q.x, q.y, q.z);
}

Eigen::Affine3d
interpolate(const Eigen::Affine3d& t1, const Eigen::Affine3d& t2, double ratio) {
  Eigen::Affine3d t;
  t.translation() = interpolate(t1.translation(), t2.translation(), ratio);
  t.linear() =
      interpolate(Eigen::Quaterniond(t1.linear()), Eigen::Quaterniond(t2.linear()), ratio)
          .toRotationMatrix();
  return t;
}

Eigen::Quaterniond
interpolate(const Eigen::Quaterniond& q1, const Eigen::Quaterniond& q2, double ratio) {
  return q1.slerp(ratio, q2);
}

Eigen::Vector3d
interpolate(const Eigen::Vector3d& v1, const Eigen::Vector3d& v2, double ratio) {
  return v1 + ratio * (v2 - v1);
}

msg::geometry::Transform interpolate(const msg::geometry::Transform& t1,
                                     const msg::geometry::Transform& t2,
                                     double                          ratio) {
  return eigen_to_msg(interpolate(msg_to_eigen(t1), msg_to_eigen(t2), ratio));
}

msg::geometry::Quaternion interpolate(const msg::geometry::Quaternion& q1,
                                      const msg::geometry::Quaternion& q2,
                                      double                           ratio) {
  return eigen_to_msg(interpolate(msg_to_eigen(q1), msg_to_eigen(q2), ratio));
}

msg::geometry::Vector3 interpolate(const msg::geometry::Vector3& v1,
                                   const msg::geometry::Vector3& v2,
                                   double                        ratio) {
  return eigen_to_msg(interpolate(msg_to_eigen(v1), msg_to_eigen(v2), ratio));
}

} // namespace rix