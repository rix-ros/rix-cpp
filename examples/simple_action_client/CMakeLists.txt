cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(simple_action_client)

find_package(rix REQUIRED)

# Add the test executable
add_executable(simple_action_client simple_action_client.cpp main.cpp)
target_link_libraries(simple_action_client rix::rix)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")
    enable_testing()
    add_executable(simple_action_client_test
        test.cpp
        simple_action_client.cpp
    )
    target_link_libraries(simple_action_client_test GTest::gtest_main GTest::gmock rix::rix)
endif()
