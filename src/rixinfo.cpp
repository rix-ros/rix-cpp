#include <iomanip>
#include <sstream>
#include <array>
#include <fstream>

#include "rix/rix.hpp"

using namespace rix::core;
using namespace rix::util;

// Helper to convert uint64_t to base62 string (a-zA-Z0-9)
std::string id_to_str(uint64_t id) {
    static const char base62_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string s;
    do {
        s = base62_chars[id % 62] + s;
        id /= 62;
    } while (id > 0);
    // Pad to 8 chars for consistent length
    while (s.length() < 8) s = "0" + s;
    return s;
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
            continue;  // Error parsing line
        }
        // Convert hash string (example: "0112179c1731bc7c0ec81825b8f487f4") to
        // std::array<uint64_t, 2> hash
        std::array<uint64_t, 2> file_hash;
        file_hash[0] = std::stoull(hash_str.substr(0, 16), nullptr, 16);
        file_hash[1] = std::stoull(hash_str.substr(16, 16), nullptr, 16);
        if (file_hash == hash) {
            name = message_name;
            file.close();
            return true;  // Hash found
        }

        // If the hash is greater than the current hash, we can stop searching
        // as the file is sorted.
        if (file_hash > hash) {
            break;  // Hash not found
        }
    }
    file.close();
    return false;  // Hash not found
}

int main(int argc, char **argv) {
    ArgumentParser parser("rixinfo", "RIX Info CLI");
    parser.add<std::string>("ip", "RIX Hub IP Address", 'i', "127.0.0.1");

    if (!parser.parse(argc, argv)) {
        std::cerr << "Error parsing arguments" << std::endl;
        return 1;
    }

    std::string hub_ip;
    parser.get<std::string>("ip", hub_ip);

    Node node("rixinfo", rix::ipc::Endpoint(hub_ip, rix::core::RIXHUB_PORT));
    if (!node.ok()) {
        Log::error << "Failed to initialize node" << std::endl;
        return 1;
    }

    rix::msg::mediator::SystemInfo system_info;
    if (!node.get_system_info(system_info)) {
        Log::error << "Failed to get system info" << std::endl;
        return 1;
    }

    std::unordered_map<uint64_t, std::stringstream> node_info;
    for (const auto &node : system_info.nodes) {
        std::stringstream ss;
        ss << node.name << ":\n";
        ss << "  ID: '" << id_to_str(node.id) << "'\n";
        node_info[node.id] = std::move(ss);
    }

    for (const auto &pub : system_info.publishers) {
        std::stringstream &ss = node_info[pub.node_id];
        ss << "  Publishers:\n";
        ss << "    '" << id_to_str(pub.id) << "':\n";
        ss << "      IP: " << pub.endpoint.address << "\n";
        ss << "      Port: " << pub.endpoint.port << "\n";
        ss << "      Topic: " << pub.topic_info.name << "\n";
        std::string message_name;
        if (get_message_name(pub.topic_info.message_hash, message_name)) {
            ss << "      Message Type: " << message_name << "\n";
        } else {
            ss << "      Message Type: Unknown\n";
        }
    }

    for (const auto &sub : system_info.subscribers) {
        std::stringstream &ss = node_info[sub.node_id];
        ss << "  Subscribers:\n";
        ss << "    '" << id_to_str(sub.id) << "':\n";
        ss << "      IP: " << sub.endpoint.address << "\n";
        ss << "      Port: " << sub.endpoint.port << "\n";
        ss << "      Topic: " << sub.topic_info.name << "\n";
        std::string message_name;
        if (get_message_name(sub.topic_info.message_hash, message_name)) {
            ss << "      Message Type: " << message_name << "\n";
        } else {
            ss << "      Message Type: Unknown\n";
        }
    }

    for (const auto &srv : system_info.services) {
        std::stringstream &ss = node_info[srv.node_id];
        ss << "  Services:\n";
        ss << "    '" << id_to_str(srv.id) << "':\n";
        ss << "      Name: " << srv.name << "\n";
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

    for (const auto &act : system_info.actions) {
        std::stringstream &ss = node_info[act.node_id];
        ss << "  Actions:\n";
        ss << "    '" << id_to_str(act.id) << "':\n";
        ss << "      Name: " << act.name << "\n";
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
    for (const auto &pair : node_info) {
        std::cout << pair.second.str() << std::endl;
    }
    return 0;
}