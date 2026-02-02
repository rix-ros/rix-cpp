# RIX: Robotics Interprocess eXchange

## Fast, Modular Interprocess Communication for Robotics

**RIX** is a high-performance C++ framework for real-time interprocess communication in robotics and distributed systems. It delivers a robust messaging infrastructure, node management, and service orchestration—empowering you to build scalable, reliable robot software architectures.

- 🚀 **Fast & Lightweight:** Zero third-party dependencies, optimized for low-latency and high-throughput.
- 🧩 **Modular:** Easily extendable with publishers, subscribers, services, actions, timers, and more.
- 🤖 **Robotics-Ready:** Designed to meet the demands of modern robotics applications.
- 🔒 **Reliable:** TCP-based communication ensures message integrity; loosely-coupled nodes provide system stability across distributed environments.
- 📝 **Testable:** Test fixture designed to enable users to write readable, straight-forward unit tests for their RIX nodes.

### Support for Robotics Applications
- 🌳 **Transformation Trees:** Built-in support for 3D spatial transform trees (`rix::tf`), including frame graph management, transform broadcasting/listening, and time-based interpolation.
- 🦾 **Robot Model & Kinematics:** Parse robot descriptions from JSON (JRDF), manage kinematic chains, and perform forward/inverse kinematics with the `rix::rob` module.

