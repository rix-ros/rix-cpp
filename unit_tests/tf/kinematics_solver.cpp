#include "rix/rob/kinematics_solver.hpp"

#include <gtest/gtest.h>

#include <iomanip>
#include <random>

#include "rix/rob/robot_model.hpp"
#include "robots.hpp"

using rix::msg::geometry::Transform;
using rix::msg::sensor::JS;
using namespace rix::rob;

void print_joint_state(const JS &js, const std::string &label) {
    std::cout << label << ":\n";
    for (const auto &joint : js.joint_states) {
        std::cout << "\t" << joint.name << ": " << joint.position << "\n";
    }
    std::cout << std::endl;
}

void test_ik_solve(std::shared_ptr<RobotModel> robot, const std::string &ee, const Transform &goal, const JS &q_init) {
    auto solver = std::make_shared<KinematicsSolver>(robot, 0.5, 1e-4, 100);

    robot->set_state(q_init);
    JS result;
    bool status = solver->solve_ik(ee, goal, q_init, result);
    EXPECT_TRUE(status);

    // Check if result is equal to goal
    robot->set_state(result);
    auto eigen_goal = msg_to_eigen(goal);
    Eigen::Affine3d ee_transform = msg_to_eigen(solver->solve_fk(ee));
    Eigen::Vector3d goal_translation = eigen_goal.translation();
    Eigen::Vector3d ee_translation = ee_transform.translation();
    Eigen::AngleAxisd rotation_error = Eigen::AngleAxisd(eigen_goal.rotation() * ee_transform.rotation().transpose());

    Eigen::VectorXd dp = Eigen::VectorXd::Zero(6);
    dp.head<3>() = goal_translation - ee_translation;
    dp.tail<3>() = rotation_error.axis() * rotation_error.angle();

    double error = dp.norm();
    EXPECT_LT(error, 1e-4);
}

TEST(KinematicsSolver, SolveFK_SimpleBot_Default) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(SIMPLEBOT_JRDF));
    KinematicsSolver solver(robot);

    auto fk = solver.solve_fk("tool");

    Eigen::Affine3d eigen_fk = msg_to_eigen(fk);
    Eigen::MatrixXd sol(4, 4);
    sol << 1, 0, 0, 0.3375,
           0, 1, 0,      0,
           0, 0, 1,    0.3,
           0, 0, 0,      1;
    EXPECT_TRUE(sol.isApprox(eigen_fk.matrix(), 1e-6));
}

TEST(KinematicsSolver, SolveFK_RX200_Default) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(RX200_JRDF));
    KinematicsSolver solver(robot);

    auto fk = solver.solve_fk("/gripper_link");
    Eigen::Affine3d eigen_fk = msg_to_eigen(fk);
    Eigen::MatrixXd sol(4, 4);
    sol << -1,  0, 0,  -0.315,
            0, -1, 0,       0,
            0,  0, 1, 0.30391,
            0,  0, 0,       1;
    EXPECT_TRUE(sol.isApprox(eigen_fk.matrix(), 1e-6));
}

TEST(KinematicsSolver, SolveFK_Fetch_Default) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(FETCH_JRDF));
    KinematicsSolver solver(robot);

    auto fk = solver.solve_fk("gripper_link");
    Eigen::Affine3d eigen_fk = msg_to_eigen(fk);
    Eigen::MatrixXd sol(4, 4);
    sol << 1, -0, 0,  1.1281,
           0,  1, 0,       0,
          -0,  0, 1, 0.78601,
           0,  0, 0,       1;
    EXPECT_TRUE(sol.isApprox(eigen_fk.matrix(), 1e-6));
}

TEST(KinematicsSolver, SolveFK_SimpleBot) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(SIMPLEBOT_JRDF));
    KinematicsSolver solver(robot);

    JS state;
    state.joint_states.resize(4);
    state.joint_states[0].name = "waist";
    state.joint_states[0].position = 0.1;
    state.joint_states[1].name = "shoulder";
    state.joint_states[1].position = 0.2;
    state.joint_states[2].name = "elbow";
    state.joint_states[2].position = -0.3;
    state.joint_states[3].name = "wrist";
    state.joint_states[3].position = 0.4;
    robot->set_state(state);

    auto fk = solver.solve_fk("tool");

    Eigen::Affine3d eigen_fk = msg_to_eigen(fk);
    Eigen::MatrixXd sol(4, 4);
    sol << 0.990033,  -0.0998334,  -0.0993347,    0.291417,
          0.0993347,    0.995004, -0.00996671,    0.431248,
          0.0998334,           0,    0.995004,    0.270249,
                  0,           0,           0,           1;
    EXPECT_TRUE(sol.isApprox(eigen_fk.matrix(), 1e-6));
}

