#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "rix/util/time.hpp"

namespace rix {

namespace detail {

/**
 * @brief TeeBuffer class. This is used to write data to multiple streams at
 * once. This is used by the Log class to write data to both stdout and a log
 * file at the same time.
 *
 */
class TeeBuffer final : public std::streambuf {
public:
  explicit TeeBuffer(std::vector<std::streambuf*> targets);

  int overflow(int c) override;
  std::streamsize xsputn(const char* s, std::streamsize n) override;
  int sync() override;
  void add(std::streambuf* target);

private:
  std::vector<std::streambuf*> targets_;
};

inline TeeBuffer::TeeBuffer(std::vector<std::streambuf*> targets) : targets_(std::move(targets)) {}
inline int TeeBuffer::overflow(const int c) {
  if (c != EOF) {
    for (auto target : targets_) {
      if (target->sputc(static_cast<char>(c)) == EOF) {
        return EOF;
      }
    }
  }
  return 0;
}
inline int TeeBuffer::sync() {
  int result = 0;
  for (auto* target : targets_) {
    if (target->pubsync() == -1) {
      result = -1;
    }
  }
  return result;
}
inline std::streamsize TeeBuffer::xsputn(const char* s, std::streamsize n) {
  for (auto* target : targets_) {
    if (target->sputn(s, n) != n) {
      return 0;
    }
  }
  return n;
}
inline void TeeBuffer::add(std::streambuf* target) { targets_.push_back(target); }

} // namespace detail

/**
 * @brief A class for logging messages with different log levels.
 */
class Log {
public:
  /**
   * @brief Log level enum. These values are used as template parameters to
   * the Stream class. They will be checked against the value of
   * RIX_UTIL_LOG_LEVEL at compile time to determine if data should be sent
   * to stdout/stderr.
   *
   */
  enum Level { DEBUGV, DEBUG, INFO, WARN, ERROR, FATAL };

private:
  template <rix::Log::Level level> class Line {
  public:
    Line() : active_(false), has_content_(false), buffer_(nullptr) {}
    Line(std::ostream& stream, std::mutex& mutex)
        : active_(true), has_content_(false), buffer_(stream.rdbuf()), stream_(std::in_place), lock_(mutex) {}
    ~Line() {
      if (active_ && has_content_) {
        const std::string& content = stream_->str();
        buffer_->sputn(content.data(), static_cast<std::streamsize>(content.size()));
        buffer_->sputc('\n');
        buffer_->pubsync();
      }
    }
    Line(Line&& other) noexcept
        : active_(other.active_), has_content_(other.has_content_), buffer_(other.buffer_),
          stream_(std::move(other.stream_)), lock_(std::move(other.lock_)) {
      other.active_ = false;
    }
    Line& operator=(Line&& other) noexcept {
      active_ = other.active_;
      has_content_ = other.has_content_;
      buffer_ = other.buffer_;
      stream_ = std::move(other.stream_);
      lock_ = std::move(other.lock_);
      other.active_ = false;
      return *this;
    }
    template <typename T> Line& operator<<(const T& val) {
      if (active_) {
        *stream_ << val;
        has_content_ = true;
      }
      return *this;
    }
    Line& operator<<(std::ostream& (*m)(std::ostream&)) {
      if (active_) {
        m(*stream_);
        has_content_ = true;
      }
      return *this;
    }

  private:
    bool active_;
    bool has_content_;
    std::streambuf* buffer_;
    std::optional<std::ostringstream> stream_;
    std::unique_lock<std::mutex> lock_;
  };

  /**
   * @brief static declaration of NullBuffer used by Stream objects to
   * "write" to when the RIX_UTIL_LOG_LEVEL < level.
   *
   */
  // inline static detail::NullBuffer null_buffer{};
  inline static std::ofstream logFile{};
  inline static detail::TeeBuffer tee_buffer{std::vector<std::streambuf*>{std::cout.rdbuf()}};
  inline static std::mutex mutex{};

  /**
   * @brief Stream class. This class has a << operator that will append
   * header information to the data that is input to the stream. The level
   * template parameter is used to determine if data should be logged at
   * compile time. This minimizes runtime overhead when data should not be
   * logged.
   *
   * @tparam level
   */
  template <Level level> class Stream {
  public:
    template <typename T> Line<level> operator<<(const T& val);

    // inline static std::ostream null_stream{&Log::null_buffer};
    inline static std::ostream tee_stream{&Log::tee_buffer};
    inline static std::mutex& mutex{Log::mutex};

    static const std::string level_prefix;
    inline static std::string create_header(const Time& t);
  };

public:
  inline static void init(const std::string& name);
  inline static void set_log_level(Level level) { level_ = level; }

  /**
   * The public Stream objects. These are used to log information at the
   * corresponding level. If RIX_UTIL_LOG_LEVEL is greater than the template
   * level parameter, then no data will be logged when used.
   *
   */
  inline static Stream<Level::DEBUGV> debugv{};
  inline static Stream<Level::DEBUG> debug{};
  inline static Stream<Level::INFO> info{};
  inline static Stream<Level::WARN> warn{};
  inline static Stream<Level::ERROR> error{};
  inline static Stream<Level::FATAL> fatal{};

private:
  static constexpr const char* reset_color_{"\033[0m"};
  static constexpr const char* bold_{"\x1b[1m"};
  static constexpr const char* unbold_{"\x1b[22m"};
  inline static std::string name_{};
  inline static bool is_init_{false};
  inline static Level level_{Level::INFO};

  static constexpr const char* get_color_code(Level level);
  static constexpr const char* get_level_string(Level level);
};

template <Log::Level level> template <typename T> Log::Line<level> Log::Stream<level>::operator<<(const T& val) {
  if (level < level_) {
    return Log::Line<level>();
  }

  Log::Line<level> l(tee_stream, mutex);
  std::string header = create_header(Time::now());
  l << header << val;
  return std::move(l);
}

template <Log::Level level>
const std::string Log::Stream<level>::level_prefix = [] {
  std::string s = "[";
  s += bold_;
  s += get_color_code(level);
  s += get_level_string(level);
  s += reset_color_;
  s += "] ";
  if (s.size() < 21)
    s.resize(21, ' ');
  return s;
}();

template <Log::Level level> inline std::string Log::Stream<level>::create_header(const Time& t) {
  std::string header;
  header.reserve(96);
  header += '[';
  header += t.to_string();
  header += "] ";
  header += level_prefix;
  if (is_init_) {
    header += '[';
    header += bold_;
    header += name_;
    header += unbold_;
    header += "] ";
  }
  return header;
}

inline void Log::init(const std::string& name) {
  if (is_init_) {
    return;
  }
  name_ = name;
  is_init_ = true;
}

constexpr const char* Log::get_color_code(Level level) {
  switch (level) {
  case Level::DEBUGV:
    return "\033[34m";
  case Level::DEBUG:
    return "\033[36m";
  case Level::INFO:
    return "\033[32m";
  case Level::WARN:
    return "\033[33m";
  case Level::ERROR:
    return "\033[31m";
  case Level::FATAL:
    return "\033[35m";
  default:
    return "\033[0m";
  }
}

constexpr const char* Log::get_level_string(Level level) {
  switch (level) {
  case Level::DEBUGV:
    return "DEBUGV";
  case Level::DEBUG:
    return "DEBUG";
  case Level::INFO:
    return "INFO";
  case Level::WARN:
    return "WARN";
  case Level::ERROR:
    return "ERROR";
  case Level::FATAL:
    return "FATAL";
  default:
    return "";
  }
}

} // namespace rix