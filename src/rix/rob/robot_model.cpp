#include "rix/rob/robot_model.hpp"

#include "rix/util/environment.hpp"
#include <eigen3/Eigen/Geometry>
#include <nlohmann/json.hpp>
#include <stack>

#include "rix/rob/eigen_util.hpp"

using Json = nlohmann::json;

namespace rix {

namespace detail {

bool convert_json_sphere(const Json& src, Sphere& dst);
bool convert_json_box(const Json& src, Box& dst);
bool convert_json_cylinder(const Json& src, Cylinder& dst);
bool convert_json_mesh(const Json& src, Mesh& dst);
bool convert_json_material(const Json& src, Material& dst);
bool convert_json_origin(const Json& src, geometry_msgs::Transform& dst);
bool convert_json_inertial(const Json& src, Inertial& dst);
bool convert_json_geometry(const Json& src, std::shared_ptr<Geometry>& dst);
bool convert_json_visual(const Json& src, Visual& dst);
bool convert_json_collision(const Json& src, Collision& dst);
bool convert_json_joint_dynamics(const Json& src, JointDynamics& dst);
bool convert_json_joint_limits(const Json& src, JointLimits& dst);
std::shared_ptr<Joint> convert_json_joint(const Json& src);
std::shared_ptr<Link> convert_json_link(const Json& src,
                                        const std::map<std::string, std::string>& parent_map,
                                        const std::map<std::string, std::vector<std::string>>& children_map);
void parse_jrdf(Json& json,
                std::map<std::string, std::shared_ptr<Joint>>& joints,
                std::map<std::string, std::shared_ptr<Link>>& links,
                std::string& root);

} // namespace detail

RobotModel RobotModel::from_model(const std::string& name) {
  std::string jrdf_path = get_env("HOME", std::string());
  if (jrdf_path.empty())
    return RobotModel();
  if (jrdf_path[jrdf_path.size() - 1] != '/') {
    jrdf_path.push_back('/');
  }
  jrdf_path.append(".rix/jrdf/models/" + name + "/model.json");
  return RobotModel(jrdf_path);
}

RobotModel RobotModel::from_json(const std::string& json_str) {
  RobotModel robot;
  Json json = Json::parse(json_str);
  detail::parse_jrdf(json, robot.joints, robot.links, robot.root);
  return robot;
}

RobotModel::RobotModel() : root(""), world_to_root(transform_identity()) {}

RobotModel::RobotModel(const std::string& file_path) : world_to_root(transform_identity()) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    return;
  }
  Json json = Json::parse(file);
  detail::parse_jrdf(json, joints, links, root);
}

RobotModel::RobotModel(const RobotModel& other)
    : root(other.root), joints(other.joints), links(other.links), world_to_root(other.world_to_root) {}

RobotModel& RobotModel::operator=(const RobotModel& other) {
  if (this != &other) {
    joints = other.joints;
    links = other.links;
    root = other.root;
    world_to_root = other.world_to_root;
    return *this;
  }
  return *this;
}

bool RobotModel::is_valid() const { return !joints.empty() && !links.empty() && !root.empty(); }

size_t RobotModel::get_joint_count() const { return joints.size(); }
size_t RobotModel::get_link_count() const { return links.size(); }

std::vector<std::string> RobotModel::get_joint_names() const {
  std::vector<std::string> names;
  for (auto j : joints) {
    names.push_back(j.second->name());
  }
  return names;
}

std::vector<std::string> RobotModel::get_link_names() const {
  std::vector<std::string> names;
  for (auto l : links) {
    names.push_back(l.second->name());
  }
  return names;
}

bool RobotModel::has_joint(const std::string& name) const { return joints.find(name) != joints.end(); }
bool RobotModel::has_link(const std::string& name) const { return links.find(name) != links.end(); }

std::shared_ptr<Joint> RobotModel::get_joint(const std::string& name) const {
  auto it = joints.find(name);
  if (it == joints.end()) {
    return nullptr;
  }
  return it->second;
}

std::shared_ptr<Link> RobotModel::get_link(const std::string& name) const {
  auto it = links.find(name);
  if (it == links.end()) {
    return nullptr;
  }
  return it->second;
}