TEST(KinematicsSolver, SolveFK_RX200) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(RX200_JRDF));
    KinematicsSolver solver(robot);

    JS state;
    state.joint_states.resize(5);
    state.joint_states[0].name = "waist";
    state.joint_states[0].position = 0.5;
    state.joint_states[1].name = "shoulder";
    state.joint_states[1].position = -0.5;
    state.joint_states[2].name = "elbow";
    state.joint_states[2].position = 0.7;
    state.joint_states[3].name = "wrist_angle";
    state.joint_states[3].position = -0.2;
    state.joint_states[4].name = "wrist_rotate";
    state.joint_states[4].position = 1.0;
    robot->set_state(state);

    auto fk = solver.solve_fk("/gripper_link");
    Eigen::Affine3d eigen_fk = msg_to_eigen(fk);
    Eigen::MatrixXd sol(4, 4);
    sol <<    -0.877583, 0.259035, -0.403423, -0.351715,
              -0.479426, -0.47416,   0.73846, -0.192143,
           -2.25009e-08, 0.841471,  0.540302,  0.295189,
                      0,        0,         0,         1;
    EXPECT_TRUE(sol.isApprox(eigen_fk.matrix(), 1e-6));
}

TEST(KinematicsSolver, SolveFK_Fetch) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(FETCH_JRDF));
    KinematicsSolver solver(robot);

    JS state;
    state.joint_states.resize(9);
    state.joint_states[0].name = "torso_lift_joint";
    state.joint_states[0].position = 0.25;
    state.joint_states[1].name = "shoulder_pan_joint";
    state.joint_states[1].position = -0.5;
    state.joint_states[2].name = "shoulder_lift_joint";
    state.joint_states[2].position = 0.7;
    state.joint_states[3].name = "upperarm_roll_joint";
    state.joint_states[3].position = -0.2;
    state.joint_states[4].name = "elbow_flex_joint";
    state.joint_states[4].position = 1.0;
    state.joint_states[5].name = "forearm_roll_joint";
    state.joint_states[5].position = -0.3;
    state.joint_states[6].name = "wrist_flex_joint";
    state.joint_states[6].position = 1.2;
    state.joint_states[7].name = "wrist_roll_joint";
    state.joint_states[7].position = 0.7;
    state.joint_states[8].name = "gripper_axis";
    state.joint_states[8].position = 0; // Fixed joint
    robot->set_state(state);

    auto fk = solver.solve_fk("gripper_link");
    Eigen::Affine3d eigen_fk = msg_to_eigen(fk);
    Eigen::MatrixXd sol(4, 4);
    sol <<    -0.980358,  0.172838, 0.0950045, 0.0135625,
              0.0500323,  0.683882, -0.727875, -0.198882,
              -0.190777, -0.708825, -0.679096,  0.436372,
                      0,         0,         0,         1;
    EXPECT_TRUE(sol.isApprox(eigen_fk.matrix(), 1e-6));
}

TEST(KinematicsSolver, GetJacobian_SimpleBot_Default) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(SIMPLEBOT_JRDF));
    KinematicsSolver solver(robot);

    auto chain = robot->get_joints_in_chain("tool");
    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd jacobian = solver.get_jacobian(chain, ee_transform);

    Eigen::MatrixXd fk_sol(4, 4);
    fk_sol << 1, 0, 0, 0.3375,
              0, 1, 0,      0,
              0, 0, 1,    0.3,
              0, 0, 0,      1;
    EXPECT_TRUE(fk_sol.isApprox(ee_transform.matrix(), 1e-6));

    // Jacobian should have 6 rows and N columns (N = number of joints in chain)
    EXPECT_EQ(jacobian.rows(), 6);
    EXPECT_EQ(jacobian.cols(), chain.size());

    Eigen::MatrixXd sol(6, chain.size());
    sol <<      0,    0,    0, 0,
           0.3375,    0,    0, 1,
                0, -0.3, -0.1, 0,
                0,    0,    0, 0,
                0,    1,    1, 0,
                1,    0,    0, 0;
    EXPECT_TRUE(sol.isApprox(jacobian, 1e-6));
}

