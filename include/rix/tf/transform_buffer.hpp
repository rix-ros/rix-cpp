#pragma once

#include <deque>

#include "rix/geometry_msgs/Transform.hpp"
#include "rix/util/time.hpp"

namespace rix {

class TransformBuffer {
public:
  TransformBuffer();
  explicit TransformBuffer(const Duration& duration);
  TransformBuffer(const TransformBuffer& other);
  TransformBuffer& operator=(const TransformBuffer& other);

  size_t size() const;
  bool empty() const;
  void clear();

  void insert(const Time& time, const geometry_msgs::Transform& transform);
  bool get(const Time& time, geometry_msgs::Transform& transform) const;

  const std::deque<std::pair<Time, geometry_msgs::Transform>>& data() const;

  Duration duration() const;
  void set_duration(const Duration& duration);

private:
  Duration duration_;
  std::deque<std::pair<Time, geometry_msgs::Transform>> buffer_;
};

} // namespace rix