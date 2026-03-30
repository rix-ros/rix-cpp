cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)

project(parameter_server)

find_package(rix REQUIRED)

# Add the test executable
add_executable(parameter_server main.cpp parameter_server.cpp)
target_link_libraries(parameter_server rix::rix)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")
    enable_testing()
    add_executable(parameter_server_test
        test.cpp
        parameter_server.cpp
    )
    target_link_libraries(parameter_server_test GTest::gtest_main GTest::gmock rix::rix)
endif()