# Simple Publisher Example

This example demonstrates a basic publisher node using the rix-cpp framework. It shows how to publish messages to a topic at a regular interval.

## Files
- `main.cpp`: Entry point for the publisher node.
- `simple_publisher.cpp` / `simple_publisher.hpp`: Implementation and interface for the publisher logic.
- `test.cpp`: Unit tests for the publisher.
- `CMakeLists.txt`: Build configuration for the example.

## Usage
1. Build the example:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```
2. Run the publisher:
   ```bash
   ./simple_publisher
   ```

## Features
- Periodic message publishing
- Custom message types

## Requirements
- rix-cpp library
- C++17 or newer

See the main project README for setup instructions.