# Parameter Server Example

This example demonstrates a simple parameter server implementation using the rix-cpp framework. It shows how to create a node that manages parameters, allowing other nodes to get and set values at runtime.

## Files
- `main.cpp`: Entry point for the parameter server node.
- `CMakeLists.txt`: Build configuration for the example.

## Usage
1. Build the example:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```
2. Run the parameter server:
   ```bash
   ./parameter_server
   ```

## Features
- Parameter management
- Runtime get/set operations

## Requirements
- rix-cpp library
- C++17 or newer

See the main project README for setup instructions.