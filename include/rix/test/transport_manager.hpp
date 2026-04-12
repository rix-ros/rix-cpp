#pragma once

#include <memory>
#include <queue>

#include "rix/core/common.hpp"
#include "rix/test/mock_acceptor.hpp"
#include "rix/test/mock_stream.hpp"

namespace rix {

/**
 * @brief Manages creation and queueing of MockAcceptor and MockStream objects
 *        for test injection via the TransportFactory mechanism.
 */
class TransportManager {
public:
  TransportManager()
      : acceptors_(std::make_shared<std::queue<std::shared_ptr<MockAcceptor>>>()),
        streams_(std::make_shared<std::queue<std::shared_ptr<MockStream>>>()) {
    factory_.create_acceptor = [acceptors = acceptors_](const Endpoint&) -> std::shared_ptr<Acceptor> {
      if (acceptors->empty()) {
        return nullptr;
      }
      auto a = acceptors->front();
      acceptors->pop();
      return a;
    };
    factory_.create_stream = [streams = streams_](const Endpoint&, bool) -> std::shared_ptr<Stream> {
      if (streams->empty()) {
        return nullptr;
      }
      auto s = streams->front();
      streams->pop();
      return s;
    };
  }

  TransportManager(const TransportManager&) = default;
  TransportManager& operator=(const TransportManager&) = default;
  TransportManager(TransportManager&&) noexcept = default;
  TransportManager& operator=(TransportManager&&) noexcept = default;

  /**
   * @brief Get the TransportFactory that draws from the mock queues.
   */
  TransportFactory& get_factory() { return factory_; }

  /**
   * @brief Create a new MockAcceptor and add it to the acceptor queue.
   */
  std::shared_ptr<MockAcceptor> create_acceptor() {
    auto a = std::make_shared<MockAcceptor>();
    acceptors_->push(a);
    return a;
  }

  /**
   * @brief Create a new MockStream and add it to the stream queue.
   */
  std::shared_ptr<MockStream> create_stream() {
    auto s = std::make_shared<MockStream>();
    streams_->push(s);
    return s;
  }

  /**
   * @brief Add a preconfigured MockAcceptor to the queue.
   */
  void add_acceptor(const std::shared_ptr<MockAcceptor>& a) { acceptors_->push(a); }

  /**
   * @brief Add a preconfigured MockStream to the queue.
   */
  void add_stream(const std::shared_ptr<MockStream>& s) { streams_->push(s); }

  void reset() {
    while (!acceptors_->empty())
      acceptors_->pop();
    while (!streams_->empty())
      streams_->pop();
  }

private:
  std::shared_ptr<std::queue<std::shared_ptr<MockAcceptor>>> acceptors_;
  std::shared_ptr<std::queue<std::shared_ptr<MockStream>>> streams_;
  TransportFactory factory_;
};

} // namespace rix
