#pragma once

#include "rix/core/node.hpp"
#include "rix/core/publisher.hpp"
#include "rix/core/timer_callback.hpp"
#include "rix/std_msgs/Header.hpp"
#include "rix/util/log.hpp"
#include "rix/util/time.hpp"

const std::string NAME = "simple_publisher";

/**
 * @brief Example publisher node, templated on TNode so it can be swapped with
 *        MockNode in unit tests.
 *
 * Production use:  SimplePublisher<>          (defaults to rix::Node)
 * Test use:        SimplePublisher<MockNode>
 */
class SimplePublisher final : public rix::Node {
public:
  /**
   * @param rate   Publish rate in Hz.
   * @param port   Bind port for the publisher socket (0 = ephemeral, unused
   *               when TNode is MockNode).
   */
  SimplePublisher(double rate, int port = 0);

private:
  std::shared_ptr<rix::Publisher> pub_;
  std::shared_ptr<rix::TimerCallback> timer_;
  rix::std_msgs::Header message_;

  void timer_callback_(const rix::TimerCallback::Event& /*event*/);
};