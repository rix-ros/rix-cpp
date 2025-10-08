#pragma once

#include <any>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <typeindex>
#include <vector>

namespace rix {

namespace detail {
template <typename> struct is_std_vector : std::false_type {};
template <typename T, typename A> struct is_std_vector<std::vector<T, A>> : std::true_type {};
bool isalnum(const std::string &str);
} // namespace detail

/**
 * @brief A command-line parser class.
 */
class ArgumentParser {

public:
  using Value = std::any;
  using ParserFunction = std::function<bool(const std::string &, std::any &)>;

private:
  /**
   * @brief A class representing a command line argument.
   */
  struct Arg {
    Arg(const std::string &name, const std::string &description, char short_name, const Value &default_value,
        bool required);

    std::string name{};        ///< long name (--<name>), must be alphanumeric and greater than 2 characters
    std::string description{}; ///< description of the argument
    Value default_value{};     ///< default value of the argument
    Value value{};             ///< value of the argument
    bool required{};           ///< whether the argument is required
    char short_name{};         ///< short name (single character) (-<short_name>), must be alphanumeric
  };

  void add(const Arg &arg);
  bool parse_arg(char **argv, int argc, int &offset, Arg &arg);

public:
  /**
   * @brief Constructs a ArgumentParser object.
   * @param name The name of the program.
   * @param description The description of the program.
   */
  ArgumentParser(const std::string &name, const std::string &description, bool disable_help = false);

  /**
   * @brief Adds an argument to the parser.
   * @param argument The argument to add.
   */
  template <typename T> void add(const std::string &name, const std::string &description);
  template <typename T> void add(const std::string &name, const std::string &description, const T &default_value);
  template <typename T>
  void add(const std::string &name, const std::string &description, char short_name, const T &default_value);

  /**
   * @brief Parses the command-line arguments.
   * @param argc The number of arguments.
   * @param argv The array of arguments.
   */
  bool parse(int argc, char **argv);

  /**
   * @brief Gets the values of an option.
   * @param name The name of the option.
   * @return The value of the option.
   */
  template <typename T> bool get(const std::string &name, T &value);

  /**
   * @brief Gets the help message.
   * @return The help message as a string.
   */
  std::string help();

  template <typename T> void add_parser(ParserFunction parser);

private:
  ///< The name of the program.
  std::string name_{};
  ///< The description of the program.
  std::string description_{};
  ///< Ordering of required arguments
  std::vector<std::string> required_arg_names_{};
  ///< short name to long name
  std::map<char, std::string> short_to_long_{};
  ///< arguments map
  std::map<std::string, Arg> args_{};
  ///< custom parsers for user-defined types
  std::map<std::type_index, ParserFunction> parsers_{};
  ///< disable automatic help argument
  bool disable_help_{false};

  void detect_help_column_widths(size_t &name_width, size_t &short_width, size_t &desc_width, size_t &default_width);
};

template <typename T> bool ArgumentParser::get(const std::string &name, T &value) {
  auto arg = args_.find(name);
  if (arg == args_.end()) {
    return false;
  }

  // At compile time, check if T is a std::vector
  if constexpr (detail::is_std_vector<T>::value) {
    using ElemType = typename T::value_type;
    const auto &vec = std::any_cast<const std::vector<std::any> &>(arg->second.value);
    std::vector<ElemType> result;
    result.reserve(vec.size());

    // Iterate and cast each element
    for (const auto &elem : vec) {
      if (elem.type() != typeid(ElemType)) {
        return false;
      }
      result.push_back(std::any_cast<ElemType>(elem));
    }

    value = result;
    return true;
  }

  // Not a vector, just cast directly
  if (arg->second.value.type() != typeid(T)) {
    return false;
  }
  value = std::any_cast<T>(arg->second.value);
  return true;
}

template <typename T> void ArgumentParser::add(const std::string &name, const std::string &description) {
  add(Arg(name, description, '\0', T(), true));
}

template <typename T>
void ArgumentParser::add(const std::string &name, const std::string &description, const T &default_value) {
  add(Arg(name, description, '\0', default_value, false));
}

template <typename T>
void ArgumentParser::add(const std::string &name, const std::string &description, char short_name,
                         const T &default_value) {
  if (!std::isalnum(short_name)) {
    throw std::invalid_argument("Short name must be alphanumeric.");
  }
  add(Arg(name, description, short_name, default_value, false));
}

namespace detail {

bool parse_vector(ArgumentParser::ParserFunction parser, const std::string &str, std::any &value);

} // namespace detail

template <typename T> void ArgumentParser::add_parser(ParserFunction parser) {
  auto it = parsers_.find(typeid(T));
  if (it != parsers_.end()) {
    throw std::invalid_argument("Parser for type already exists.");
  }
  parsers_[typeid(T)] = parser;
  parsers_[typeid(std::vector<T>)] =
      std::bind(detail::parse_vector, parser, std::placeholders::_1, std::placeholders::_2);
}

} // namespace rix