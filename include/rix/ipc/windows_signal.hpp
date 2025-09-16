#pragma once

#include <functional>

#include <windows.h>

#include "rix/ipc/generic_signal.hpp"

namespace rix::ipc {

class WindowsSignal : public GenericSignal {
public:
  WindowsSignal(int signum);
  virtual ~WindowsSignal();

  virtual bool ignore() const override;
  virtual bool raise() const override;
  virtual bool wait(const rix::util::Duration &d) const override;
};

} // namespace rix::ipc