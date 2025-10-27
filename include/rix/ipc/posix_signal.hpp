#pragma once

#include "rix/ipc/generic_signal.hpp"
#include <functional>

namespace rix {

class POSIXSignal final : public GenericSignal {
public:
  explicit POSIXSignal(int signum);
  POSIXSignal(const POSIXSignal&) = delete;
  POSIXSignal& operator=(const POSIXSignal&) = delete;
  POSIXSignal(POSIXSignal&&) = delete;
  POSIXSignal& operator=(POSIXSignal&&) = delete;
  ~POSIXSignal() override;

  bool ignore() const override;
  bool raise() const override;
  bool wait(const Duration& d) const override;

private:
  struct Notifier {
    Notifier() {}
    std::array<int, 2> pipe{}; /**< 0: read end, 1: write end */
    bool is_init{false};       /**< false if Notifier has not been initialized */
  };
  static constexpr int MAX_SIGNALS{32};
  static inline std::array<Notifier, MAX_SIGNALS> notifier{};
  static void handler(int signum);

  int signum_;
};

} // namespace rix