TEST(KinematicsSolver, GetJacobian_RX200_Default) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(RX200_JRDF));
    KinematicsSolver solver(robot);
    
    auto chain = robot->get_joints_in_chain("/gripper_link");
    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd jacobian = solver.get_jacobian(chain, ee_transform);

    Eigen::MatrixXd fk_sol(4, 4);
    fk_sol << -1,  0, 0,  -0.315,
               0, -1, 0,       0,
               0,  0, 1, 0.30391,
               0,  0, 0,       1;
    EXPECT_TRUE(fk_sol.isApprox(ee_transform.matrix(), 1e-6));
    
    // Jacobian should have 6 rows and N columns (N = number of joints in chain)
    EXPECT_EQ(jacobian.rows(), 6);
    EXPECT_EQ(jacobian.cols(), chain.size());

    Eigen::MatrixXd sol(6, chain.size());
    sol <<      0,   0.2,     0,     0,   0,
           -0.315,     0,     0,     0,   0,
                0, 0.315, 0.265, 0.065,  -0,
                0,     0,     0,     0,  -1,
                0,     1,     1,     1,   0,
                1,     0,     0,     0,   0;
    EXPECT_TRUE(sol.isApprox(jacobian, 1e-6));
}

TEST(KinematicsSolver, GetJacobian_Fetch_Default) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(FETCH_JRDF));
    KinematicsSolver solver(robot);
    
    auto chain = robot->get_joints_in_chain("gripper_link");
    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd jacobian = solver.get_jacobian(chain, ee_transform);
    
    Eigen::MatrixXd fk_sol(4, 4);
    fk_sol << 1, -0, 0,  1.1281,
              0,  1, 0,       0,
             -0,  0, 1, 0.78601,
              0,  0, 0,       1;
    EXPECT_TRUE(fk_sol.isApprox(ee_transform.matrix(), 1e-6));

    // Jacobian should have 6 rows and N columns (N = number of joints in chain)
    EXPECT_EQ(jacobian.rows(), 6);
    EXPECT_EQ(jacobian.cols(), chain.size());
    Eigen::MatrixXd sol(6, chain.size());
    sol << 0,       0,        0, 0,        0, 0,        0, 0, 0,
           0, 1.09545,        0, 0,        0, 0,        0, 0, 0,
           1,       0, -0.97845, 0, -0.62645, 0, -0.30495, 0, 0,
           0,       0,        0, 1,        0, 1,        0, 1, 0,
           0,       0,        1, 0,        1, 0,        1, 0, 0,
           0,       1,        0, 0,        0, 0,        0, 0, 0;

    EXPECT_TRUE(sol.isApprox(jacobian, 1e-6));
}

TEST(KinematicsSolver, GetJacobian_SimpleBot) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(SIMPLEBOT_JRDF));
    KinematicsSolver solver(robot);

    JS state;
    state.joint_states.resize(4);
    state.joint_states[0].name = "waist";
    state.joint_states[0].position = 0.1;
    state.joint_states[1].name = "shoulder";
    state.joint_states[1].position = 0.2;
    state.joint_states[2].name = "elbow";
    state.joint_states[2].position = -0.3;
    state.joint_states[3].name = "wrist";
    state.joint_states[3].position = 0.4;
    robot->set_state(state);

    auto chain = robot->get_joints_in_chain("tool");
    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd jacobian = solver.get_jacobian(chain, ee_transform);

    Eigen::MatrixXd fk_sol(4, 4);
    fk_sol <<  0.990033,  -0.0998334,  -0.0993347,    0.291417,
              0.0993347,    0.995004, -0.00996671,    0.431248,
              0.0998334,           0,    0.995004,    0.270249,
                      0,           0,           0,           1;
    EXPECT_TRUE(fk_sol.isApprox(ee_transform.matrix(), 1e-6));

    // Jacobian should have 6 rows and N columns (N = number of joints in chain)
    EXPECT_EQ(jacobian.rows(), 6);
    EXPECT_EQ(jacobian.cols(), chain.size());

    Eigen::MatrixXd sol(6, chain.size());
    sol << -0.431248,  -0.0296019,  0.00993347,  -0.0998334,
            0.291417,  -0.0029701, 0.000996671,    0.995004,
                   0,   -0.295514,  -0.0995004,           0,
                   0,  -0.0998334,  -0.0998334,           0,
                   0,    0.995004,    0.995004,           0,
                   1,           0,           0,           0;
    EXPECT_TRUE(sol.isApprox(jacobian, 1e-6));
}

