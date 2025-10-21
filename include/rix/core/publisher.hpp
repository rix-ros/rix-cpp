#pragma once

#include <memory>
#include <mutex>
#include <set>

#include "rix/core/common.hpp"
#include "rix/core/spinner.hpp"
#include "rix/msg/mediator/Operation.hpp"
#include "rix/msg/mediator/PubInfo.hpp"
#include "rix/msg/mediator/Status.hpp"
#include "rix/msg/mediator/SubInfo.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/util/log.hpp"

namespace rix {

class Node; // Forward declaration

class Publisher : public Spinner {
  friend class Node;

public:
  Publisher(const Publisher &) = delete;
  Publisher &operator=(const Publisher &) = delete;
  Publisher(Publisher &&) = delete;
  Publisher &operator=(Publisher &&) = delete;
  ~Publisher();

  void publish(const msg::Message &msg);
  size_t get_subscriber_count() const;

private:
  msg::mediator::PubInfo info_;
  SocketFactory socket_factory_;
  std::shared_ptr<GenericSocket> server_;
  std::set<std::shared_ptr<GenericSocket>> connections_;
  mutable std::mutex connections_mutex_;
  Endpoint rixhub_endpoint_;
  std::atomic<bool> registered_flag_;

#ifdef RIX_MULTITHREADED
  std::thread spin_thread_{};
#endif

  Publisher(const msg::mediator::PubInfo &info, SocketFactory factory, Endpoint rixhub_endpoint);

  using Spinner::spin;
  using Spinner::spin_once;
  void on_spin() override;
};

} // namespace rix