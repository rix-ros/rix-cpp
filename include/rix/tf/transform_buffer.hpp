#pragma once

#include <deque>

#include "rix/msg/geometry/Transform.hpp"
#include "rix/msg/standard/Time.hpp"

#include "rix/util/time.hpp"

namespace rix {

class TransformBuffer {
   public:
    TransformBuffer();
    TransformBuffer(const Duration &duration);
    TransformBuffer(const TransformBuffer &other);
    TransformBuffer &operator=(const TransformBuffer &other);

    size_t size() const;
    bool empty() const;
    void clear();

    void insert(const Time &time, const msg::geometry::Transform &transform);
    bool get(const Time &time, msg::geometry::Transform &transform) const;

    const std::deque<std::pair<Time, msg::geometry::Transform>> &data() const;

    Duration duration() const;
    void set_duration(const Duration &duration);

   private:
    Duration duration_;
    std::deque<std::pair<Time, msg::geometry::Transform>> buffer_;
};

}  // namespace rix