TEST(KinematicsSolver, GetJacobian_RX200) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(RX200_JRDF));
    KinematicsSolver solver(robot);

    JS state;
    state.joint_states.resize(5);
    state.joint_states[0].name = "waist";
    state.joint_states[0].position = 0.5;
    state.joint_states[1].name = "shoulder";
    state.joint_states[1].position = -0.5;
    state.joint_states[2].name = "elbow";
    state.joint_states[2].position = 0.7;
    state.joint_states[3].name = "wrist_angle";
    state.joint_states[3].position = -0.2;
    state.joint_states[4].name = "wrist_rotate";
    state.joint_states[4].position = 1.0;
    robot->set_state(state);
    
    auto chain = robot->get_joints_in_chain("/gripper_link");
    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd jacobian = solver.get_jacobian(chain, ee_transform);

    Eigen::MatrixXd fk_sol(4, 4);
    fk_sol <<    -0.877583, 0.259035, -0.403423, -0.351715,
                 -0.479426, -0.47416,   0.73846, -0.192143,
              -2.25009e-08, 0.841471,  0.540302,  0.295189,
                         0,        0,         0,         1;
    EXPECT_TRUE(fk_sol.isApprox(ee_transform.matrix(), 1e-6));
    
    // Jacobian should have 6 rows and N columns (N = number of joints in chain)
    EXPECT_EQ(jacobian.rows(), 6);
    EXPECT_EQ(jacobian.cols(), chain.size());

    Eigen::MatrixXd sol(6, chain.size());
    sol <<  0.192143,     0.167863,    0.0348697,            0,            0,
           -0.351715,    0.0917041,    0.0190494,            0,            0,
                   0,     0.400778,     0.261013,        0.065,            0,
                   0,    -0.479426,    -0.479426,    -0.479426,    -0.877583,
                   0,     0.877583,     0.877583,     0.877583,    -0.479426,
                   1,            0,            0,            0,            0;
    EXPECT_TRUE(sol.isApprox(jacobian, 1e-6));
}

