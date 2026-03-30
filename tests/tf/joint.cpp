#include <gtest/gtest.h>

#include "rix/rob/eigen_util.hpp"
#include "rix/rob/robot_model.hpp"

using namespace rix;
using namespace rix;

// Compare two doubles with tolerance
inline bool almost_equal_scalar(double a, double b, double tol = 1e-6) { return std::fabs(a - b) <= tol; }

// Compare Vector3
inline bool
almost_equal(const rix::geometry_msgs::Vector3& a, const rix::geometry_msgs::Vector3& b, double tol = 1e-6) {
  return almost_equal_scalar(a.x, b.x, tol) && almost_equal_scalar(a.y, b.y, tol) && almost_equal_scalar(a.z, b.z, tol);
}

// Compare Quaternion (account for double-cover: q and -q are equivalent)
inline bool
almost_equal(const rix::geometry_msgs::Quaternion& a, const rix::geometry_msgs::Quaternion& b, double tol = 1e-6) {
  bool direct = almost_equal_scalar(a.w, b.w, tol) && almost_equal_scalar(a.x, b.x, tol) &&
                almost_equal_scalar(a.y, b.y, tol) && almost_equal_scalar(a.z, b.z, tol);

  bool negated = almost_equal_scalar(a.w, -b.w, tol) && almost_equal_scalar(a.x, -b.x, tol) &&
                 almost_equal_scalar(a.y, -b.y, tol) && almost_equal_scalar(a.z, -b.z, tol);

  return direct || negated;
}

// Compare Transform
inline bool
almost_equal(const rix::geometry_msgs::Transform& a, const rix::geometry_msgs::Transform& b, double tol = 1e-6) {
  return almost_equal(a.translation, b.translation, tol) && almost_equal(a.rotation, b.rotation, tol);
}

rix::geometry_msgs::Transform rotation_about_z(double angle) {
  rix::geometry_msgs::Transform t = transform_identity();
  t.rotation = eigen_to_msg(Eigen::Quaterniond(Eigen::AngleAxisd(angle, Eigen::Vector3d({0, 0, 1}))));
  return t;
}

rix::geometry_msgs::Transform translation_along_x(double dist) {
  rix::geometry_msgs::Transform t = transform_identity();
  t.translation.x = dist;
  t.translation.y = 0;
  t.translation.z = 0;
  return t;
}

// --- FIXED JOINT ---
TEST(JointTransformTest, FixedJointIdentity) {
  Joint j(vector3_zeros(), transform_identity(), Joint::FIXED);
  j.set_state(0.5, 0.0, 0.0); // Position shouldn't matter

  rix::geometry_msgs::Transform tf = j.transform();
  EXPECT_TRUE(almost_equal(tf, transform_identity()));
}

// --- REVOLUTE JOINT ---
TEST(JointTransformTest, RevoluteJointRotation) {
  rix::geometry_msgs::Vector3 axis;
  axis.x = 0;
  axis.y = 0;
  axis.z = 1;
  Joint j(axis, transform_identity(), Joint::REVOLUTE);
  j.set_state(M_PI / 2, 0.0, 0.0);

  rix::geometry_msgs::Transform tf = j.transform();
  rix::geometry_msgs::Transform expected = rotation_about_z(M_PI / 2);

  EXPECT_TRUE(almost_equal(tf, expected, 1e-6));
}

// --- CONTINUOUS JOINT ---
TEST(JointTransformTest, ContinuousJointRotation) {
  rix::geometry_msgs::Vector3 axis;
  axis.x = 0;
  axis.y = 0;
  axis.z = 1;
  Joint j(axis, transform_identity(), Joint::CONTINUOUS);
  j.set_state(M_PI / 4, 0.0, 0.0);

  rix::geometry_msgs::Transform tf = j.transform();
  rix::geometry_msgs::Transform expected = rotation_about_z(M_PI / 4);

  EXPECT_TRUE(almost_equal(tf, expected, 1e-6));
}

// --- CONTINUOUS JOINT WITH WRAP ---
TEST(JointTransformTest, ContinuousJointRotationWrap) {
  rix::geometry_msgs::Vector3 axis;
  axis.x = 0;
  axis.y = 0;
  axis.z = 1;
  Joint j(axis, transform_identity(), Joint::CONTINUOUS);
  j.set_state(2 * M_PI, 0.0, 0.0);

  rix::geometry_msgs::Transform tf = j.transform();
  rix::geometry_msgs::Transform expected = transform_identity(); // full rotation should wrap

  EXPECT_TRUE(almost_equal(tf, expected, 1e-6));
}

// --- PRISMATIC JOINT ---
TEST(JointTransformTest, PrismaticJointTranslation) {
  rix::geometry_msgs::Vector3 axis;
  axis.x = 1;
  axis.y = 0;
  axis.z = 0;
  Joint j(axis, transform_identity(), Joint::PRISMATIC);
  j.set_state(2.0, 0.0, 0.0);

  rix::geometry_msgs::Transform tf = j.transform();
  rix::geometry_msgs::Transform expected = translation_along_x(2.0);

  EXPECT_TRUE(almost_equal(tf, expected, 1e-6));
}

// --- ENSURE NO ORIGIN OFFSET ---
TEST(JointTransformTest, OriginOffsetIsNotApplied) {
  rix::geometry_msgs::Vector3 axis;
  axis.x = 0;
  axis.y = 0;
  axis.z = 1;
  rix::geometry_msgs::Transform origin = translation_along_x(1.0);
  Joint j(axis, origin, Joint::REVOLUTE);
  j.set_state(M_PI / 2, 0.0, 0.0);

  rix::geometry_msgs::Transform tf = j.transform();
  rix::geometry_msgs::Transform expected = rotation_about_z(M_PI / 2);

  EXPECT_TRUE(almost_equal(tf, expected, 1e-6));
}
