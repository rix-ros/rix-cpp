#include "rix/tf/transform_buffer.hpp"

#include <algorithm>
#include <eigen3/Eigen/Geometry>

#include "rix/rob/eigen_util.hpp"

namespace rix {

TransformBuffer::TransformBuffer() {}

TransformBuffer::TransformBuffer(const Duration& duration) : duration_(duration) {}

TransformBuffer::TransformBuffer(const TransformBuffer& other) : duration_(other.duration_), buffer_(other.buffer_) {}

TransformBuffer& TransformBuffer::operator=(const TransformBuffer& other) {
  if (this != &other) {
    duration_ = other.duration_;
    buffer_ = other.buffer_;
  }
  return *this;
}

size_t TransformBuffer::size() const { return buffer_.size(); }

bool TransformBuffer::empty() const { return buffer_.empty(); }

void TransformBuffer::clear() { buffer_.clear(); }

/*< TODO: Implement the insert method. */
void TransformBuffer::insert(const Time& time, const geometry_msgs::Transform& transform) {
  // Insert new entry with binary search
  auto it = std::lower_bound(
      buffer_.begin(), buffer_.end(), time, [](const std::pair<Time, geometry_msgs::Transform>& a, const Time& b) {
        return a.first < b;
      });
  // If the times are equal, overwrite
  if (it != buffer_.end() && it->first == time) {
    it->second = transform;
  } else {
    // Otherwise, insert into the buffer
    buffer_.insert(it, std::make_pair(time, transform));
  }

  // Remove old entries
  while (!buffer_.empty() && (buffer_.back().first - buffer_.front().first) > duration_) {
    buffer_.pop_front();
  }
}

/*< TODO: Implement the get method. */
bool TransformBuffer::get(const Time& time, geometry_msgs::Transform& transform) const {
  if (buffer_.empty()) {
    return false;
  }

  // If time is zero, return the latest entry
  if (time.to_nanoseconds() == 0) {
    transform = buffer_.back().second;
    return true;
  }

  // Find the closest entry in the buffer
  auto it = std::lower_bound(
      buffer_.begin(), buffer_.end(), time, [](const std::pair<Time, geometry_msgs::Transform>& a, const Time& b) {
        return a.first < b;
      });

  // If the input time is later than the last entry, return the last entry
  if (it == buffer_.end()) {
    transform = (it - 1)->second;
    return true;
  }

  // If the input time is earlier than the first entry, return the first entry
  if (it == buffer_.begin()) {
    transform = it->second;
    return true;
  }

  // If the input time matches an entry, return that entry
  if (it->first == time) {
    transform = it->second;
    return true;
  }

  // Otherwise, linear interpolate between the two closest entries.
  // Use LERP for translation and SLERP for rotation.
  auto it_prev = it - 1;
  double t = static_cast<double>((time - it_prev->first).to_nanoseconds()) /
             static_cast<double>((it->first - it_prev->first).to_nanoseconds());
  transform = interpolate(it_prev->second, it->second, t);
  return true;
}

const std::deque<std::pair<Time, geometry_msgs::Transform>>& TransformBuffer::data() const { return buffer_; }

Duration TransformBuffer::duration() const { return duration_; }

void TransformBuffer::set_duration(const Duration& duration) { duration_ = duration; }

} // namespace rix