#pragma once
#include <cstdlib>
#include <string>

namespace rix {

static inline std::string get_env(const std::string &var, const std::string &default_val) {
  const char *val = std::getenv(var.c_str());
  return val == nullptr ? default_val : std::string(val);
}

} // namespace rix