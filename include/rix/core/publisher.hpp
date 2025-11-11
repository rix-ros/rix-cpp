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
  Publisher(const Publisher&) = delete;
  Publisher& operator=(const Publisher&) = delete;
  Publisher(Publisher&&) = delete;
  Publisher& operator=(Publisher&&) = delete;
  ~Publisher() override;

  void publish(const Message& msg);
  size_t get_subscriber_count() const;

private:
  sys_msgs::PubInfo info_;
  SocketFactory socket_factory_;
  std::shared_ptr<GenericSocket> server_;
  std::set<std::shared_ptr<GenericSocket>> connections_;
  mutable std::mutex connections_mutex_;
  Endpoint rixhub_endpoint_;
  std::atomic<bool> registered_flag_;
  std::thread spin_thread_{};

  Publisher(const sys_msgs::PubInfo& info, SocketFactory factory, Endpoint rixhub_endpoint);

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;
};

} // namespace rix