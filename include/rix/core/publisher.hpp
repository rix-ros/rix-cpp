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

namespace rix::core {

class Node; // Forward declaration

class Publisher : public Spinner {
  friend class Node;

public:
  Publisher(const Publisher &) = delete;
  Publisher &operator=(const Publisher &) = delete;
  Publisher(Publisher &&) = delete;
  Publisher &operator=(Publisher &&) = delete;
  ~Publisher();

  bool ok() const override;
  void shutdown() override;
  void publish(const rix::msg::Message &msg);
  size_t get_subscriber_count() const;

private:
  rix::msg::mediator::PubInfo info_;
  SocketFactory socket_factory_;
  std::shared_ptr<rix::ipc::GenericSocket> server_;
  std::set<std::shared_ptr<rix::ipc::GenericSocket>> connections_;
  mutable std::mutex connections_mutex_;
  rix::ipc::Endpoint rixhub_endpoint_;
  std::atomic<bool> shutdown_flag_;
  std::atomic<bool> registered_flag_;

  Publisher(const rix::msg::mediator::PubInfo &info, SocketFactory factory, rix::ipc::Endpoint rixhub_endpoint);

  using Spinner::spin;
  void spin_once() override;
};

} // namespace rix::core