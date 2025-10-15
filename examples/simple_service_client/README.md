# Simple Service Client Example

This example demonstrates a basic service client node using the rix-cpp framework. It shows how to send requests to a service and process the responses.

## Files
- `main.cpp`: Entry point for the service client node.
- `simple_service_client.cpp` / `simple_service_client.hpp`: Implementation and interface for the client logic.
- `test.cpp`: Unit tests for the client.
- `CMakeLists.txt`: Build configuration for the example.

## Usage
1. Build the example:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```
2. Run the service client:
   ```bash
   ./simple_service_client
   ```

## Features
- Service request sending
- Response handling

## Requirements
- rix-cpp library
- C++17 or newer

See the main project README for setup instructions.