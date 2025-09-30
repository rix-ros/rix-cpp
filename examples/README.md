# rix-cpp Examples

This directory contains example programs demonstrating the usage of the `rix-cpp` library. Each example showcases a different feature or pattern for interacting with the library.

## Example List

- **parameter_server.cpp**  
  Demonstrates how to use a parameter server for storing and retrieving configuration parameters.

- **simple_publisher.cpp**  
  Shows how to publish messages to a topic.

- **simple_subscriber.cpp**  
  Shows how to subscribe to a topic and receive messages.

- **simple_service.cpp**  
  Provides an example of implementing a service server.

- **simple_service_client.cpp**  
  Demonstrates how to call a service as a client.

- **system_info.cpp**  
  Retrieves and displays RIX runtime system information.

## How to Build

From the root of the repository, run:

```sh
mkdir build
cd build
cmake ..
make
```

## How to Run

After building, you can run any example using:

```sh
./examples/<example_name>
```

For example:

```sh
./examples/simple_publisher
```

## Requirements

- C++17 or newer
- CMake 3.10+

## License

See [LICENSE.md](LICENSE.md) for details.