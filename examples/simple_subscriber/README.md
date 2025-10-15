# Simple Subscriber Example

This example demonstrates a basic subscriber node using the rix-cpp framework. It shows how to subscribe to a topic and process incoming messages.

## Files
- `main.cpp`: Entry point for the subscriber node.
- `simple_subscriber.cpp` / `simple_subscriber.hpp`: Implementation and interface for the subscriber logic.
- `test.cpp`: Unit tests for the subscriber.
- `CMakeLists.txt`: Build configuration for the example.

## Usage
1. Build the example:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```
2. Run the subscriber:
   ```bash
   ./simple_subscriber
   ```

## Features
- Topic subscription
- Message processing

## Requirements
- rix-cpp library
- C++17 or newer

See the main project README for setup instructions.