The `rix::tf` library depends on [`Eigen`](https://github.com/PX4/eigen) for linear algebra operations and [`nlohmann::json`](https://github.com/nlohmann/json) for JSON parsing.

RIX makes it easy to develop complex robotic systems, offering a clean API and powerful tools for node registration, topic management, service handling, spatial transforms, and robot modeling.

---

## Get Started: Build Scalable, Real-Time Robotic Applications with RIX

### Installation

Before installing, ensure that you have `wget` and `cmake` installed. `Python3.12` must be available in your `PATH`.

Run the install script to set up RIX and its dependencies:

```bash
bash install.bash
```

This will:
- Build and install the RIX C++ library and tools to `$HOME/.rix/`
- Build and install the `rixmsg` tool for generating message headers.
- Install the RIX Python API (`rix-py`) for Python users, which installs the `jrdf` and `rixtopic` tools.
- Set up environment scripts and virtual environments

**Note:** RIX is designed for POSIX-compliant systems (Linux, MacOS). Windows support is in progress (use WSL as a workaround).


### Multithreaded Support
By default, RIX will use a single thread to manage all components of a Node. There are many cases where using multiple threads will result in a significant boost in performance. If you would like to use RIX with multithreading support, set the `RIX_MULTITHREADED` environment variable to `1`.

```bash
export RIX_MULTITHREADED=1
```

### Environment Setup

Before running any RIX executable, source the setup script to set environment variables:

```bash
source ~/.rix/setup.bash
```

You can add this to your `.bashrc` (or similar) for convenience:

```bash
echo 'source ~/.rix/setup.bash' >> ~/.bashrc
```

By default, RIX binds servers to the loopback address (`127.0.0.1`). You can configure this and other defaults in `~/.rix/setup.bash` using environment variables:

- `RIX_DEFAULT_IP`
- `RIX_RIXHUB_IP`
- `RIX_RIXHUB_PORT`
- `RIX_MULTITHREADED`

---

## C++ API Tutorial

RIX is organized into Nodes that communicate via message streams (topics) and remote procedural calls (services). Source the setup script and start the `rixhub` server before running your nodes.

```bash
source ~/.rix/setup.bash
rixhub
```

### Timer Example
Create a timer that prints a message at 1 Hz:

```cpp
#include "rix/rix.hpp"
using namespace rix;

int main() {
  Node node("timer_node");
  node.create_timer(Duration(1.0), [&](const TimerCallback::Event event) {
    Log::info << "Timer tick!" << std::endl;
  });
  node.spin();
}
```

### Publisher Example

Create a publisher that sends `Header` messages at 1 Hz:

```cpp
#include "rix/rix.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;
using rix::std_msgs::Header;

std::shared_ptr<Publisher> publisher;

void timer_callback(const rix::TimerCallback::Event event) {
  static int i = 0;
  if (publisher) {
    Header msg;
    msg.frame_id = "Hello, world!";
    msg.seq = i++;
    publisher->publish(msg);
  }
}

int main() {
  Node node("publisher_node");
  publisher = node.create_publisher<Header>("/my_topic");
  node.create_timer(Duration(1.0), timer_callback);
  node.spin();
}
```

### Subscriber Example

Register a subscriber on the same topic:

```cpp
#include "rix/rix.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;
using rix::std_msgs::Header;

void subscriber_callback(const Header &msg) {
  Log::info << msg.frame_id << ", " << msg.seq << std::endl;
}

int main() {
  Node node("subscriber_node");
  node.create_subscriber("/my_topic", subscriber_callback);
  node.spin();
}
```

### Service Example

Provide a request-response service:

```cpp
#include "rix/rix.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;
using rix::std_msgs::UInt32;
using rix::std_msgs::Header;

void service_callback(const UInt32 &req, Header &res) {
  Log::info << "Received request: " << req.data << std::endl;
  res.frame_id = "Hello from service!";
  res.seq = req.data;
}

int main() {
  Node node("service_node");
  node.create_service("/my_service", service_callback);
  node.spin();
}
```

### Service Client Example

Call a service from another node:

```cpp
#include "rix/rix.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;
using rix::std_msgs::UInt32;
using rix::std_msgs::Header;

int main() {
  Node node("service_client_node");
  auto service_client = node.create_service_client<UInt32, Header>("/my_service");
  UInt32 req;
  req.data = 5;
  Header res;
  if (service_client->call(req, res)) {
    Log::info << "Response: " << res.frame_id << ", " << res.seq << std::endl;
  }
  return 0;
}
```

### Action Example

Provide a preemptible task via an action server:

```cpp
#include "rix/rix.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;
using rix::std_msgs::UInt32;
using rix::std_msgs::Header;

bool action_callback(const UInt32 &goal, const Header &feedback, Header &result) {
  static int count = 0;
  Log::info << "Received goal!" << std::endl;
  count++;
  if (count < goal.data) {
    Header fb;
    fb.frame_id = "Action feedback";
    fb.seq = count;
    return false;
  }
  result.frame_id = "Hello from action!";
  result.seq = count;
  count = 0;
  return true;
}

int main() {
  Node node("action_node");
  node.create_action_server("/my_action", action_callback);
  node.spin();
}
```

### Action Client Example

Dispatch an action goal from another node:

```cpp
#include "rix/rix.hpp"
#include "rix/std_msgs/UInt32.hpp"
#include "rix/std_msgs/Header.hpp"

using namespace rix;
using namespace rix::std_msgs;

std::shared_ptr<ActionClient> act_cli;

void timer_callback(const TimerCallback::Event &event) {
  static int i = 0;
  Header result;
  UInt32 goal;
  goal.data = 5; // Number of feedback messages to receive
  act_cli->dispatch(goal);
}

int main() {
  Node node("action_client_node");
  act_cli = node.create_action_client<UInt32, Header, Header>("/my_action");
  act_cli->set_feedback_callback([](const Header &msg) { Log::info << "Feedback: " << msg.seq << std::endl; });
  act_cli->set_result_callback([](const Header &msg) { Log::info << "Result: " << msg.seq << std::endl; });
  node.create_timer(Duration(1.0), timer_callback);
  node.spin();
}
```

---

## Using RIX in Your Project

To include RIX in a CMake project, simply find the package and link to the RIX library.

```cmake
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)

project(my_cmake_project)

find_package(rix REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main rix::rix)
```

To compile your project, first source `~/.rix/setup.bash` and build:

```bash
source ~/.rix/setup.bash
mkdir build
cd build
cmake ..
make
```

---

## Finding Your IP Address

If you do not have a static IP address, you can modify the `~/.rix/setup.bash` file to use your public IP address.

On Linux:
```bash
export RIX_DEFAULT_IP=$(hostname -I | xargs)
```

On MacOS:
```bash
export RIX_DEFAULT_IP=$(ipconfig getifaddr en0)
```

---

## More Information

- [RIX Message Documentation](https://github.com/rix-ros/rix-msg)
- [RIX Python Documentation](https://github.com/rix-ros/rix-py)

---

## License

See [LICENSE.md](LICENSE.md) for details.

---