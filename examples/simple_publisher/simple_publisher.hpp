#include "rix/rix.hpp"
#include "rix/std_msgs/Header.hpp"

const std::string NAME = "simple_publisher";

class SimplePublisher final : public rix::Node {
public:
  SimplePublisher(double rate, int port);

private:
  std::shared_ptr<rix::Publisher> pub;
  std::shared_ptr<rix::TimerCallback> timer;
  rix::std_msgs::Header message;

  /**
   * @brief TimerCallback callback that is invoked by the Node at 1.0 Hz during spin
   *
   */
  void timer_callback(const rix::TimerCallback::Event& event);
};