cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(parameter_server)

find_package(rix REQUIRED)

# Add the test executable
add_executable(parameter_server main.cpp)
target_link_libraries(parameter_server rix::rix)
