#pragma once

#include "rix/geometry_msgs/Pose.hpp"
#include "rix/rob/eigen_util.hpp"
#include "rix/rob/robot_model.hpp"

namespace rix {

class KinematicsSolver {
public:
  explicit KinematicsSolver(std::shared_ptr<RobotModel> robot,
                            double step_scale = 0.25,
                            double tolerance = 1e-5,
                            uint32_t max_iterations = 1000);

  bool solve_ik(const std::string& link_name,
                const geometry_msgs::Transform& goal,
                sensor_msgs::JS initial_guess,
                sensor_msgs::JS& solution) const;

  bool solve_ik(const std::string& link_name,
                const geometry_msgs::Transform& goal,
                sensor_msgs::JS initial_guess,
                std::vector<sensor_msgs::JS>& solution) const;

  geometry_msgs::Transform solve_fk(const std::string& link_name) const;

  static Eigen::MatrixXd get_jacobian(const std::vector<std::shared_ptr<Joint>>& chain, Eigen::Affine3d& ee_transform);

  double get_step_scale() const { return step_scale_; }
  double get_tolerance() const { return tolerance_; }
  uint32_t get_max_iterations() const { return max_iterations_; }

  void set_step_scale(const double step_scale) { step_scale_ = step_scale; }
  void set_tolerance(const double tolerance) { tolerance_ = tolerance; }
  void set_max_iterations(const uint32_t max_iterations) { max_iterations_ = max_iterations; }

private:
  std::shared_ptr<RobotModel> robot_;
  double step_scale_;
  double tolerance_;
  uint32_t max_iterations_;

  bool iterate_ik(const std::vector<std::shared_ptr<Joint>>& chain,
                  const std::string& link_name,
                  const Eigen::Affine3d& goal) const;
};

} // namespace rix