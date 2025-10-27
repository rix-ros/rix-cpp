#pragma once
#include "rix/util/time.hpp"

namespace rix {

class MockClock final : public GenericClock {
public:
  MockClock() = default;
  ~MockClock() override = default;
  Time now() const noexcept override { return current_time; }
  void sleep_for(const Duration& duration) override { current_time += duration; }
  void sleep_until(const Time& time) override { current_time = time; }

private:
  Time current_time{0};
};

}; // namespace rix