#pragma once

#include <memory>
#include <mutex>
#include <set>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/PubInfo.hpp"

namespace rix {

class Publisher : public Spinner {
public:
  virtual ~Publisher() = default;
  virtual void publish(const Message& msg) = 0;
  virtual size_t get_subscriber_count() const = 0;
};

// TODO: Move PublisherImpl to a separate source file and hide it from the public interface
namespace detail {

class PublisherImpl final : public Publisher {
public:
  /**
   * @brief Constructs a PublisherImpl with the given PubInfo, socket factory, and RIXHub endpoint.
   * @param info The PubInfo message containing publisher details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  PublisherImpl(const sys_msgs::PubInfo& info, Endpoint rixhub_endpoint);

  // Disable copy and move semantics
  PublisherImpl(const PublisherImpl&) = delete;
  PublisherImpl& operator=(const PublisherImpl&) = delete;
  PublisherImpl(PublisherImpl&&) = delete;
  PublisherImpl& operator=(PublisherImpl&&) = delete;

  /**
   * @brief Destructor. Deregisters the publisher from rixhub.
   */
  ~PublisherImpl() override;

  /**
   * @brief Publishes a message to all connected subscribers.
   * @param msg The message to publish.
   */
  void publish(const Message& msg) override;

  /**
   * @brief Returns the number of connected subscribers.
   * @return The number of connected subscribers.
   */
  size_t get_subscriber_count() const override;

private:
  sys_msgs::PubInfo info_;                        ///< The publisher information.
  TransportFactory factory_;                      ///< The socket factory function.
  TransportFactory tcp_factory_;                  ///< TCP factory captured at construction for rixhub comms.
  std::shared_ptr<Acceptor> server_;              ///< The server socket for incoming connections.
  std::set<std::shared_ptr<Stream>> connections_; ///< The set of connected subscriber sockets.
  mutable std::mutex connections_mutex_;          ///< Mutex for protecting the connections set.
  Endpoint rixhub_endpoint_;                      ///< The RIXHub endpoint.
  std::atomic<bool> registered_flag_;             ///< Flag indicating if the publisher is registered.
  std::thread spin_thread_{};                     ///< The thread running the spin loop.

  // Disable public spin methods (only Node can spin the PublisherImpl)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the PublisherImpl.
   */
  void on_spin() override;
};

} // namespace detail
} // namespace rix