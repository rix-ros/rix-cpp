#include "rix/util/argument_parser.hpp"

namespace rix {
namespace util {

ArgumentParser::ArgumentParser(const std::string &name, const std::string &description)
    : name_(name), description_(description) {}

void ArgumentParser::add(const Arg &arg) {
    if (arg.required) {
        required_arg_names_.emplace_back(arg.name);
    } else if (arg.short_name != '\0') {
        short_to_long_[arg.short_name] = arg.name;
    }
    args_.emplace(arg.name, arg);
}

bool ArgumentParser::parse_arg(char **argv, int argc, int &offset, Arg &arg) {
    Value &value(arg.value);
    switch (value.index()) {
        case VariantIndex::BOOL:
            return detail::parse(argv, argc, offset, std::get<bool>(value));
        case VariantIndex::CHAR:
            return detail::parse(argv, argc, offset, std::get<char>(value));
        case VariantIndex::INT:
            return detail::parse(argv, argc, offset, std::get<int>(value));
        case VariantIndex::DOUBLE:
            return detail::parse(argv, argc, offset, std::get<double>(value));
        case VariantIndex::STRING:
            return detail::parse(argv, argc, offset, std::get<std::string>(value));
        case VariantIndex::INT_VEC:
            return detail::parse(argv, argc, offset, std::get<std::vector<int>>(value));
        case VariantIndex::DOUBLE_VEC:
            return detail::parse(argv, argc, offset, std::get<std::vector<double>>(value));
        case VariantIndex::STRING_VEC:
            return detail::parse(argv, argc, offset, std::get<std::vector<std::string>>(value));
    }
    return false;
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

namespace detail {

bool isalnum(const std::string &str) {
    for (const char &c : str) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

bool parse(char **argv, int argc, int &offset, std::string &value) {
    if (offset >= argc) {
        return false;
    }
    value = argv[offset++];
    return true;
}

bool parse(char **argv, int argc, int &offset, bool &value) {
    if (offset > argc) {
        return false;
    }
    value = !value;
    return true;
}

}  // namespace detail

}  // namespace util
}  // namespace rix