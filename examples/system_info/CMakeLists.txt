cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(system_info)

find_package(rix REQUIRED)

# Add the test executable
add_executable(system_info main.cpp)
target_link_libraries(system_info rix::rix)