TEST(KinematicsSolver, GetJacobian_Fetch) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(FETCH_JRDF));
    KinematicsSolver solver(robot);

    JS state;
    state.joint_states.resize(9);
    state.joint_states[0].name = "torso_lift_joint";
    state.joint_states[0].position = 0.25;
    state.joint_states[1].name = "shoulder_pan_joint";
    state.joint_states[1].position = -0.5;
    state.joint_states[2].name = "shoulder_lift_joint";
    state.joint_states[2].position = 0.7;
    state.joint_states[3].name = "upperarm_roll_joint";
    state.joint_states[3].position = -0.2;
    state.joint_states[4].name = "elbow_flex_joint";
    state.joint_states[4].position = 1.0;
    state.joint_states[5].name = "forearm_roll_joint";
    state.joint_states[5].position = -0.3;
    state.joint_states[6].name = "wrist_flex_joint";
    state.joint_states[6].position = 1.2;
    state.joint_states[7].name = "wrist_roll_joint";
    state.joint_states[7].position = 0.7;
    state.joint_states[8].name = "gripper_axis";
    state.joint_states[8].position = 0; // Fixed joint
    robot->set_state(state);
    
    auto chain = robot->get_joints_in_chain("gripper_link");

    Eigen::Affine3d ee_transform;
    Eigen::MatrixXd jacobian = solver.get_jacobian(chain, ee_transform);

    Eigen::MatrixXd fk_sol(4, 4);
    fk_sol << -0.980358,  0.172838, 0.0950045, 0.0135625,
              0.0500323,  0.683882, -0.727875, -0.198882,
              -0.190777, -0.708825, -0.679096,  0.436372,
                      0,         0,         0,         1;
    EXPECT_TRUE(fk_sol.isApprox(ee_transform.matrix(), 1e-6));
    
    // Jacobian should have 6 rows and N columns (N = number of joints in chain)
    EXPECT_EQ(jacobian.rows(), 6);
    EXPECT_EQ(jacobian.cols(), chain.size());
    Eigen::MatrixXd sol(6, chain.size());
    sol << 0,   0.198882, -0.526232,  0.127891, -0.345668,  0.0201773, -0.0561136,         0, 0,
           0, -0.0190875,  0.287482,  0.480927,  0.187724,   0.281943,  0.0354173,         0, 0,
           1,          0, 0.0384019, -0.140491,  0.325004, -0.0297452,   0.297643,         0, 0,
           0,          0,  0.479426,  0.671212,   0.35755,  -0.183736,  0.0709905, -0.980358, 0,
           0,          0,  0.877583, -0.366685,  0.921449, -0.0901186,   0.991972, 0.0500323, 0,
           0,          1,         0, -0.644218, -0.151951,  -0.978836,  -0.104654, -0.190777, 0;

    EXPECT_TRUE(sol.isApprox(jacobian, 1e-6));
}


TEST(KinematicsSolver, SimpleBot_Examples_IK) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(SIMPLEBOT_JRDF));
    std::vector<std::pair<std::vector<std::pair<std::string, double>>, Eigen::Matrix4d>>
        cases = {{
                    {{"waist", -0.00851973}, {"shoulder", 0.234836}, {"elbow", -0.0237233}, {"wrist", 0.0240176}},
                    (Eigen::Matrix4d() <<
                         0.958927,  -0.0863283,   0.270197,    0.324406,
                        0.0830928,    0.996267,   0.023413,   0.0328809,
                        -0.271209, 5.95148e-10,    0.96252,    0.217853,
                                0,           0,          0,           1).finished()            
                 },
                 {
                    {{"waist", -0.876576}, {"shoulder", 1.09555}, {"elbow", -1.32566}, {"wrist", -0.072574}},
                    (Eigen::Matrix4d() <<
                         0.604409,     0.782409, -0.150089,    0.143823,
                        -0.759346,     0.622765,  0.188564,   -0.171414,
                         0.241004,  1.49438e-09,  0.970524,    0.145059,
                                0,            0,         0,           1).finished()            
                 },
                 {
                    {{"waist", -1.13097}, {"shoulder", 0.0602358}, {"elbow", -0.638475}, {"wrist", 0.0843991}},
                    (Eigen::Matrix4d() <<
                         0.435182,      0.86174, -0.260806,    0.172908,
                        -0.739163,      0.50735,  0.442983,   -0.273249,
                         0.514056, -1.66613e-09,  0.857756,    0.345714,
                                0,            0,         0,           1).finished()            
                 },
                 {
                    {{"waist", 0.763125}, {"shoulder", 1.05778}, {"elbow", -0.0360139}, {"wrist", -0.0276642}},
                    (Eigen::Matrix4d() <<
                         0.327245,    -0.756508,  0.566221,    0.114147,
                         0.378547,     0.653984,  0.654986,    0.137304,
                        -0.865802,  2.24406e-09,  0.500387,   0.0353128,
                                0,            0,         0,           1).finished()            
                 },
                 {
                    {{"waist", -2.3405}, {"shoulder", 1.27085}, {"elbow", -0.700231}, {"wrist", -0.0292202}},
                    (Eigen::Matrix4d() <<
                         -0.60932,     0.685203, -0.399031,   -0.136892,
                        -0.573223,    -0.728352, -0.375392,   -0.137821,
                        -0.547855, -1.61194e-08,  0.836573,   0.0590067,
                                0,            0,         0,           1).finished()
                    }
                 };
    for (const auto &c : cases) {
        JS q_init;
        q_init.joint_states.resize(c.first.size());
        for (size_t i = 0; i < c.first.size(); ++i) {
            q_init.joint_states[i].name = c.first[i].first;
            q_init.joint_states[i].position = c.first[i].second;
        }
        Transform goal = eigen_to_msg(Eigen::Affine3d(c.second));
        test_ik_solve(robot, "tool", goal, q_init);
    }
}

