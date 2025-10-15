# Simple Service Example

This example demonstrates a basic service node using the rix-cpp framework. It shows how to implement a service that responds to requests from clients.

## Files
- `main.cpp`: Entry point for the service node.
- `simple_service.cpp` / `simple_service.hpp`: Implementation and interface for the service logic.
- `test.cpp`: Unit tests for the service.
- `CMakeLists.txt`: Build configuration for the example.

## Usage
1. Build the example:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```
2. Run the service:
   ```bash
   ./simple_service
   ```

## Features
- Service request handling
- Custom service types

## Requirements
- rix-cpp library
- C++17 or newer

See the main project README for setup instructions.