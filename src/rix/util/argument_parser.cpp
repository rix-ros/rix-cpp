#include "rix/util/argument_parser.hpp"

namespace rix {
namespace util {

namespace detail {

bool isalnum(const std::string &str) {
  for (const char &c : str) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
      return false;
    }
  }
  return true;
}

bool parse_int8(const std::string &str, std::any &value) {
  try {
    value = static_cast<int8_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_int16(const std::string &str, std::any &value) {
  try {
    value = static_cast<int16_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_int32(const std::string &str, std::any &value) {
  try {
    value = static_cast<int32_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_int64(const std::string &str, std::any &value) {
  try {
    value = static_cast<int64_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_uint8(const std::string &str, std::any &value) {
  try {
    value = static_cast<uint8_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_uint16(const std::string &str, std::any &value) {
  try {
    value = static_cast<uint16_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_uint32(const std::string &str, std::any &value) {
  try {
    value = static_cast<uint32_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_uint64(const std::string &str, std::any &value) {
  try {
    value = static_cast<uint64_t>(std::stoi(str));
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_float(const std::string &str, std::any &value) {
  try {
    value = std::stof(str);
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_double(const std::string &str, std::any &value) {
  try {
    value = std::stod(str);
  } catch (std::invalid_argument const &ex) {
    return false;
  } catch (std::out_of_range const &ex) {
    return false;
  }
  return true;
}
bool parse_string(const std::string &str, std::any &value) {
  value = str;
  return true;
}
bool parse_bool(const std::string &str, std::any &value) {
  if (str == "true") {
    value = true;
  } else if (str == "false") {
    value = false;
  } else {
    return false;
  }
  return true;
}

bool parse_vector(ArgumentParser::ParserFunction parser, const std::string &str, std::any &value) {
  std::vector<std::any> vec;
  size_t start = 0;
  size_t end = str.find(',');
  while (end != std::string::npos) {
    std::string token = str.substr(start, end - start);
    std::any element;
    if (!parser(token, element)) {
      return false;
    }
    vec.push_back(element);
    start = end + 1;
    end = str.find(',', start);
  }
  std::string token = str.substr(start);
  std::any element;
  if (!parser(token, element)) {
    return false;
  }
  vec.push_back(element);
  value = vec;
  return true;
}

} // namespace detail

ArgumentParser::ArgumentParser(const std::string &name, const std::string &description)
    : name_(name), description_(description) {
  // Add default parsers for built-in types
  parsers_[typeid(int8_t)] = detail::parse_int8;
  parsers_[typeid(std::vector<int8_t>)] =
      std::bind(detail::parse_vector, detail::parse_int8, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(int16_t)] = detail::parse_int16;
  parsers_[typeid(std::vector<int16_t>)] =
      std::bind(detail::parse_vector, detail::parse_int16, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(int32_t)] = detail::parse_int32;
  parsers_[typeid(std::vector<int32_t>)] =
      std::bind(detail::parse_vector, detail::parse_int32, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(int64_t)] = detail::parse_int64;
  parsers_[typeid(std::vector<int64_t>)] =
      std::bind(detail::parse_vector, detail::parse_int64, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(uint8_t)] = detail::parse_uint8;
  parsers_[typeid(std::vector<uint8_t>)] =
      std::bind(detail::parse_vector, detail::parse_uint8, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(uint16_t)] = detail::parse_uint16;
  parsers_[typeid(std::vector<uint16_t>)] =
      std::bind(detail::parse_vector, detail::parse_uint16, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(uint32_t)] = detail::parse_uint32;
  parsers_[typeid(std::vector<uint32_t>)] =
      std::bind(detail::parse_vector, detail::parse_uint32, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(uint64_t)] = detail::parse_uint64;
  parsers_[typeid(std::vector<uint64_t>)] =
      std::bind(detail::parse_vector, detail::parse_uint64, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(float)] = detail::parse_float;
  parsers_[typeid(std::vector<float>)] =
      std::bind(detail::parse_vector, detail::parse_float, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(double)] = detail::parse_double;
  parsers_[typeid(std::vector<double>)] =
      std::bind(detail::parse_vector, detail::parse_double, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(std::string)] = detail::parse_string;
  parsers_[typeid(std::vector<std::string>)] =
      std::bind(detail::parse_vector, detail::parse_string, std::placeholders::_1, std::placeholders::_2);

  parsers_[typeid(bool)] = detail::parse_bool;
  parsers_[typeid(std::vector<bool>)] =
      std::bind(detail::parse_vector, detail::parse_bool, std::placeholders::_1, std::placeholders::_2);
}

ArgumentParser::Arg::Arg(const std::string &name, const std::string &description, char short_name,
                         const Value &default_value, bool required)
    : name(name), description(description), value(default_value), required(required), short_name(short_name) {
  if (name.size() < 2 || !detail::isalnum(name)) {
    throw std::invalid_argument("Name must be at least 2 alphanumeric characters.");
  }
}

void ArgumentParser::add(const Arg &arg) {
  if (arg.required) {
    required_arg_names_.emplace_back(arg.name);
  } else if (arg.short_name != '\0') {
    short_to_long_[arg.short_name] = arg.name;
  }
  args_.emplace(arg.name, arg);
}

bool ArgumentParser::parse_arg(char **argv, int argc, int &offset, Arg &arg) {
  auto it = parsers_.find(arg.value.type());
  if (it == parsers_.end()) {
    return false;
  }

  auto &parser = it->second;
  if (!parser(std::string(argv[offset++]), arg.value)) {
    return false;
  }
  return true;
}

bool ArgumentParser::parse(int argc, char **argv) {
  if (argc - 1 < required_arg_names_.size()) {
    return false;
  }

  int offset = 1;
  for (const std::string &name : required_arg_names_) {
    if (!parse_arg(argv, argc, offset, args_.at(name))) {
      return false;
    }
  }

  while (offset < argc) {
    std::string name(argv[offset++]);
    if (name.size() < 2 || name.size() == 3) {
      return false;
    }

    // Handle short_name
    if (name.size() == 2) {
      if (!name.starts_with('-')) {
        return false;
      }
      auto it = short_to_long_.find(name[1]);
      // If the option is not recognized
      if (it == short_to_long_.end()) {
        return false;
      }
      name = it->second;
    } else {
      // name.size() >= 4
      if (!name.starts_with("--")) {
        return false;
      }
      name = name.substr(2);
    }

    auto it = args_.find(name);
    if (it == args_.end()) {
      return false;
    }

    if (!parse_arg(argv, argc, offset, it->second)) {
      return false;
    }
  }

  return true;
}

std::string ArgumentParser::help() {
  std::string usage = "Usage: " + name_;
  std::string opt_usage;
  std::string description = "\n\n" + description_ + "\n\nArguments:\n";
  std::string opt_description;
  for (const auto &arg : required_arg_names_) {
    const auto &argInfo = args_.at(arg);
    usage += " " + argInfo.name;
    description += "  " + argInfo.name + " - " + argInfo.description + "\n";
  }
  for (const auto &pair : args_) {
    const auto &arg = pair.second;
    if (!arg.required) {
      opt_usage += " [--" + arg.name + " (-" + arg.short_name + ")]";
      opt_description += "  --" + arg.name + " (-" + arg.short_name + ")" + " - " + arg.description + "\n";
    }
  }
  return usage + opt_usage + description + opt_description;
}

} // namespace util
} // namespace rix