TEST(KinematicsSolver, RX200_Examples_IK) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(RX200_JRDF));
    std::vector<std::pair<std::vector<std::pair<std::string, double>>, Eigen::Matrix4d>>
        cases = {{{{"waist", 2.79152},
                   {"shoulder", -1.35681},
                   {"elbow", -0.416538},
                   {"wrist_angle", -0.00660815},
                   {"wrist_rotate", 1.54007}},
                 (Eigen::Matrix4d() <<
                     -0.244979,   0.950819,  -0.189549,   0.135299,
                     0.0690338,  -0.177904,  -0.981623, -0.0381266,
                     -0.967068,  -0.253562,  -0.022056,  -0.157679,
                             0,          0,          0,          1).finished()
                  },
                 {{{"waist", -1.19709},
                   {"shoulder", -0.598191},
                   {"elbow", 1.3987},
                   {"wrist_angle", -0.849349},
                   {"wrist_rotate", 0.375566}},
                 (Eigen::Matrix4d() <<
                     -0.455498,  -0.806989,   0.375886,  -0.162256,
                      0.888692,  -0.387326,   0.245366,   0.316567,
                    -0.0524169,   0.445811,   0.893591,   0.383042,
                             0,          0,          0,          1).finished()
                  },
                 {{{"waist", 0.564334},
                   {"shoulder", -1.85544},
                   {"elbow", 1.09125},
                   {"wrist_angle", 0.668327},
                   {"wrist_rotate", -1.14885}},
                 (Eigen::Matrix4d() <<
                      -0.86808,   0.332554,   0.368572,  -0.347441,
                     -0.479441,  -0.369089,  -0.796184,  -0.191892,
                     -0.128739,   -0.86786,   0.479839,  -0.132789,
                             0,          0,          0,          1).finished()
                  },
                 {{{"waist", -0.579734},
                   {"shoulder", 0.776433},
                   {"elbow", -1.02289},
                   {"wrist_angle", 0.0105127},
                   {"wrist_rotate", -0.01554}},
                 (Eigen::Matrix4d() <<
                     -0.764169,  -0.584967,  -0.271771,  -0.120588,
                      0.548661,  -0.811035,    0.20296,  0.0865805,
                      -0.33914, 0.00598567,   0.940717,   0.192128,
                             0,          0,          0,          1).finished()
                  },
                 {{{"waist", 1.88344},
                   {"shoulder", 0.307632},
                   {"elbow", -0.239317},
                   {"wrist_angle", -1.79176},
                   {"wrist_rotate", 1.13031}},
                 (Eigen::Matrix4d() <<
                   -0.0906899,   0.637917,  -0.764746,    0.06057,
                     0.244249,  -0.730209,  -0.638073,  -0.163129,
                    -0.965462,  -0.244655,  -0.089588,    0.24138,
                            0,          0,          0,          1).finished()
                  }};
    for (const auto &c : cases) {
        JS q_init;
        q_init.joint_states.resize(c.first.size());
        for (size_t i = 0; i < c.first.size(); ++i) {
            q_init.joint_states[i].name = c.first[i].first;
            q_init.joint_states[i].position = c.first[i].second;
        }
        Transform goal = eigen_to_msg(Eigen::Affine3d(c.second));
        test_ik_solve(robot, "/gripper_link", goal, q_init);
    }
}

