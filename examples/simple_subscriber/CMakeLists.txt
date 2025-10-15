cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)

project(simple_subscriber)

find_package(rix REQUIRED)

# Add the test executable
add_executable(simple_subscriber simple_subscriber.cpp main.cpp)
target_link_libraries(simple_subscriber rix::rix)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")
    enable_testing()
    add_executable(simple_subscriber_test
        test.cpp
        simple_subscriber.cpp
    )
    target_link_libraries(simple_subscriber_test GTest::gtest_main GTest::gmock rix::rix)
endif()
