cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(simple_action)

find_package(rix REQUIRED)

# Add the test executable
add_executable(simple_action simple_action.cpp main.cpp)
target_link_libraries(simple_action rix::rix)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")
    enable_testing()
    add_executable(simple_action_test
        test.cpp
        simple_action.cpp
    )
    target_link_libraries(simple_action_test GTest::gtest_main GTest::gmock rix::rix)
endif()