TEST(KinematicsSolver, Fetch_Examples_IK) {
    auto robot = std::make_shared<RobotModel>(RobotModel::from_json(FETCH_JRDF));
    std::vector<std::pair<std::vector<std::pair<std::string, double>>, Eigen::Matrix4d>>
        cases = {{{{"torso_lift_joint", 0.271159},
                   {"shoulder_pan_joint", 0.0153619},
                   {"shoulder_lift_joint", 1.22109},
                   {"upperarm_roll_joint", 1.13923},
                   {"elbow_flex_joint", -2.01748},
                   {"forearm_roll_joint", -0.616621},
                   {"wrist_flex_joint", -0.233911},
                   {"wrist_roll_joint", -3.01906},
                   {"gripper_axis", -0.086187}},
                 (Eigen::Matrix4d() <<
                      0.378114,  -0.907304,   0.183924,  0.504556,
                     -0.584353,  -0.388007,  -0.712729, -0.420819,
                      0.718026,   0.162016,  -0.676897,   1.16568,
                             0,          0,          0,         1).finished()
                  },
                 {{{"torso_lift_joint", 0.292493},
                   {"shoulder_pan_joint", 1.14194},
                   {"shoulder_lift_joint", 0.115859},
                   {"upperarm_roll_joint", 1.58023},
                   {"elbow_flex_joint", -2.11817},
                   {"forearm_roll_joint", 0.293174},
                   {"wrist_flex_joint", -0.0965631},
                   {"wrist_roll_joint", 1.66085},
                   {"gripper_axis", -0.0663085}},
                 (Eigen::Matrix4d() <<
                      0.548694,  -0.819966,   0.163063,  0.600062,
                     -0.835988,  -0.539932,  0.0979694,  -0.11457,
                    0.00771158,  -0.190074,  -0.981739,   1.02145,
                             0,          0,          0,         1).finished()
                  },
                 {{{"torso_lift_joint", 0.0398074},
                   {"shoulder_pan_joint", -0.899875},
                   {"shoulder_lift_joint", 0.992555},
                   {"upperarm_roll_joint", 1.36401},
                   {"elbow_flex_joint", 0.530538},
                   {"forearm_roll_joint", -2.14509},
                   {"wrist_flex_joint", -1.71346},
                   {"wrist_roll_joint", 2.52178},
                   {"gripper_axis", -0.0873979}},
                 (Eigen::Matrix4d() <<
                      0.764896,  -0.388212,    0.51403,  0.627277,
                     0.0691685,  -0.743878,  -0.664726, -0.236125,
                       0.64043,   0.544001,  -0.542137,  0.543858,
                             0,          0,          0,         1).finished()
                  },
                 {{{"torso_lift_joint", 0.376251},
                   {"shoulder_pan_joint", 1.30391},
                   {"shoulder_lift_joint", -0.463121},
                   {"upperarm_roll_joint", 2.98598},
                   {"elbow_flex_joint", 0.425505},
                   {"forearm_roll_joint", -3.08149},
                   {"wrist_flex_joint", 1.86472},
                   {"wrist_roll_joint", 0.199162},
                   {"gripper_axis", 0.0155281}},
                 (Eigen::Matrix4d() <<
                      0.260804,  -0.907784,   0.328495,  0.218688,
                      0.603931,   0.418884,   0.678088,  0.763406,
                     -0.753159,  0.0215397,   0.657485,   1.31891,
                             0,          0,          0,         1).finished()
                  },
                 {{{"torso_lift_joint", 0.0721504},
                   {"shoulder_pan_joint", -0.255437},
                   {"shoulder_lift_joint", -1.09229},
                   {"upperarm_roll_joint", 1.69525},
                   {"elbow_flex_joint", -0.253466},
                   {"forearm_roll_joint", -2.71933},
                   {"wrist_flex_joint", 0.395761},
                   {"wrist_roll_joint", 0.904808},
                   {"gripper_axis", 0.0417266}},
                 (Eigen::Matrix4d() <<
                      0.428673,   0.717385,  -0.549179,   0.60186,
                     -0.664888,   0.662055,   0.345842, -0.420099,
                      0.611689,   0.216889,   0.760786,   1.53369,
                             0,          0,          0,         1).finished()
                  }};
    for (const auto &c : cases) {
        JS q_init;
        q_init.joint_states.resize(c.first.size());
        for (size_t i = 0; i < c.first.size(); ++i) {
            q_init.joint_states[i].name = c.first[i].first;
            q_init.joint_states[i].position = c.first[i].second;
        }
        Transform goal = eigen_to_msg(Eigen::Affine3d(c.second));
        test_ik_solve(robot, "gripper_link", goal, q_init);
    }
}