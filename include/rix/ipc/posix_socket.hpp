#include "rix/ipc/generic_socket.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

namespace rix::ipc {

class POSIXSocket : public GenericSocket {
public:
  POSIXSocket();
  POSIXSocket(const POSIXSocket &) = delete;
  POSIXSocket &operator=(const POSIXSocket &) = delete;
  POSIXSocket(POSIXSocket &&) = delete;
  POSIXSocket &operator=(POSIXSocket &&) = delete;
  ~POSIXSocket() override;

  bool bind(const Endpoint &endpoint) const override;
  bool listen(int backlog) const override;
  std::shared_ptr<GenericSocket> accept(Endpoint &remote_endpoint) const override;
  bool connect(const Endpoint &endpoint) const override;
  void close() const override;
  ssize_t send(const void *buf, size_t len, int flags) const override;
  ssize_t recv(void *buf, size_t len, int flags) const override;
  bool wait_readable(const rix::util::Duration &timeout) const override;
  bool wait_writable(const rix::util::Duration &timeout) const override;
  bool wait_exception(const rix::util::Duration &timeout) const override;
  bool set_blocking(bool blocking) const override;
  bool get_blocking() const override;
  bool set_reuse_address(bool reuse) const override;
  bool get_reuse_address() const override;
  Endpoint local_endpoint() const override;
  Endpoint remote_endpoint() const override;
  int get_fd() const override;

private:
  POSIXSocket(int fd);
  int fd_;
};

} // namespace rix::ipc