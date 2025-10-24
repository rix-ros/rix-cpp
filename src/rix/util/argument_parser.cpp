#include "rix/util/argument_parser.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

namespace rix {

namespace detail {

bool isalnum(const std::string& str) {
  return std::all_of(
      str.begin(), str.end(), [](const auto& c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
}

bool parse_int8(const std::string& str, std::any& value) {
  try {
    value = static_cast<int8_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_int16(const std::string& str, std::any& value) {
  try {
    value = static_cast<int16_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_int32(const std::string& str, std::any& value) {
  try {
    value = static_cast<int32_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_int64(const std::string& str, std::any& value) {
  try {
    value = static_cast<int64_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_uint8(const std::string& str, std::any& value) {
  try {
    value = static_cast<uint8_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_uint16(const std::string& str, std::any& value) {
  try {
    value = static_cast<uint16_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_uint32(const std::string& str, std::any& value) {
  try {
    value = static_cast<uint32_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_uint64(const std::string& str, std::any& value) {
  try {
    value = static_cast<uint64_t>(std::stoi(str));
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_float(const std::string& str, std::any& value) {
  try {
    value = std::stof(str);
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_double(const std::string& str, std::any& value) {
  try {
    value = std::stod(str);
  } catch (std::invalid_argument const&) {
    return false;
  } catch (std::out_of_range const&) {
    return false;
  }
  return true;
}
bool parse_string(const std::string& str, std::any& value) {
  value = str;
  return true;
}
bool parse_bool(const std::string& str, std::any& value) {
  if (str == "true") {
    value = true;
  } else if (str == "false") {
    value = false;
  } else {
    return false;
  }
  return true;
}

bool parse_vector(const ArgumentParser::ParserFunction& parser, const std::string& str, std::any& value) {
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

ArgumentParser::ArgumentParser(std::string name, std::string description, bool disable_help)
    : name_(std::move(name)), description_(std::move(description)), disable_help_(disable_help) {
  // Add default parsers for built-in types
  parsers_[typeid(int8_t)] = detail::parse_int8;
  parsers_[typeid(std::vector<int8_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_int8, in, out);
  };

  parsers_[typeid(int16_t)] = detail::parse_int16;
  parsers_[typeid(std::vector<int16_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_int16, in, out);
  };

  parsers_[typeid(int32_t)] = detail::parse_int32;
  parsers_[typeid(std::vector<int32_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_int32, in, out);
  };

  parsers_[typeid(int64_t)] = detail::parse_int64;
  parsers_[typeid(std::vector<int64_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_int64, in, out);
  };

  parsers_[typeid(uint8_t)] = detail::parse_uint8;
  parsers_[typeid(std::vector<uint8_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_uint8, in, out);
  };

  parsers_[typeid(uint16_t)] = detail::parse_uint16;
  parsers_[typeid(std::vector<uint16_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_uint16, in, out);
  };

  parsers_[typeid(uint32_t)] = detail::parse_uint32;
  parsers_[typeid(std::vector<uint32_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_uint32, in, out);
  };

  parsers_[typeid(uint64_t)] = detail::parse_uint64;
  parsers_[typeid(std::vector<uint64_t>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_uint64, in, out);
  };

  parsers_[typeid(float)] = detail::parse_float;
  parsers_[typeid(std::vector<float>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_float, in, out);
  };

  parsers_[typeid(double)] = detail::parse_double;
  parsers_[typeid(std::vector<double>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_double, in, out);
  };

  parsers_[typeid(std::string)] = detail::parse_string;
  parsers_[typeid(std::vector<std::string>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_string, in, out);
  };

  // parsers_[typeid(bool)] = detail::parse_bool;
  // No parser for bool because it is a flag and does not take a value
  parsers_[typeid(std::vector<bool>)] = [](const auto& in, auto& out) {
    return detail::parse_vector(detail::parse_bool, in, out);
  };

  if (disable_help_) {
    return;
  }

  // Add default help argument
  add(Arg("help", "Show this help message.", 'h', false, false));
}

ArgumentParser::Arg::Arg(const std::string& name,
                         std::string description,
                         const char short_name,
                         const Value& default_value,
                         const bool required)
    : name(name), description(std::move(description)), default_value(default_value), value(default_value),
      required(required), short_name(short_name) {
  if (name.size() < 2 || !detail::isalnum(name)) {
    throw std::invalid_argument("Name must be at least 2 alphanumeric characters.");
  }
}

void ArgumentParser::add(const Arg& arg) {
  if (arg.required) {
    required_arg_names_.emplace_back(arg.name);
  } else if (arg.short_name != '\0') {
    short_to_long_[arg.short_name] = arg.name;
  }
  args_.emplace(arg.name, arg);
}

bool ArgumentParser::parse_arg(char** argv, int argc, int& offset, Arg& arg) {
  // If the argument is a bool, then it is a flag and does not take a value
  if (arg.value.type() == typeid(bool)) {
    arg.value = !std::any_cast<bool>(arg.value); // Invert the default value
    return true;
  }

  auto it = parsers_.find(arg.value.type());
  if (it == parsers_.end()) {
    return false;
  }

  auto& parser = it->second;
  if (!parser(std::string(argv[offset++]), arg.value)) {
    return false;
  }
  return true;
}

bool ArgumentParser::parse(int argc, char** argv) {
  size_t required_size = argc - 1;
  if (required_size < required_arg_names_.size()) {
    return false;
  }

  int offset = 1;
  for (const std::string& name : required_arg_names_) {
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
      if (name[0] != '-') {
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
      if (name[0] != '-' || name[1] != '-') {
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

  // If help was requested, print help and return false
  if (disable_help_) {
    return true;
  }
  auto it = args_.find("help");
  if (it != args_.end()) {
    bool help_value = false;
    if (it->second.value.type() == typeid(bool)) {
      help_value = std::any_cast<bool>(it->second.value);
    }
    if (help_value) {
      std::cout << help() << std::endl;
      return false;
    }
  }
  return true;
}

// clang-format off
void ArgumentParser::detect_help_column_widths(int &name_width, int &short_width, int &desc_width,
                                               int &default_width) const {
  // First, scan all arguments to determine max width for each column
  name_width = static_cast<int>(std::string("Name").size());
  short_width = static_cast<int>(std::string("Short").size());
  desc_width = static_cast<int>(std::string("Description").size());
  default_width = static_cast<int>(std::string("Default").size());

  // Required arguments
  for (const auto &arg_name : required_arg_names_) {
    const auto &arg = args_.at(arg_name);
    name_width = std::max(name_width, static_cast<int>(arg.name.size()));
    short_width = std::max(short_width, arg.short_name == '\0' ? 0 : 1);
    desc_width = std::max(desc_width, static_cast<int>(arg.description.size()));
    default_width = std::max(default_width, static_cast<int>(sizeof("(required)")));
  }

  // Optional arguments
  for (const auto &pair : args_) {
    const auto &arg = pair.second;
    if (!arg.required) {
      std::string name_str = "--" + arg.name;
      name_width = std::max(name_width, static_cast<int>(name_str.size()));
      short_width = std::max(short_width, arg.short_name == '\0' ? 0 : 2);
      desc_width = std::max(desc_width, static_cast<int>(arg.description.size()));

      std::string default_str;
      if (arg.default_value.has_value()) {
        try {
          if (arg.default_value.type() == typeid(std::string)) {
            default_str = std::any_cast<std::string>(arg.default_value);
          } else if (arg.default_value.type() == typeid(int)) {
            default_str = std::to_string(std::any_cast<int>(arg.default_value));
          } else if (arg.default_value.type() == typeid(double)) {
            default_str = std::to_string(std::any_cast<double>(arg.default_value));
          } else if (arg.default_value.type() == typeid(bool)) {
            default_str = std::any_cast<bool>(arg.default_value) ? "true" : "false";
          } else {
            default_str = "<value>";
          }
        } catch (const std::bad_any_cast &) {
          default_str = "<value>";
        }
      }
      default_width = std::max(default_width, static_cast<int>(default_str.size()));
    }
  }

  // Add padding
  name_width += 2;
  short_width += 2;
  desc_width += 2;
  default_width += 2;
}

std::string ArgumentParser::help() {
  // First, scan all arguments to determine max width for each column
  int name_width, short_width, desc_width, default_width;
  detect_help_column_widths(name_width, short_width, desc_width, default_width);

  std::stringstream ss;
  ss << "Usage: " << name_;

  // Required arguments
  for (const auto &arg_name : required_arg_names_) {
    const auto &arg = args_.at(arg_name);
    ss << " " << arg.name;
  }

  // Optional arguments
  for (const auto &pair : args_) {
    const auto &arg = pair.second;
    if (!arg.required) {
      ss << " [--" << arg.name;
      if (arg.short_name != '\0') {
        ss << " (-" << arg.short_name << ")";
      }
      if (arg.value.type() != typeid(bool)) {
        ss << " <value>";
      }
      ss << "]";
    }
  }

  ss << "\n\n" << description_ << "\n\nArguments\n";
  ss << std::string(name_width + short_width + desc_width + default_width, '-') << "\n";
  ss << std::left
     << std::setw(name_width) << "Name"
     << std::setw(short_width) << "Short"
     << std::setw(desc_width) << "Description"
     << std::setw(default_width) << "Default"
     << "\n";
  ss << std::string(name_width + short_width + desc_width + default_width, '-') << "\n";

  // Required arguments description
  for (const auto &arg_name : required_arg_names_) {
    const auto &arg = args_.at(arg_name);
    ss << std::left
       << std::setw(name_width) << arg.name
       << std::setw(short_width) << (arg.short_name == '\0' ? "" : std::string(1, arg.short_name))
       << std::setw(desc_width) << arg.description
       << std::setw(default_width) << "(required)"
       << "\n";
  }

  // Optional arguments description
  for (const auto & [_, arg] : args_) {
    if (!arg.required) {
      std::string default_str = "<value>";
      if (arg.default_value.has_value()) {
        try {
          if (arg.default_value.type() == typeid(std::string)) {
            default_str = std::any_cast<std::string>(arg.default_value);
          } else if (arg.default_value.type() == typeid(int)) {
            default_str = std::to_string(std::any_cast<int>(arg.default_value));
          } else if (arg.default_value.type() == typeid(double)) {
            default_str = std::to_string(std::any_cast<double>(arg.default_value));
          } else if (arg.default_value.type() == typeid(bool)) {
            default_str = std::any_cast<bool>(arg.default_value) ? "true" : "false";
          }
        } catch (const std::bad_any_cast &) {}
      }

      ss << std::left
         << std::setw(name_width) << ("--" + arg.name)
         << std::setw(short_width) << (arg.short_name == '\0' ? "" : ("-" + std::string(1, arg.short_name)))
         << std::setw(desc_width) << arg.description
         << std::setw(default_width) << (default_str.empty() ? "" : default_str)
         << "\n";
    }
  }

  return ss.str();
}
// clang-format on

} // namespace rix