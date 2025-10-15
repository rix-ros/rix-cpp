#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "rix/rix.hpp"

using namespace rix;

std::array<uint64_t, 2> str_to_id(std::string id) {
  std::array<uint64_t, 2> arr;
  const size_t len = 32;
  if (id.length() != len) {
    throw std::invalid_argument("ID must be a 32 character hexadecimal string.");
  }

  const size_t base = 16;
  arr[0] = std::stoull(id.substr(0, len / 2), nullptr, base);
  arr[1] = std::stoull(id.substr(len / 2, len / 2), nullptr, base);
  return arr;
}

bool get_message_name(const std::array<uint64_t, 2> &hash, std::string &name) {
  // Open the file ~/.rix/rixmsg/index.txt
  std::string pathname = std::getenv("HOME") + std::string("/.rix/rixmsg/index.txt");
  std::ifstream file(pathname);
  if (!file.is_open()) {
    Log::error << "Failed to open index file" << std::endl;
    return false;
  }

  // The file is formatted as:
  // <hash> <message_name>
  // and is sorted by hash.
  // Perform linear search to find the hash. If found store the message name in name and return true.
  // If not found return false.
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string hash_str;
    std::string message_name;
    if (!(iss >> hash_str >> message_name)) {
      Log::error << "Error parsing line: " << line << std::endl;
      continue; // Error parsing line
    }
    // Convert hash string (example: "0112179c1731bc7c0ec81825b8f487f4") to
    // std::array<uint64_t, 2> hash
    std::array<uint64_t, 2> file_hash = str_to_id(hash_str);
    if (file_hash == hash) {
      name = message_name;
      file.close();
      return true; // Hash found
    }

    // If the hash is greater than the current hash, we can stop searching
    // as the file is sorted.
    if (file_hash > hash) {
      break; // Hash not found
    }
  }
  file.close();
  return false; // Hash not found
}

int main(int argc, char **argv) {
  Node node("rixinfo");
  if (!node.ok()) {
    Log::error << "Failed to initialize node" << std::endl;
    return 1;
  }

  msg::mediator::SystemInfo system_info;
  if (!node.get_system_info(system_info)) {
    Log::error << "Failed to get system info" << std::endl;
    return 1;
  }

  std::unordered_map<uint64_t, std::stringstream> node_info;
  for (const auto &node : system_info.nodes) {
    std::stringstream ss;
    ss << "  " << node.name << "\n";
    // ID as hex
    ss << "    ID: " << node.id << "\n";
    node_info[node.id] = std::move(ss);
  }

  for (const auto &node : system_info.nodes) {
    std::stringstream &ss = node_info[node.id];
    ss << "    Publishers:\n";
  }

  for (const auto &pub : system_info.publishers) {
    std::stringstream &ss = node_info[pub.node_id];
    ss << "      ID: " << pub.id << "\n";
    ss << "      IP: " << pub.endpoint.address << "\n";
    ss << "      Port: " << pub.endpoint.port << "\n";
    ss << "      Topic: " << pub.topic_info.name << "\n";
  }

  for (const auto &node : system_info.nodes) {
    std::stringstream &ss = node_info[node.id];
    ss << "    Subscribers:\n";
  }

  for (const auto &sub : system_info.subscribers) {
    std::stringstream &ss = node_info[sub.node_id];
    ss << "      ID: " << sub.id << "\n";
    ss << "      IP: " << sub.endpoint.address << "\n";
    ss << "      Port: " << sub.endpoint.port << "\n";
    ss << "      Topic: " << sub.topic_info.name << "\n";
  }

  for (const auto &node : system_info.nodes) {
    std::stringstream &ss = node_info[node.id];
    ss << "    Services:\n";
  }

  for (const auto &srv : system_info.services) {
    std::stringstream &ss = node_info[srv.node_id];
    ss << "      Name: " << srv.name << "\n";
    ss << "      ID: " << srv.id << "\n";
    std::string request_name;
    if (get_message_name(srv.request_hash, request_name)) {
      ss << "      Request Type: " << request_name << "\n";
    } else {
      ss << "      Request Type: Unknown\n";
    }
    std::string response_name;
    if (get_message_name(srv.response_hash, response_name)) {
      ss << "      Response Type: " << response_name << "\n";
    } else {
      ss << "      Response Type: Unknown\n";
    }
  }

  for (const auto &node : system_info.nodes) {
    std::stringstream &ss = node_info[node.id];
    ss << "    Actions:\n";
  }

  for (const auto &act : system_info.actions) {
    std::stringstream &ss = node_info[act.node_id];
    ss << "      Name: " << act.name << "\n";
    ss << "      ID: " << act.id << "\n";
    std::string goal_name;
    if (get_message_name(act.goal_hash, goal_name)) {
      ss << "      Goal Type: " << goal_name << "\n";
    } else {
      ss << "      Goal Type: Unknown\n";
    }
    std::string feedback_name;
    if (get_message_name(act.feedback_hash, feedback_name)) {
      ss << "      Feedback Type: " << feedback_name << "\n";
    } else {
      ss << "      Feedback Type: Unknown\n";
    }
    std::string result_name;
    if (get_message_name(act.result_hash, result_name)) {
      ss << "      Result Type: " << result_name << "\n";
    } else {
      ss << "      Result Type: Unknown\n";
    }
  }

  std::cout << "Topics:\n";
  for (const auto &topic : system_info.topics) {
    std::cout << "  " << topic.name << ": ";
    std::string message_name;
    if (get_message_name(topic.message_hash, message_name)) {
      std::cout << message_name << "\n";
    } else {
      std::cout << "Unknown\n";
    }
  }
  std::cout << std::endl;

  std::cout << "Nodes:\n";
  for (const auto &pair : node_info) {
    std::cout << pair.second.str() << std::endl;
  }
  return 0;
}