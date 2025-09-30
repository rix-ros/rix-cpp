# RIX: Robotics Interprocess eXchange

## Fast, Modular Interprocess Communication for Robotics

**RIX** is a high-performance C++ framework for real-time interprocess communication in robotics and distributed systems. It delivers a robust messaging infrastructure, node management, and service orchestration—empowering you to build scalable, reliable robot software architectures.

- 🚀 **Fast & Lightweight:** Zero third-party dependencies, optimized for low-latency and high-throughput.
- 🧩 **Modular:** Easily extendable with publishers, subscribers, services, timers, and more.
- 🤖 **Robotics-Ready:** Designed to meet the demands of modern robotics applications.
- 🔒 **Reliable:** TCP-based communication ensures message integrity; loosely-coupled nodes provide system stability across distributed environments.

### Support for Robotics Applications
- 🌳 **Transformation Trees:** Built-in support for 3D spatial transform trees (`rix::tf`), including frame graph management, transform broadcasting/listening, and time-based interpolation.
- 🦾 **Robot Model & Kinematics:** Parse robot descriptions from JSON (JRDF), manage kinematic chains, and perform forward/inverse kinematics with the `rix::rob` module.

The `rix::tf` library depends on [`Eigen`](https://github.com/PX4/eigen) for linear algebra operations and [`nlohmann::json`](https://github.com/nlohmann/json) for JSON parsing.

RIX makes it easy to develop complex robotic systems, offering a clean API and powerful tools for node registration, topic management, service handling, spatial transforms, and robot modeling.

---

## Get Started: Build Scalable, Real-Time Robotic Applications with RIX

### Installation

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
The default RIX install will use a single thread to manage all components of a Node. There are many cases where using multiple threads will result in a significant boost in performance. If you would like to install RIX with multithreading support, run the install script with the `--multithreaded` argument.

```bash
bash install.bash --multitheaded
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

---

## C++ API Tutorial

RIX is organized into Nodes that communicate via message streams (topics) and remote procedural calls (services). Source the setup script and start the `rixhub` server before running your nodes.

```bash
source ~/.rix/setup.bash
rixhub
```

### Publisher Example

Create a publisher that sends `Header` messages at 1 Hz:

```cpp
#include "rix/rix.hpp"
#include "rix/msg/standard/Header.hpp"

using namespace rix::core;
using namespace rix::util;
using rix::msg::standard::Header;

std::shared_ptr<Publisher> publisher;

void timer_callback(const rix::core::Timer::Event event) {
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
  node.spin(std::move(create_signal(SIGINT)));
}
```

### Subscriber Example

Register a subscriber on the same topic:

```cpp
#include "rix/rix.hpp"
#include "rix/msg/standard/Header.hpp"

using namespace rix::core;
using namespace rix::util;
using rix::msg::standard::Header;

void subscriber_callback(const Header &msg) {
  Log::info << msg.frame_id << ", " << msg.seq << std::endl;
}

int main() {
  Node node("subscriber_node");
  node.create_subscriber<Header>("/my_topic", subscriber_callback);
  node.spin(std::move(create_signal(SIGINT)));
}
```

### Service Example

Provide a request-response service:

```cpp
#include "rix/rix.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/msg/standard/Header.hpp"

using namespace rix::core;
using namespace rix::util;
using rix::msg::standard::UInt32;
using rix::msg::standard::Header;

void service_callback(const UInt32 &req, Header &res) {
  Log::info << "Received request!" << std::endl;
  res.frame_id = "Hello from service!";
  res.seq = req.data;
}

int main() {
  Node node("service_node");
  node.create_service<UInt32, Header>("/my_service", service_callback);
  node.spin(std::move(create_signal(SIGINT)));
}
```

### Service Client Example

Call a service from another node:

```cpp
#include "rix/rix.hpp"
#include "rix/msg/standard/UInt32.hpp"
#include "rix/msg/standard/Header.hpp"

using namespace rix::core;
using namespace rix::util;
using rix::msg::standard::UInt32;
using rix::msg::standard::Header;

std::shared_ptr<ServiceClient> service_client;

void timer_callback(const rix::core::Timer::Event event) {
  static int i = 0;
  if (service_client) {
    Header res;
    UInt32 req;
    req.data = i++;
    if (service_client->call(req, res)) {
      Log::info << res.frame_id << ", " << res.seq << std::endl;
    }
  }
}

int main() {
  Node node("service_client_node");
  service_client = node.create_service_client<UInt32, Header>("/my_service");
  node.create_timer(Duration(1.0), timer_callback);
  node.spin(std::move(create_signal(SIGINT)));
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