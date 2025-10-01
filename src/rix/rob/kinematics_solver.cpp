#include "rix/rob/kinematics_solver.hpp"

#include <eigen3/Eigen/Geometry>
#include <random>

#include "rix/rob/eigen_util.hpp"

namespace rix::rob {

double get_random(double lower, double upper) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(lower, upper);  // [lower, upper)
    return dist(gen);
}

KinematicsSolver::KinematicsSolver(std::shared_ptr<RobotModel> robot, double step_scale, double tolerance,
                                   uint32_t max_iterations)
    : robot_(robot), step_scale_(step_scale), tolerance_(tolerance), max_iterations_(max_iterations) {}

bool KinematicsSolver::solve_ik(const std::string &link_name, const rix::msg::geometry::Transform &goal,
                                rix::msg::sensor::JS initial_guess, rix::msg::sensor::JS &solution) {
    auto chain = robot_->get_joints_in_chain(link_name);
    if (initial_guess.joint_states.empty()) {
        for (auto j : chain) {
            double p;
            if (j->limits().lower == 0 && j->limits().upper == 0) {
                p = get_random(-M_PI, M_PI);
            } else if (j->limits().lower < -1000 || j->limits().upper > 1000) {
                p = 0;
            } else {
                p = get_random(j->limits().lower, j->limits().upper);
            }

            rix::msg::sensor::JointState joint_state;
            joint_state.name = j->name();
            joint_state.position = p;
            joint_state.velocity = 0;
            joint_state.effort = 0;
            initial_guess.joint_states.push_back(joint_state);
        }
    }

    robot_->set_state(initial_guess);

    // The gradient descent loop
    Eigen::Affine3d goal_eigen = msg_to_eigen(goal);
    bool converged = false;
    for (size_t i = 0; i < max_iterations_; i++) {
        converged = iterate_ik(chain, link_name, goal_eigen);
        if (converged) break;
    }

    if (converged) {
        // std::cout << "Converged!" << std::endl;
        solution.joint_states.clear();
        solution.joint_states.reserve(chain.size());
        for (auto &j : chain) {
            solution.joint_states.push_back(j->get_state());
            // std::cout << "Found " << j->name() << " as " << j->position() << std::endl;
        }
        // std::cout << std::endl;
    }
    return converged;
}

bool KinematicsSolver::solve_ik(const std::string &link_name, const rix::msg::geometry::Transform &goal,
                                rix::msg::sensor::JS initial_guess, std::vector<rix::msg::sensor::JS> &solution) {
    auto chain = robot_->get_joints_in_chain(link_name);
    if (initial_guess.joint_states.empty()) {
        for (auto j : chain) {
            double p;
            if (j->limits().lower == 0 && j->limits().upper == 0) {
                p = get_random(-M_PI, M_PI);
            } else if (j->limits().lower < -1000 || j->limits().upper > 1000) {
                p = 0;
            } else {
                p = get_random(j->limits().lower, j->limits().upper);
            }

            rix::msg::sensor::JointState joint_state;
            joint_state.name = j->name();
            joint_state.position = p;
            joint_state.velocity = 0;
            joint_state.effort = 0;
            initial_guess.joint_states.push_back(joint_state);
        }
    }

    robot_->set_state(initial_guess);

    // The gradient descent loop
    Eigen::Affine3d goal_eigen = msg_to_eigen(goal);
    bool converged = false;
    for (size_t i = 0; i < max_iterations_; i++) {
        converged = iterate_ik(chain, link_name, goal_eigen);

        rix::msg::sensor::JS intermediate_solution;
        intermediate_solution.joint_states.reserve(chain.size());
        for (auto &j : chain) {
            intermediate_solution.joint_states.push_back(j->get_state());
        }
        solution.push_back(intermediate_solution);
        if (converged) break;
    }
    return converged;
}

rix::msg::geometry::Transform KinematicsSolver::solve_fk(const std::string &link) const {
    auto chain = robot_->get_joints_in_chain(link);
    Eigen::Affine3d transform = Eigen::Affine3d::Identity();
    for (auto j : chain) {
        auto O_J = msg_to_eigen(j->origin());
        auto X = msg_to_eigen(j->transform());
        transform = transform * O_J * X;
    }
    return eigen_to_msg(transform);
}

