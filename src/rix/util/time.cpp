#include "rix/util/time.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace rix {

Time Clock::now() const noexcept { return Time(detail::clock_t::now()); }

void Clock::sleep_for(const Duration& duration) { std::this_thread::sleep_for(duration.raw()); }

void Clock::sleep_until(const Time& time) { std::this_thread::sleep_until(time.raw()); }

std::string Time::to_string(bool local_time) const {
  auto time = Type::clock::to_time_t(std::chrono::time_point_cast<std::chrono::seconds>(tp));
  std::stringstream ss;
  std::tm* timeinfo;
  if (local_time) {
    timeinfo = std::localtime(&time);
  } else {
    timeinfo = std::gmtime(&time);
  }

  ss << std::put_time(timeinfo, "%D %T");
  int64_t us = to_microseconds() % 1'000'000;
  if (us == 0) {
    ss << ".000000";
  } else {
    ss << "." << us;
  }

  if (local_time) {
    ss << " " << std::put_time(timeinfo, "%Z");
  } else {
    ss << " GMT";
  }
  return ss.str();
}

msg::standard::Time Time::to_msg() {
  int64_t ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
  msg::standard::Time msg;
  msg.sec = ns_ / 1'000'000'000;
  msg.nsec = ns_ % 1'000'000'000;
  return msg;
}

Time Time::now() { return clock_->now(); }

void Time::sleep_for(const Duration& duration) { clock_->sleep_for(duration); }

void Time::sleep_until(const Time& time) { clock_->sleep_until(time); }

Time::Time() : tp{} {}

Time::Time(const Type& time_point) : tp(time_point) {}

