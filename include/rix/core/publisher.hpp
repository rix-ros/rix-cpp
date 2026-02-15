#pragma once

#include <memory>
#include <mutex>
#include <set>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/sys_msgs/PubInfo.hpp"

namespace rix {

class Node; // Forward declaration

class Publisher final : public Spinner {
  friend class Node;

public:
  // Disable copy and move semantics
  Publisher(const Publisher&) = delete;
  Publisher& operator=(const Publisher&) = delete;
  Publisher(Publisher&&) = delete;
  Publisher& operator=(Publisher&&) = delete;

  /**
   * @brief Destructor. Deregisters the publisher from rixhub.
   */
  ~Publisher() override;

  /**
   * @brief Publishes a message to all connected subscribers.
   * @param msg The message to publish.
   */
  void publish(const Message& msg);

  /**
   * @brief Returns the number of connected subscribers.
   * @return The number of connected subscribers.
   */
  size_t get_subscriber_count() const;

private:
  sys_msgs::PubInfo info_;                        ///< The publisher information.
  TransportFactory socket_factory_;               ///< The socket factory function.
  std::shared_ptr<Acceptor> server_;              ///< The server socket for incoming connections.
  std::set<std::shared_ptr<Stream>> connections_; ///< The set of connected subscriber sockets.
  mutable std::mutex connections_mutex_;          ///< Mutex for protecting the connections set.
  Endpoint rixhub_endpoint_;                      ///< The RIXHub endpoint.
  std::atomic<bool> registered_flag_;             ///< Flag indicating if the publisher is registered.
  std::thread spin_thread_{};                     ///< The thread running the spin loop.

  /**
   * @brief Constructs a Publisher with the given PubInfo, socket factory, and RIXHub endpoint.
   * @param info The PubInfo message containing publisher details.
   * @param factory The socket factory to create sockets.
   * @param rixhub_endpoint The RIXHub endpoint.
   */
  Publisher(const sys_msgs::PubInfo& info, TransportFactory factory, Endpoint rixhub_endpoint);

  // Disable public spin methods (only Node can spin the Publisher)
  using Spinner::spin;
  using Spinner::spin_once;

  /**
   * @brief Internal spin implementation for the Publisher.
   */
  void on_spin() override;
};

} // namespace rix