Eigen::MatrixXd KinematicsSolver::get_jacobian(const std::vector<std::shared_ptr<Joint>> &chain,
                                               Eigen::Affine3d &ee_transform) {
    // Initialize 6 x N matrix
    Eigen::MatrixXd J;
    J.resize(6, chain.size());

    // Get transforms to base frame for each joint
    std::vector<Eigen::Affine3d> Tj;
    Tj.reserve(chain.size());
    Eigen::Affine3d transform = Eigen::Affine3d::Identity();
    for (auto j : chain) {
        auto O_J = msg_to_eigen(j->origin());
        auto X = msg_to_eigen(j->transform());
        transform = transform * O_J * X;
        Tj.push_back(transform);
    }

    // The last transform is the end-effector
    ee_transform = Tj.back();
    Eigen::Vector3d ol = ee_transform.translation();  // link origin

    // Assemble Jacobian
    for (size_t i = 0; i < chain.size(); ++i) {
        auto &j = chain[i];
        Eigen::Vector3d oj = Tj[i].translation();
        Eigen::Vector3d zj = Tj[i].linear() * msg_to_eigen(j->axis());

        // Initialize the column vector properly using Eigen
        J.col(i) = Eigen::VectorXd::Zero(6);

        switch (j->type()) {
            case Joint::Type::FIXED:
                // Fixed joints contribute nothing to the Jacobian
                break;
            case Joint::Type::REVOLUTE:
            case Joint::Type::CONTINUOUS: {
                // For revolute/continuous joints, compute angular and linear components
                J.col(i).head<3>() = zj.cross(ol - oj);  // Linear velocity component
                J.col(i).tail<3>() = zj;                 // Angular velocity component
                break;
            }
            case Joint::Type::PRISMATIC: {
                // For prismatic joints, compute linear component only
                J.col(i).head<3>() = zj;                       // Linear velocity component
                J.col(i).tail<3>() = Eigen::Vector3d::Zero();  // No angular velocity
                break;
            }
            default:
                break;
        }
    }
    return J;
}

bool KinematicsSolver::iterate_ik(const std::vector<std::shared_ptr<Joint>> &chain, const std::string &link_name,
                                  const Eigen::Affine3d &goal) {
    // Get the end effector transform during Jacobian calculation so we don't have to do it twice (small speed-up)
    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd J = get_jacobian(chain, ee_transform);

    // Compute the error vector dp
    Eigen::VectorXd dp = Eigen::VectorXd::Zero(6);
    dp.head<3>() = goal.translation() - ee_transform.translation();  // Linear error

    Eigen::AngleAxisd rotation_error(goal.rotation() * ee_transform.rotation().transpose());
    dp.tail<3>() = rotation_error.axis() * rotation_error.angle();  // Angular error

    // If the norm of our error is below the tolerance, return true (converged)
    double error = dp.norm();
    if (error < tolerance_) {
        return true;
    }

    // Perform gradient descent
    Eigen::MatrixXd Jt = J.transpose();
    Eigen::MatrixXd Jinv;

    // Moore-Penrose pseudo-inverse
    if (J.cols() == J.rows()) {
        Jinv = J.inverse();
    } else if (J.cols() < J.rows()) {
        Eigen::MatrixXd JtJ = Jt * J;
        Jinv = JtJ.inverse() * Jt;
    } else {
        Eigen::MatrixXd JJt = J * Jt;
        Jinv = Jt * JJt.inverse();
    }

    // Damped Least Squares (Levenberg-Marquardt)
    // int m = J.rows();
    // int n = J.cols();
    // if (m >= n) {
    //     // Overdetermined or square
    //     Jinv = (Jt * J + 1e-4 * Eigen::MatrixXd::Identity(n, n)).ldlt().solve(Jt);
    // } else {
    //     // Underdetermined
    //     Jinv = Jt * (J * Jt + 1e-4 * Eigen::MatrixXd::Identity(m, m)).ldlt().solve(Eigen::MatrixXd::Identity(m, m));
    // }

    // Singular Value Decomposition
    // Eigen::JacobiSVD<Eigen::MatrixXd> svd(J, Eigen::ComputeThinU | Eigen::ComputeThinV);
    // const auto& singularValues = svd.singularValues();
    // Eigen::MatrixXd S_inv = Eigen::MatrixXd::Zero(svd.matrixV().cols(), svd.matrixU().cols());
    // for (int i = 0; i < singularValues.size(); ++i) {
    //     if (singularValues(i) > 1e-3) {
    //         S_inv(i, i) = 1.0 / singularValues(i);
    //     }
    // }
    // Jinv = svd.matrixV() * S_inv * svd.matrixU().transpose();

    Eigen::VectorXd dq = Jinv * dp;

    for (size_t k = 0; k < dq.rows(); k++) {
        auto &j = chain[k];
        double new_pos = j->clamp(j->position() + step_scale_ * dq(k));
        j->set_state(new_pos, 0.0, 0.0);
    }
    return false;
}

}  // namespace rix::rob