Time::Time(double seconds)
    : tp(Time::Type() + Duration::Type(static_cast<int64_t>(std::llround(seconds * 1'000'000'000)))) {}

Time::Time(int32_t second, int32_t nanosecond)
    : tp(Time::Type() +
         Duration::Type(static_cast<int64_t>(second) * 1'000'000'000 + static_cast<int64_t>(nanosecond))) {}

Time::Time(const msg::standard::Time& msg) : Time(msg.sec, msg.nsec) {}

Time::Time(const Time& other) : tp(other.tp) {}

Time& Time::operator=(const Time& other) {
  if (this == &other) {
    return *this;
  }
  Time tmp(other);
  std::swap(tp, tmp.tp);
  return *this;
}

Time Time::operator+(const Duration& other) const { return Time(tp + other.raw()); }

Time Time::operator-(const Duration& other) const { return Time(tp - other.raw()); }

Duration Time::operator-(const Time& other) const { return Duration(tp - other.raw()); }

Time& Time::operator+=(const Duration& other) {
  tp += other.raw();
  return *this;
}

Time& Time::operator-=(const Duration& other) {
  tp -= other.raw();
  return *this;
}

bool Time::operator==(const Time& other) const { return tp == other.tp; }

bool Time::operator!=(const Time& other) const { return tp != other.tp; }

bool Time::operator<(const Time& other) const { return tp < other.tp; }

bool Time::operator<=(const Time& other) const { return tp <= other.tp; }

bool Time::operator>(const Time& other) const { return tp > other.tp; }

bool Time::operator>=(const Time& other) const { return tp >= other.tp; }

int64_t Time::to_seconds(RoundType type) const {
  switch (type) {
  case Time::RoundType::FLOOR:
  default:
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
  case Time::RoundType::CEIL:
    return std::chrono::ceil<std::chrono::seconds>(tp.time_since_epoch()).count();
  case Time::RoundType::NEAREST:
    return std::chrono::round<std::chrono::seconds>(tp.time_since_epoch()).count();
  }
}

int64_t Time::to_milliseconds(RoundType type) const {
  switch (type) {
  case Time::RoundType::FLOOR:
  default:
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
  case Time::RoundType::CEIL:
    return std::chrono::ceil<std::chrono::milliseconds>(tp.time_since_epoch()).count();
  case Time::RoundType::NEAREST:
    return std::chrono::round<std::chrono::milliseconds>(tp.time_since_epoch()).count();
  }
}

int64_t Time::to_microseconds(RoundType type) const {
  switch (type) {
  case Time::RoundType::FLOOR:
  default:
    return std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count();
  case Time::RoundType::CEIL:
    return std::chrono::ceil<std::chrono::microseconds>(tp.time_since_epoch()).count();
  case Time::RoundType::NEAREST:
    return std::chrono::round<std::chrono::microseconds>(tp.time_since_epoch()).count();
  }
}

int64_t Time::to_nanoseconds() const { return (tp.time_since_epoch()).count(); }

const Time::Type& Time::raw() const { return tp; }

Time::Type& Time::raw() { return tp; }

Duration::Duration() : d{} {}

Duration::Duration(const msg::standard::Duration& msg) : Duration(msg.sec, msg.nsec) {}

Duration::Duration(const Type& duration) : d(duration) {}

Duration::Duration(double seconds) : d(static_cast<int64_t>(std::llround(seconds * 1'000'000'000))) {}

Duration::Duration(int32_t seconds, int32_t nanoseconds)
    : d(static_cast<int64_t>(seconds) * 1'000'000'000 + static_cast<int64_t>(nanoseconds)) {}

Duration::Duration(const Duration& other) : d(other.d) {}

Duration& Duration::operator=(const Duration& other) {
  if (this == &other) {
    return *this;
  }
  Duration tmp(other);
  std::swap(d, tmp.d);
  return *this;
}

Time Duration::operator+(const Time& other) const { return Time(other.raw() + d); }

Duration Duration::operator+(const Duration& other) const { return Duration(d + other.raw()); }

Duration Duration::operator-(const Duration& other) const { return Duration(d - other.raw()); }

Duration Duration::operator*(double factor) const {
  return Duration(std::chrono::duration_cast<Duration::Type>(d * factor));
}

Duration Duration::operator/(double factor) const {
  return Duration(std::chrono::duration_cast<Duration::Type>(d / factor));
}

Duration Duration::operator*(int factor) const { return Duration(d * factor); }

Duration Duration::operator/(int factor) const { return Duration(d / factor); }

Duration Duration::operator-() const { return Duration(-d); }

Duration& Duration::operator+=(const Duration& other) {
  d += other.raw();
  return *this;
}

Duration& Duration::operator-=(const Duration& other) {
  d -= other.raw();
  return *this;
}

Duration& Duration::operator*=(double factor) {
  d = std::chrono::duration_cast<Duration::Type>(d * factor);
  return *this;
}

Duration& Duration::operator/=(double factor) {
  d = std::chrono::duration_cast<Duration::Type>(d / factor);
  return *this;
}

Duration& Duration::operator*=(int factor) {
  d *= factor;
  return *this;
}

Duration& Duration::operator/=(int factor) {
  d /= factor;
  return *this;
}

bool Duration::operator==(const Duration& other) const { return d == other.d; }

bool Duration::operator!=(const Duration& other) const { return d != other.d; }

bool Duration::operator<(const Duration& other) const { return d < other.d; }

bool Duration::operator<=(const Duration& other) const { return d <= other.d; }

bool Duration::operator>(const Duration& other) const { return d > other.d; }

bool Duration::operator>=(const Duration& other) const { return d >= other.d; }

msg::standard::Duration Duration::to_msg() {
  int64_t ns_ = d.count();
  msg::standard::Duration msg;
  msg.sec = ns_ / 1'000'000'000;
  msg.nsec = ns_ % 1'000'000'000;
  return msg;
}

int64_t Duration::to_seconds(Time::RoundType type) const {
  switch (type) {
  case Time::RoundType::FLOOR:
  default:
    return std::chrono::duration_cast<std::chrono::seconds>(d).count();
  case Time::RoundType::CEIL:
    return std::chrono::ceil<std::chrono::seconds>(d).count();
  case Time::RoundType::NEAREST:
    return std::chrono::round<std::chrono::seconds>(d).count();
  }
}

int64_t Duration::to_milliseconds(Time::RoundType type) const {
  switch (type) {
  case Time::RoundType::FLOOR:
  default:
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
  case Time::RoundType::CEIL:
    return std::chrono::ceil<std::chrono::milliseconds>(d).count();
  case Time::RoundType::NEAREST:
    return std::chrono::round<std::chrono::milliseconds>(d).count();
  }
}

int64_t Duration::to_microseconds(Time::RoundType type) const {
  switch (type) {
  case Time::RoundType::FLOOR:
  default:
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
  case Time::RoundType::CEIL:
    return std::chrono::ceil<std::chrono::microseconds>(d).count();
  case Time::RoundType::NEAREST:
    return std::chrono::round<std::chrono::microseconds>(d).count();
  }
}

int64_t Duration::to_nanoseconds() const { return (d).count(); }

const Duration::Type& Duration::raw() const { return d; }

Duration::Type& Duration::raw() { return d; }

Timer::Timer() {}

Timer::Timer(const Timer& other) : start_(other.start_), end_(other.end_) {}

Timer& Timer::operator=(const Timer& other) {
  if (this == &other) {
    return *this;
  }
  Timer tmp(other);
  std::swap(start_, tmp.start_);
  std::swap(end_, tmp.end_);
  return *this;
}

void Timer::start() { start_ = Time::now(); }

void Timer::stop() { end_ = Time::now(); }

Duration Timer::get() const { return end_ - start_; }

Rate::Rate() : period_(0), start_(Time::now()) {}

Rate::Rate(double frequency)
    : period_((frequency <= min_frequency()) ? max_period() : Duration(1.0 / frequency)), start_(Time::now()) {}

Rate::Rate(Duration period) : period_((period <= min_period()) ? min_period() : period), start_(Time::now()) {}

Rate::Rate(const Rate& other) : period_(other.period_), start_(other.start_) {}

Rate& Rate::operator=(const Rate& other) {
  if (this == &other) {
    return *this;
  }
  Rate tmp(other);
  std::swap(period_, tmp.period_);
  std::swap(start_, tmp.start_);
  return *this;
}

bool Rate::sleep() {
  auto now = Time::now();
  if (start_ + period_ > now) {
    Time::sleep_until(start_ + period_);
    start_ = Time::now();
    return true;
  } else {
    start_ = now;
    return false;
  }
}
Duration Rate::period() const { return period_; }

void Rate::set_period(const Duration& period) { period_ = period; }

double Rate::frequency() const {
  auto ns = period_.to_nanoseconds();
  if (ns == std::chrono::nanoseconds::max().count()) {
    return 0.0;
  }
  return 1.0 / (ns * 1e-9);
}

void Rate::set_frequency(double frequency) {
  if (frequency <= min_frequency()) {
    period_ = max_period();
  } else {
    period_ = Duration(1.0 / frequency);
  }
}

} // namespace rix