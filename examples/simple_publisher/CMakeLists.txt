cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(simple_publisher)

find_package(rix REQUIRED)

# Add the test executable
add_executable(simple_publisher simple_publisher.cpp main.cpp)
target_link_libraries(simple_publisher rix::rix)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")
    enable_testing()
    add_executable(simple_publisher_test
        test.cpp
        simple_publisher.cpp
    )
    target_link_libraries(simple_publisher_test GTest::gtest_main GTest::gmock rix::rix)
endif()
