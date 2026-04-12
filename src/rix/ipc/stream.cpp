#include "rix/ipc/stream.hpp"

namespace rix {

// Write operation and message
bool Stream::send_message(uint8_t opcode, const Message& msg) const {
  // Get the message prefix
  const size_t prefix_len = msg.get_prefix_len();
  auto prefix_buffer = std::make_unique<uint8_t[]>(prefix_len);
  size_t offset = 0;
  msg.get_prefix(prefix_buffer.get(), offset);

  // Serialize the message
  sys_msgs::Operation operation;
  operation.len = msg.get_prefix_len();
  operation.opcode = opcode;
  const int segment_count = operation.get_segment_count() + msg.get_segment_count() + 1;
  std::vector<ConstMessageSegment> segments(segment_count);
  offset = 0;
  operation.get_segments(segments.data(), segments.size(), offset);
  segments[offset++] = ConstMessageSegment(prefix_buffer.get(), prefix_len);
  msg.get_segments(segments.data(), segments.size(), offset);

  // Send the serialized message
  return send_all(segments.data(), segments.size());
}

// Read message only
bool Stream::recv_message(Message& msg, size_t prefix_len) const {
  ssize_t bytes = 0;
  if (prefix_len > 0) {
    // Read the prefix first
    auto prefix_buffer = std::make_unique<uint8_t[]>(prefix_len);
    bytes = recv(prefix_buffer.get(), prefix_len, 0);

    // Resize the message
    size_t offset = 0;
    if (!msg.resize(prefix_buffer.get(), bytes, offset)) {
      return false;
    }
  }

  // Read the segments
  std::vector<MessageSegment> segments(msg.get_segments());
  return recv_all(segments.data(), segments.size());
}

// Read both operation and message (useful if message type is known)
bool Stream::recv_message(sys_msgs::Operation& operation, Message& msg) const {
  // Read the operation header first
  if (!recv_message(operation, operation.get_prefix_len())) {
    return false;
  }
  // Then read the message body
  if (!recv_message(msg, operation.len)) {
    return false;
  }
  return true;
}

void Stream::ignore_message(size_t len) const {
  // Read and discard 'len' bytes
  std::vector<uint8_t> buffer(len);
  size_t bytes = 0;
  while (bytes < buffer.size()) {
    ssize_t result = recv(buffer.data() + bytes, buffer.size() - bytes, 0);
    if (result <= 0) {
      return;
    }
    bytes += result;
  }
}

bool Stream::send_all(const ConstMessageSegment* segments, size_t segment_count) const {
  ssize_t bytes_sent = -1;
  size_t total_len = 0;
  for (size_t i = 0; i < segment_count; ++i) {
    total_len += segments[i].len();
  }

  // If writev returns false, it is not implemented
  if (!writev(segments, segment_count, bytes_sent)) {
    auto buffer = std::make_unique<uint8_t[]>(total_len);
    size_t bytes_copied = 0;
    for (size_t i = 0; i < segment_count; ++i) {
      size_t len = segments[i].len();
      memcpy(buffer.get() + bytes_copied, segments[i].ptr(), len);
      bytes_copied += len;
    }
    bytes_sent = send(buffer.get(), total_len, 0);
  }

  return bytes_sent == total_len;
}

bool Stream::recv_all(MessageSegment* segments, size_t segment_count) const {
  ssize_t bytes_read = -1;
  size_t total_len = 0;
  for (size_t i = 0; i < segment_count; ++i) {
    total_len += segments[i].len();
  }

  // If readv returns false, it is not implemented
  if (!readv(segments, segment_count, bytes_read)) {
    auto buffer = std::make_unique<uint8_t[]>(total_len);
    bytes_read = recv(buffer.get(), total_len, 0);
    if (bytes_read == total_len) {
      size_t bytes_copied = 0;
      for (size_t i = 0; i < segment_count; ++i) {
        size_t len = segments[i].len();
        memcpy(segments[i].ptr(), buffer.get() + bytes_copied, len);
        bytes_copied += len;
      }
    }
  }

  return bytes_read == total_len;
}

} // namespace rix