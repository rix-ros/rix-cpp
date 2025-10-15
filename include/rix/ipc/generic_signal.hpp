#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "rix/util/time.hpp"

namespace rix {

class GenericSignal {
public:
  GenericSignal() = default;
  virtual ~GenericSignal() = default;

  // Disable copy and move semantics (force use of shared/unique pointers)
  GenericSignal(const GenericSignal &) = delete;
  GenericSignal &operator=(const GenericSignal &) = delete;
  GenericSignal(GenericSignal &&) = delete;
  GenericSignal &operator=(GenericSignal &&) = delete;

  bool is_ready() const { return wait(Duration(0.0)); }
  virtual bool ignore() const = 0;
  virtual bool raise() const = 0;
  virtual bool wait(const Duration &duration) const = 0;
};

} // namespace rix