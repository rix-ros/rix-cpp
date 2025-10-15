#include "rix/msg/standard/Header.hpp"
#include "rix/rix.hpp"
#include "rix/test/test_fixture.hpp"

const std::string NAME = "simple_publisher";

class SimplePublisher : public rix::Node {
public:
  SimplePublisher(double rate, int port);
  
private:
  std::shared_ptr<rix::Publisher> pub;
  std::shared_ptr<rix::TimerCallback> timer;
  rix::msg::standard::Header message;

  /**
   * @brief TimerCallback callback that is invoked by the Node at 1.0 Hz during spin
   *
   */
  void timer_callback(const rix::TimerCallback::Event& event);
};