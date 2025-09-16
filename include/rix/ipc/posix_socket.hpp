#include "rix/ipc/generic_socket.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace rix::ipc {

class POSIXSocket : public GenericSocket {
public:
  POSIXSocket();
  ~POSIXSocket() override;

  virtual bool bind(const Endpoint &endpoint) const override;
  virtual bool listen(int backlog) const override;
  virtual std::unique_ptr<GenericSocket>
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