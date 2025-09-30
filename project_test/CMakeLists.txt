cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)

project(rix_test)

find_package(rix REQUIRED)

# Add the test executable
add_executable(rix_test main.cpp)
target_link_libraries(rix_test rix::rix rix::tf)
