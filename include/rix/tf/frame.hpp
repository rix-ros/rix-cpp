#pragma once

#include <string>
#include <vector>

#include "rix/rob/msg_util.hpp"
#include "rix/tf/transform_buffer.hpp"

namespace rix {

struct Frame {
    Frame();
    Frame(const std::string &name, const TransformBuffer &buffer);
    Frame(const std::string &name, const Duration &duration);
    Frame(const Frame &other);
    Frame &operator=(const Frame &other);

    TransformBuffer buffer;
    std::string name;
};

}  // namespace rix