std::shared_ptr<Link> RobotModel::get_root() const { return links.at(root); }

std::vector<std::shared_ptr<Link>> RobotModel::get_end_effectors() const {
  std::vector<std::shared_ptr<Link>> end_effectors;
  for (const auto& pair : links) {
    if (pair.second->is_end_effector()) {
      end_effectors.push_back(pair.second);
    }
  }
  return end_effectors;
}

std::vector<std::shared_ptr<Joint>> RobotModel::get_joints_in_chain(const std::string& link_name) const {
  auto it = links.find(link_name);
  if (it == links.end()) {
    return {};
  }

  auto link = it->second;
  std::vector<std::shared_ptr<Joint>> joint_list;
  while (!link->is_root()) {
    auto j = joints.at(link->parent());
    joint_list.push_back(j);
    link = links.at(j->parent());
  }
  std::reverse(joint_list.begin(), joint_list.end());
  return joint_list;
}

geometry_msgs::TF RobotModel::get_transforms() const {
  geometry_msgs::TF tf;

  std_msgs::Time stamp = Time::now().to_msg();

  tf.transforms.resize(joints.size() + 1);

  tf.transforms[0].header.stamp = stamp;
  tf.transforms[0].header.seq = 0;
  tf.transforms[0].header.frame_id = "world";
  tf.transforms[0].child_frame_id = root;
  tf.transforms[0].transform = world_to_root;

  size_t index = 1;
  std::stack<std::shared_ptr<Link>> link_stack;
  link_stack.push(get_root());
  while (!link_stack.empty()) {
    auto link = link_stack.top();
    link_stack.pop();

    for (const std::string& child : link->children()) {
      std::shared_ptr<Joint> child_joint = joints.at(child);
      const std::string& grandchild_name = child_joint->child();

      tf.transforms[index].header.stamp = stamp;
      tf.transforms[index].header.seq = index;
      tf.transforms[index].header.frame_id = link->name();
      tf.transforms[index].child_frame_id = grandchild_name;

      const Eigen::Affine3d& O_J = msg_to_eigen(child_joint->origin());
      const Eigen::Affine3d& X = msg_to_eigen(child_joint->transform());
      tf.transforms[index].transform = eigen_to_msg(O_J * X);

      index++;

      link_stack.push(links.at(grandchild_name));
    }
  }
  return tf;
}

sensor_msgs::JS RobotModel::get_joint_states() const {
  sensor_msgs::JS js;
  js.stamp = Time::now().to_msg();
  for (auto j : joints) {
    if (j.second->type() == Joint::Type::FIXED) {
      continue;
    }
    js.joint_states.push_back(j.second->get_state());
  }
  return js;
}

geometry_msgs::TF RobotModel::get_static_transforms() const {
  geometry_msgs::TF tf;
  geometry_msgs::TransformStamped transform;
  transform.header.stamp = Time::now().to_msg();
  transform.header.seq = 0;

  for (const auto& pair : links) {
    const std::string& link_name = pair.first;
    std::shared_ptr<Link> link = pair.second;

    transform.header.frame_id = link_name;
    transform.child_frame_id = link_name + "/inertial";
    transform.transform = link->inertial().origin;
    tf.transforms.push_back(transform);
    transform.header.seq++;

    size_t visual_index = 0;
    for (const auto& visual : link->visuals()) {
      transform.child_frame_id = link_name + "/visual/" + std::to_string(visual_index);
      transform.transform = visual.origin;
      tf.transforms.push_back(transform);
      transform.header.seq++;
      visual_index++;
    }

    size_t collision_index = 0;
    for (const auto& collision : link->collisions()) {
      transform.child_frame_id = link_name + "/collision/" + std::to_string(collision_index);
      transform.transform = collision.origin;
      tf.transforms.push_back(transform);
      transform.header.seq++;
      collision_index++;
    }
  }
  return tf;
}

void RobotModel::set_state(const sensor_msgs::JS& js) {
  for (const auto& joint_state : js.joint_states) {
    set_state(joint_state);
  }
}

void RobotModel::set_state(const sensor_msgs::JointState& js) {
  set_state(js.name, js.position, js.velocity, js.effort);
}

