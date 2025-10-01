#include "rix/tf/frame.hpp"

namespace rix::tf {

Frame::Frame() {}

Frame::Frame(const std::string &name, const TransformBuffer &buffer) : buffer(buffer), name(name) {}

Frame::Frame(const std::string &name, const rix::util::Duration &duration)
    : buffer(duration), name(name) {}

Frame::Frame(const Frame &other) : buffer(other.buffer), name(other.name) {}

Frame &Frame::operator=(const Frame &other) {
    if (this != &other) {
        name = other.name;
        buffer = other.buffer;
    }
    return *this;
}

}  // namespace rix::tf