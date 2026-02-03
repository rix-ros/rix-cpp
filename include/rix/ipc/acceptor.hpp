#pragma once

#include "rix/ipc/endpoint.hpp"
#include "rix/ipc/poll.hpp"
#include "rix/msg/message.hpp"
#include "rix/sys_msgs/Operation.hpp"
#include "rix/util/time.hpp"

#include <memory>

namespace rix {

class Acceptor : public Pollable {
public:
  // Constructor and Destructor
  Acceptor() = default;
  virtual ~Acceptor() = default;

  // Disable copy and move semantics (force use of shared/unique pointers)
  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;
  Acceptor(Acceptor&&) = delete;
  Acceptor& operator=(Acceptor&&) = delete;

  // Socket state operations
  virtual std::shared_ptr<Stream> accept(Endpoint& remote_endpoint) const = 0;
  inline std::shared_ptr<Stream> accept() const {
    Endpoint ep;
    return accept(ep);
  }

  // Socket control operations
  virtual bool set_blocking(bool blocking) const = 0;
  virtual bool get_blocking() const = 0;

  // Endpoint retrieval
  virtual Endpoint local_endpoint() const = 0;
  virtual Endpoint remote_endpoint() const = 0;
};

} // namespace rix