void RobotModel::set_state(const std::string& joint_name, double position, double velocity, double effort) {
  auto it = joints.find(joint_name);
  if (it == joints.end()) {
    return;
  }
  it->second->set_state(position, velocity, effort);
}

void RobotModel::set_world_to_root(const geometry_msgs::Transform& transform) { world_to_root = transform; }

const geometry_msgs::Transform& RobotModel::get_world_to_root() const { return world_to_root; }

namespace detail {

bool convert_json_sphere(const Json& src, Sphere& dst) {
  if (!src.is_object())
    return false;
  if (!src.contains("radius"))
    return false;
  if (!src.at("radius").is_number())
    return false;
  double radius = src.at("radius").get<double>();
  dst.radius = radius;
  return true;
}

bool convert_json_box(const Json& src, Box& dst) {
  if (!src.is_object())
    return false;
  if (!src.contains("size"))
    return false;
  if (!src.at("size").is_array())
    return false;
  if (src.at("size").size() != 3)
    return false;
  if (!src.at("size")[0].is_number())
    return false;
  if (!src.at("size")[1].is_number())
    return false;
  if (!src.at("size")[2].is_number())
    return false;

  dst.dim.x = src.at("size")[0].get<double>();
  dst.dim.y = src.at("size")[1].get<double>();
  dst.dim.z = src.at("size")[2].get<double>();
  return true;
}

bool convert_json_cylinder(const Json& src, Cylinder& dst) {
  if (!src.is_object())
    return false;
  if (!src.contains("radius"))
    return false;
  if (!src.at("radius").is_number())
    return false;
  if (!src.contains("length"))
    return false;
  if (!src.at("length").is_number())
    return false;
  dst.radius = src.at("radius").get<double>();
  dst.length = src.at("length").get<double>();
  return true;
}

bool convert_json_mesh(const Json& src, Mesh& dst) {
  if (!src.is_object())
    return false;
  if (!src.contains("filename"))
    return false;
  if (!src.at("filename").is_string())
    return false;
  dst.filename = src.at("filename").get<std::string>();

  dst.scale.x = dst.scale.y = dst.scale.z = 1;
  if (src.contains("scale")) {
    if (!src.at("scale").is_array())
      return false;
    if (src.at("scale").size() != 3)
      return false;
    if (!src.at("scale")[0].is_number())
      return false;
    if (!src.at("scale")[1].is_number())
      return false;
    if (!src.at("scale")[2].is_number())
      return false;
    dst.scale.x = src.at("scale")[0].get<double>();
    dst.scale.y = src.at("scale")[1].get<double>();
    dst.scale.z = src.at("scale")[2].get<double>();
  }
  return true;
}

bool convert_json_material(const Json& src, Material& dst) {
  if (!src.is_object())
    return false;

  if (src.contains("texture_filename")) {
    if (!src.at("texture_filename").is_string())
      return false;
    dst.texture_filename = src.at("texture_filename").get<std::string>();
  } else if (src.contains("color")) {
    if (!src.at("color").is_array())
      return false;
    if (src.at("color").size() != 4)
      return false;
    if (!src.at("color")[0].is_number())
      return false;
    if (!src.at("color")[1].is_number())
      return false;
    if (!src.at("color")[2].is_number())
      return false;
    if (!src.at("color")[3].is_number())
      return false;
    dst.color.r = src.at("color")[0].get<float>();
    dst.color.g = src.at("color")[1].get<float>();
    dst.color.b = src.at("color")[2].get<float>();
    dst.color.a = src.at("color")[3].get<float>();
  } else {
    return false;
  }
  return true;
}

bool convert_json_origin(const Json& src, geometry_msgs::Transform& dst) {
  if (!src.is_array())
    return false;
  if (src.size() != 6)
    return false;
  if (!src[0].is_number())
    return false;
  if (!src[1].is_number())
    return false;
  if (!src[2].is_number())
    return false;
  if (!src[3].is_number())
    return false;
  if (!src[4].is_number())
    return false;
  if (!src[5].is_number())
    return false;

  Eigen::Vector3d xyz_v(src[0].get<double>(), src[1].get<double>(), src[2].get<double>());
  Eigen::Vector3d rpy_v(src[3].get<double>(), src[4].get<double>(), src[5].get<double>());

  Eigen::Affine3d matrix = Eigen::Translation3d(xyz_v) * Eigen::AngleAxisd(rpy_v[0], Eigen::Vector3d::UnitX()) *
                           Eigen::AngleAxisd(rpy_v[1], Eigen::Vector3d::UnitY()) *
                           Eigen::AngleAxisd(rpy_v[2], Eigen::Vector3d::UnitZ());

  dst = eigen_to_msg(matrix);
  return true;
}

bool convert_json_inertial(const Json& src, Inertial& dst) {
  if (!src.is_object())
    return false;
  dst.origin = transform_identity();
  if (src.contains("origin")) {
    if (!convert_json_origin(src.at("origin"), dst.origin))
      return false;
  }

  if (!src.contains("mass"))
    return false;
  if (!src.at("mass").is_number())
    return false;
  dst.mass = src.at("mass").get<double>();

  if (!src.contains("ixx"))
    return false;
  if (!src.contains("ixy"))
    return false;
  if (!src.contains("ixz"))
    return false;
  if (!src.contains("iyy"))
    return false;
  if (!src.contains("iyz"))
    return false;
  if (!src.contains("izz"))
    return false;
  if (!src.at("ixx").is_number())
    return false;
  if (!src.at("ixy").is_number())
    return false;
  if (!src.at("ixz").is_number())
    return false;
  if (!src.at("iyy").is_number())
    return false;
  if (!src.at("iyz").is_number())
    return false;
  if (!src.at("izz").is_number())
    return false;

  dst.ixx = src.at("ixx").get<double>();
  dst.ixy = src.at("ixy").get<double>();
  dst.ixz = src.at("ixz").get<double>();
  dst.iyy = src.at("iyy").get<double>();
  dst.iyz = src.at("iyz").get<double>();
  dst.izz = src.at("izz").get<double>();
  return true;
}

bool convert_json_geometry(const Json& src, std::shared_ptr<Geometry>& dst) {
  if (!src.is_object())
    return false;
  if (!src.contains("type"))
    return false;
  if (!src.at("type").is_string())
    return false;
  std::string type = src.at("type").get<std::string>();
  if (type == "box") {
    auto dst_ptr = std::make_shared<Box>();
    if (!convert_json_box(src, *dst_ptr))
      return false;
    dst = dst_ptr;
  } else if (type == "cylinder") {
    auto dst_ptr = std::make_shared<Cylinder>();
    if (!convert_json_cylinder(src, *dst_ptr))
      return false;
    dst = dst_ptr;
  } else if (type == "sphere") {
    auto dst_ptr = std::make_shared<Sphere>();
    if (!convert_json_sphere(src, *dst_ptr))
      return false;
    dst = dst_ptr;
  } else if (type == "mesh") {
    auto dst_ptr = std::make_shared<Mesh>();
    if (!convert_json_mesh(src, *dst_ptr))
      return false;
    dst = dst_ptr;
  } else {
    return false;
  }
  return true;
}

bool convert_json_visual(const Json& src, Visual& dst) {
  if (!src.is_object())
    return false;
  dst.origin = transform_identity();
  if (src.contains("origin")) {
    if (!convert_json_origin(src.at("origin"), dst.origin))
      return false;
  }

  if (!src.contains("geometry"))
    return false;
  if (!convert_json_geometry(src.at("geometry"), dst.geometry))
    return false;

  if (src.contains("material")) {
    if (!convert_json_material(src.at("material"), dst.material))
      return false;
  }
  return true;
}

bool convert_json_collision(const Json& src, Collision& dst) {
  if (!src.is_object())
    return false;
  dst.origin = transform_identity();
  if (src.contains("origin")) {
    if (!convert_json_origin(src.at("origin"), dst.origin))
      return false;
  }

  if (!src.contains("geometry"))
    return false;
  if (!convert_json_geometry(src.at("geometry"), dst.geometry))
    return false;
  return true;
}

std::shared_ptr<Link> convert_json_link(const Json& src,
                                        const std::map<std::string, std::string>& parent_map,
                                        const std::map<std::string, std::vector<std::string>>& children_map) {
  if (!src.is_object())
    return nullptr;

  if (!src.contains("name"))
    return nullptr;
  if (!src.at("name").is_string())
    return nullptr;
  std::string name = src.at("name").get<std::string>();

  auto parent_it = parent_map.find(name);
  std::string parent = "";
  if (parent_it != parent_map.end()) {
    parent = parent_it->second;
  }

  auto children_it = children_map.find(name);
  std::vector<std::string> children;
  if (children_it != children_map.end()) {
    children = children_it->second;
  }

  std::vector<Visual> visuals;
  if (src.contains("visuals")) {
    if (!src.at("visuals").is_array())
      return nullptr;
    const size_t num_visuals = src.at("visuals").size();
    visuals.resize(num_visuals);
    for (size_t i = 0; i < num_visuals; i++) {
      if (!convert_json_visual(src.at("visuals")[i], visuals[i]))
        return nullptr;
    }
  }

  std::vector<Collision> collisions;
  if (src.contains("collisions")) {
    if (!src.at("collisions").is_array())
      return nullptr;
    const size_t num_collisions = src.at("collisions").size();
    collisions.resize(num_collisions);
    for (size_t i = 0; i < collisions.size(); i++) {
      if (!convert_json_collision(src.at("collisions")[i], collisions[i]))
        return nullptr;
    }
  }

  Inertial inertial;
  if (src.contains("inertial")) {
    if (!convert_json_inertial(src.at("inertial"), inertial))
      return nullptr;
  }

  return std::make_shared<Link>(visuals, collisions, inertial, name, parent, children);
}

bool convert_json_joint_dynamics(const Json& src, JointDynamics& dst) {
  if (!src.is_object())
    return false;
  dst.damping = 0;
  if (src.contains("damping")) {
    if (!src.at("damping").is_number())
      return false;
    dst.damping = src.at("damping").get<double>();
  }
  dst.friction = 0;
  if (src.contains("friction")) {
    if (!src.at("friction").is_number())
      return false;
    dst.friction = src.at("friction").get<double>();
  }
  return true;
}

bool convert_json_joint_mimic(const Json& src, JointMimic& dst) {
  if (!src.is_object())
    return false;

  dst.offset = 0;
  if (!src.contains("offset"))
    return false;
  if (!src.at("offset").is_number())
    return false;
  dst.offset = src.at("offset").get<double>();

  dst.multiplier = 0;
  if (!src.contains("multiplier"))
    return false;
  if (!src.at("multiplier").is_number())
    return false;
  dst.multiplier = src.at("multiplier").get<double>();

  dst.name = "";
  if (!src.contains("name"))
    return false;
  if (!src.at("name").is_string())
    return false;
  dst.name = src.at("name").get<std::string>();
  return true;
}

bool convert_json_joint_limits(const Json& src, JointLimits& dst) {
  if (!src.is_object())
    return false;

  if (!src.contains("effort"))
    return false;
  if (!src.at("effort").is_number())
    return false;
  dst.effort = src.at("effort").get<double>();

  if (!src.contains("velocity"))
    return false;
  if (!src.at("velocity").is_number())
    return false;
  dst.effort = src.at("velocity").get<double>();

  dst.lower = 0;
  if (src.contains("lower")) {
    if (!src.at("lower").is_number())
      return false;
    dst.lower = src.at("lower").get<double>();
  }
  dst.upper = 0;
  if (src.contains("upper")) {
    if (!src.at("upper").is_number())
      return false;
    dst.upper = src.at("upper").get<double>();
  }
  return true;
}

std::shared_ptr<Joint> convert_json_joint(const Json& src) {
  if (!src.is_object())
    return nullptr;

  if (!src.contains("name"))
    return nullptr;
  if (!src.at("name").is_string())
    return nullptr;
  std::string name = src.at("name").get<std::string>();

  if (!src.contains("parent"))
    return nullptr;
  if (!src.at("parent").is_string())
    return nullptr;
  std::string parent = src.at("parent").get<std::string>();

  if (!src.contains("child"))
    return nullptr;
  if (!src.at("child").is_string())
    return nullptr;
  std::string child = src.at("child").get<std::string>();

  Joint::Type type;
  if (!src.contains("type"))
    return nullptr;
  if (!src.at("type").is_string())
    return nullptr;
  std::string type_str = src.at("type").get<std::string>();
  if (type_str == "revolute") {
    type = Joint::Type::REVOLUTE;
  } else if (type_str == "prismatic") {
    type = Joint::Type::PRISMATIC;
  } else if (type_str == "fixed") {
    type = Joint::Type::FIXED;
  } else if (type_str == "continuous") {
    type = Joint::Type::CONTINUOUS;
  } else {
    return nullptr;
  }

  geometry_msgs::Vector3 axis;
  bool has_axis = src.contains("axis");
  if (!has_axis && type != Joint::Type::FIXED)
    return nullptr; // All joint must have axis besides fixed
  if (has_axis) {
    if (!src.at("axis").is_array())
      return nullptr;
    if (src.at("axis").size() != 3)
      return nullptr;
    if (!src.at("axis")[0].is_number())
      return nullptr;
    if (!src.at("axis")[1].is_number())
      return nullptr;
    if (!src.at("axis")[2].is_number())
      return nullptr;
    axis.x = src.at("axis")[0].get<double>();
    axis.y = src.at("axis")[1].get<double>();
    axis.z = src.at("axis")[2].get<double>();
  }

  geometry_msgs::Transform origin = transform_identity();
  if (src.contains("origin")) {
    if (!convert_json_origin(src.at("origin"), origin))
      return nullptr;
  }

  JointLimits limits;
  bool has_limits = src.contains("limits");
  if ((type == Joint::Type::REVOLUTE || type == Joint::Type::PRISMATIC) && !has_limits)
    return nullptr;
  if (has_limits) {
    if (!convert_json_joint_limits(src.at("limits"), limits))
      return nullptr;
  }

  JointDynamics dynamics;
  if (src.contains("dynamics")) {
    if (!convert_json_joint_dynamics(src.at("dynamics"), dynamics))
      return nullptr;
  }

  JointMimic mimic;
  if (src.contains("mimic")) {
    if (!convert_json_joint_mimic(src.at("mimic"), mimic))
      return nullptr;
  }

  return std::make_shared<Joint>(axis, origin, type, limits, dynamics, mimic, name, parent, child);
}

void parse_jrdf(Json& json,
                std::map<std::string, std::shared_ptr<Joint>>& joints,
                std::map<std::string, std::shared_ptr<Link>>& links,
                std::string& root) {
  // Parse joints first to get the link's parent/children
  if (!json.is_object())
    return;

  if (!json.contains("joints"))
    return;
  if (!json.at("joints").is_array())
    return;
  auto joint_array = json.at("joints");
  const size_t num_joints = joint_array.size();
  std::map<std::string, std::string> parent_map;
  std::map<std::string, std::vector<std::string>> children_map;
  for (size_t i = 0; i < num_joints; i++) {
    std::shared_ptr<Joint> j = convert_json_joint(joint_array[i]);
    if (!j)
      return;
    joints.insert({j->name(), j});
    if (parent_map.find(j->child()) != parent_map.end())
      return; // Each link can only have 1 parent
    parent_map.insert({j->child(), j->name()});
    children_map[j->parent()].push_back(j->name());
  }

  // Assign mimic joint pointers
  for (auto j : joints) {
    if (j.second->is_mimic()) {
      j.second->mimic().joint = joints.at(j.second->mimic().name);
    }
  }

  // Parse the links
  if (!json.contains("links"))
    return;
  if (!json.at("links").is_array())
    return;
  auto link_array = json.at("links");
  const size_t num_links = link_array.size();
  for (size_t i = 0; i < num_links; i++) {
    std::shared_ptr<Link> l = convert_json_link(link_array[i], parent_map, children_map);
    if (!l)
      return;
    if (l->is_root())
      root = l->name();
    links.insert({l->name(), l});
  }
  return;
}

} // namespace detail

} // namespace rix