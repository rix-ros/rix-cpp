#pragma once

#include "rix/ipc/generic_socket.hpp"

#include <cstring>
#include <memory>

#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

namespace rix::ipc {

class WindowsSocket : public GenericSocket {
public:
  WindowsSocket();
  ~WindowsSocket() override;

  virtual bool bind(const Endpoint &endpoint) const override;
  virtual bool listen(int backlog) const override;
  virtual std::shared_ptr<GenericSocket>
  accept(Endpoint &remote_endpoint) const override;
  virtual bool connect(const Endpoint &endpoint) const override;
  virtual void close() const override;
  virtual ssize_t send(const void *buf, size_t len, int flags) const override;
  virtual ssize_t recv(void *buf, size_t len, int flags) const override;
  virtual ssize_t send_to(const void *buf, size_t len, const Endpoint &endpoint,
                          int flags) const override;
  virtual ssize_t recv_from(void *buf, size_t len, Endpoint &endpoint,
                            int flags) const override;
  virtual bool wait_readable(const rix::util::Duration &timeout) const override;
  virtual bool wait_writable(const rix::util::Duration &timeout) const override;
  virtual bool
  wait_exception(const rix::util::Duration &timeout) const override;
  virtual bool set_blocking(bool blocking) const override;
  virtual bool get_blocking() const override;
  virtual bool set_reuse_address(bool reuse) const override;
  virtual bool get_reuse_address() const override;
  virtual bool set_reuse_port(bool reuse) const override;
  virtual bool get_reuse_port() const override;
  virtual bool set_recv_buffer_size(int size) const override;
  virtual int get_recv_buffer_size() const override;
  virtual bool set_send_buffer_size(int size) const override;
  virtual int get_send_buffer_size() const override;
  virtual bool
  join_multicast_group(const std::string &multicast_address) const override;
  virtual bool
  leave_multicast_group(const std::string &multicast_address) const override;
  virtual Endpoint local_endpoint() const override;
  virtual Endpoint remote_endpoint() const override;

private:
  POSIXSocket(int fd);
  int fd_;
};

} // namespace rix::ipc