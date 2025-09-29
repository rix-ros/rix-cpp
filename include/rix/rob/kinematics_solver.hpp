#pragma once

#include "rix/msg/geometry/Pose.hpp"
#include "rix/rob/eigen_util.hpp"
#include "rix/rob/robot_model.hpp"

namespace rix::rob {

class KinematicsSolver {
public:
  KinematicsSolver(std::shared_ptr<RobotModel> robot, double step_scale = 0.25, double tolerance = 1e-5,
                   uint32_t max_iterations = 1000);

  bool solve_ik(const std::string &link_name, const rix::msg::geometry::Transform &goal,
                rix::msg::sensor::JS initial_guess, rix::msg::sensor::JS &solution);

  bool solve_ik(const std::string &link_name, const rix::msg::geometry::Transform &goal,
                rix::msg::sensor::JS initial_guess, std::vector<rix::msg::sensor::JS> &solution);

  rix::msg::geometry::Transform solve_fk(const std::string &link_name) const;

  Eigen::MatrixXd get_jacobian(const std::vector<std::shared_ptr<Joint>> &chain, Eigen::Affine3d &ee_transform);

  inline double get_step_scale() const { return step_scale_; }
  inline double get_tolerance() const { return tolerance_; }
  inline uint32_t get_max_iterations() const { return max_iterations_; }

  inline void set_step_scale(double step_scale) { step_scale_ = step_scale; }
  inline void set_tolerance(double tolerance) { tolerance_ = tolerance; }
  inline void set_max_iterations(uint32_t max_iterations) { max_iterations_ = max_iterations; }

private:
  std::shared_ptr<RobotModel> robot_;
  double step_scale_;
  double tolerance_;
  uint32_t max_iterations_;

  bool iterate_ik(const std::vector<std::shared_ptr<Joint>> &chain, const std::string &link_name,
                  const Eigen::Affine3d &goal);
};

} // namespace rix::rob