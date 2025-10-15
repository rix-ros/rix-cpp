#include "rix/rob/kinematics_solver.hpp"

#include <gtest/gtest.h>

#include <random>

#include "rix/rob/robot_model.hpp"
#include "robots.hpp"

double get_random(double lower, double upper) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(lower, upper);  // [lower, upper)
    return dist(gen);
}

void test_solve_random(std::shared_ptr<rix::robot::RobotModel> robot, const std::string &ee, size_t iterations = 1000) {
    auto solver = std::make_shared<rix::robot::KinematicsSolver>(robot, 0.5, 1e-3, 500);

    std::vector<rix::msg::sensor::JS> initial_guesses(iterations);
    std::vector<rix::msg::geometry::Transform> goals(iterations);
    auto chain = robot->get_joints_in_chain(ee);
    for (size_t i = 0; i < iterations; ++i) {
        initial_guesses[i].joint_states.resize(chain.size());
        std::vector<double> target_positions(chain.size());
        for (size_t j = 0; j < chain.size(); ++j) {
            auto &joint = chain[j];
            double p2;
            if (joint->limits().lower == 0 && joint->limits().upper == 0) {
                p2 = get_random(-M_PI, M_PI);
            } else if (joint->limits().lower < -1000 || joint->limits().upper > 1000) {
                p2 = 0;
            } else {
                p2 = get_random(joint->limits().lower, joint->limits().upper);
            }
            target_positions[j] = p2;
            joint->set_state(p2, 0, 0);
        }
        goals[i] = solver->solve_fk(ee);
        // Now generate initial guess by adding small random noise to target position
        for (size_t j = 0; j < chain.size(); ++j) {
            double noise = get_random(-0.05, 0.05); // 0.05 rad or meters noise
            initial_guesses[i].joint_states[j].position = target_positions[j] + noise;
            initial_guesses[i].joint_states[j].velocity = 0;
            initial_guesses[i].joint_states[j].effort = 0;
        }
    }

    size_t success = 0;
    rix::Time start = rix::Time::now();
    for (size_t i = 0; i < iterations; ++i) {
        rix::msg::sensor::JS sol;
        if (solver->solve_ik(ee, goals[i], initial_guesses[i], sol)) {
            success++;

            robot->set_state(sol);
            auto eigen_goal = rix::robot::msg_to_eigen(goals[i]);
            Eigen::Affine3d ee_transform = rix::robot::msg_to_eigen(solver->solve_fk(ee));
            Eigen::Vector3d goal_translation = eigen_goal.translation();
            Eigen::Vector3d ee_translation = ee_transform.translation();
            Eigen::AngleAxisd rotation_error =
                Eigen::AngleAxisd(eigen_goal.rotation() * ee_transform.rotation().transpose());

            Eigen::VectorXd dp = Eigen::VectorXd::Zero(6);
            dp.head<3>() = goal_translation - ee_translation;
            dp.tail<3>() = rotation_error.axis() * rotation_error.angle();

            double error = dp.norm();
            EXPECT_LT(error, 0.001);
        }
    }
    rix::Duration d = rix::Time::now() - start;

    double success_rate = (double)success / iterations;
    double iter_duration = ((double)d.to_nanoseconds() / 1e6) / iterations;
    std::cout << "Success rate: " << success_rate << std::endl;
    std::cout << iter_duration << " ms/iter" << std::endl;
}

TEST(IKSolver, TestRandomSimpleBot) {
    test_solve_random(std::make_shared<rix::robot::RobotModel>(rix::robot::RobotModel::from_json(SIMPLEBOT_JRDF)),
                      "tool");
}

TEST(IKSolver, TestRandomRx200) {
    test_solve_random(std::make_shared<rix::robot::RobotModel>(rix::robot::RobotModel::from_json(RX200_JRDF)),
                      "/gripper_link");
}

TEST(IKSolver, TestRandomFetch) {
    test_solve_random(std::make_shared<rix::robot::RobotModel>(rix::robot::RobotModel::from_json(FETCH_JRDF)),
                      "gripper_link");
}
