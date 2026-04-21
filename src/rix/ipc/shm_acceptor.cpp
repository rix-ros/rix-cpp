#include "rix/ipc/shm_acceptor.hpp"

#include <atomic>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

#include "rix/ipc/shm_stream.hpp"

namespace rix {

// When the caller specifies port 0, assign a unique virtual port via a
// process-wide counter so that each ShmAcceptor gets a distinct socket path.
static uint16_t unique_shm_port() {
    static std::atomic<uint16_t> counter{49000};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

ShmAcceptor::ShmAcceptor(const Endpoint& endpoint, int backlog, size_t shm_buffer_size)
    : shm_buffer_size_(shm_buffer_size), posix_fd_(::socket(AF_UNIX, SOCK_STREAM, 0)) {
    uint16_t port = endpoint.port != 0 ? endpoint.port : unique_shm_port();
    shm_path_ = "/tmp/rix_shm_" + std::to_string(port);

    if (posix_fd_ < 0) {
        std::cout << "Failed to create socket: " << strerror(errno) << std::endl;
        return;
    }

    // Remove stale socket file only if no live acceptor is using it.
    // Try connecting: if it succeeds, the path is in active use - leave it.
    // If it fails, it's a leftover from a crashed process - safe to unlink.
    {
        int probe = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (probe >= 0) {
            struct sockaddr_un probe_addr{};
            probe_addr.sun_family = AF_UNIX;
            strncpy(probe_addr.sun_path, shm_path_.c_str(), sizeof(probe_addr.sun_path) - 1);
            if (::connect(probe, (struct sockaddr*)&probe_addr, sizeof(probe_addr)) < 0) {
                ::unlink(shm_path_.c_str());  // stale - safe to remove
            }
            ::close(probe);
        }
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, shm_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(posix_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "Failed to bind socket: " << strerror(errno) << std::endl;
        ::close(posix_fd_);
        posix_fd_ = -1;
        return;
    }

    if (::listen(posix_fd_, backlog) < 0) {
        std::cout << "Failed to listen on socket: " << strerror(errno) << std::endl;
        ::unlink(shm_path_.c_str());
        ::close(posix_fd_);
        posix_fd_ = -1;
        return;
    }

    // Only set local_endpoint_ on success — port 0 indicates bind/listen failure
    local_endpoint_ = Endpoint(endpoint.address, port);
}

ShmAcceptor::~ShmAcceptor() {
    if (posix_fd_ >= 0) {
        ::close(posix_fd_);
    }
    ::unlink(shm_path_.c_str());
}

std::shared_ptr<Stream> ShmAcceptor::accept(Endpoint& remote_endpoint) const {
    std::lock_guard<std::mutex> lock(accept_mutex_);

    struct sockaddr_un addr{};
    socklen_t len = sizeof(addr);
    int sock_fd = ::accept(posix_fd_, (struct sockaddr*)&addr, &len);
    if (sock_fd < 0) {
        return nullptr;
    }

    // Create a unique shared memory name for this connection
    std::string shm_name = "/rix_shm_conn_" + std::to_string(sock_fd);

    int shm_fd = shm_open(shm_name.c_str(), O_RDWR | O_CREAT, 0666);
    if (shm_fd < 0) {
        std::cout << "Failed to create shared memory: " << strerror(errno) << std::endl;
        ::close(sock_fd);
        return nullptr;
    }

    if (ftruncate(shm_fd, shm_buffer_size_) < 0) {
        std::cout << "Failed to set shared memory size: " << strerror(errno) << std::endl;
        ::close(shm_fd);
        ::close(sock_fd);
        shm_unlink(shm_name.c_str());
        return nullptr;
    }

    void* shm_addr = mmap(nullptr, shm_buffer_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    ::close(shm_fd);  // fd no longer needed after mmap

    if (shm_addr == MAP_FAILED) {
        std::cout << "Failed to map shared memory: " << strerror(errno) << std::endl;
        ::close(sock_fd);
        shm_unlink(shm_name.c_str());
        return nullptr;
    }

    // Send the shm name to the client so it can open the same region
    uint32_t name_len = static_cast<uint32_t>(shm_name.size());
    if (::send(sock_fd, &name_len, sizeof(name_len), 0) < 0 ||
        ::send(sock_fd, shm_name.c_str(), name_len, 0) < 0) {
        std::cout << "Failed to send shm name to client: " << strerror(errno) << std::endl;
        munmap(shm_addr, shm_buffer_size_);
        ::close(sock_fd);
        shm_unlink(shm_name.c_str());
        return nullptr;
    }

    remote_endpoint = local_endpoint_;  // Unix sockets have no meaningful remote address
    return std::shared_ptr<ShmStream>(new ShmStream(sock_fd, shm_addr, shm_buffer_size_, shm_name, true,
                                                    local_endpoint_, local_endpoint_));
}

bool ShmAcceptor::set_blocking(bool blocking) const {
    int flags = fcntl(posix_fd_, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(posix_fd_, F_SETFL, flags) == 0;
}

bool ShmAcceptor::get_blocking() const {
    int flags = fcntl(posix_fd_, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return (flags & O_NONBLOCK) == 0;
}

Endpoint ShmAcceptor::local_endpoint() const {
    return local_endpoint_;
}

Endpoint ShmAcceptor::remote_endpoint() const {
    return {};  // Unix domain sockets have no meaningful remote endpoint
}

bool ShmAcceptor::wait_readable(const Duration& timeout) const {
    struct pollfd pfd{};
    pfd.fd = posix_fd_;
    pfd.events = POLLIN;
    int timeout_ms = static_cast<int>(timeout.to_milliseconds());
    int ret = ::poll(&pfd, 1, timeout_ms);
    return ret > 0 && (pfd.revents & POLLIN);
}

bool ShmAcceptor::wait_writable(const Duration& timeout) const {
    struct pollfd pfd{};
    pfd.fd = posix_fd_;
    pfd.events = POLLOUT;
    int timeout_ms = static_cast<int>(timeout.to_milliseconds());
    int ret = ::poll(&pfd, 1, timeout_ms);
    return ret > 0 && (pfd.revents & POLLOUT);
}

bool ShmAcceptor::wait_exception(const Duration& timeout) const {
    struct pollfd pfd{};
    pfd.fd = posix_fd_;
    pfd.events = 0;
    int timeout_ms = static_cast<int>(timeout.to_milliseconds());
    int ret = ::poll(&pfd, 1, timeout_ms);
    return ret > 0 && (pfd.revents & POLLHUP || pfd.revents & POLLERR || pfd.revents & POLLNVAL);
}

} // namespace rix
