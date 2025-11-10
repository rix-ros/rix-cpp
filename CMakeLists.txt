cmake_minimum_required(VERSION 3.16)
project(rix)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTS "Build unit tests" OFF)
option(MULTITHREADED "Enable multi-threaded support" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")

add_library(rix
    src/rix/core/action_client.cpp
    src/rix/core/action.cpp
    src/rix/core/mediator.cpp
    src/rix/core/node.cpp
    src/rix/core/publisher.cpp
    src/rix/core/service_client.cpp
    src/rix/core/service.cpp
    src/rix/core/subscriber.cpp
    src/rix/core/timer_callback.cpp
    src/rix/ipc/endpoint.cpp
    src/rix/ipc/posix_socket.cpp
    src/rix/ipc/posix_signal.cpp
    src/rix/ipc/poll.cpp
    src/rix/util/argument_parser.cpp
    src/rix/util/time.cpp
)
target_include_directories(rix PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:$ENV{HOME}/.rix/include>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:$ENV{HOME}/.rix/include>
)
if(MULTITHREADED)
    add_compile_definitions(RIX_MULTITHREADED)
    find_package(Threads REQUIRED)
    target_link_libraries(rix PUBLIC Threads::Threads)
endif()

add_library(tf
    src/rix/rob/joint.cpp
    src/rix/rob/link.cpp
    src/rix/rob/robot_model.cpp
    src/rix/rob/eigen_util.cpp
    src/rix/rob/kinematics_solver.cpp
    src/rix/tf/frame_graph.cpp
    src/rix/tf/frame.cpp
    src/rix/tf/transform_broadcaster.cpp
    src/rix/tf/transform_buffer.cpp
    src/rix/tf/transform_listener.cpp
)
target_include_directories(tf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:$ENV{HOME}/.rix/include>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:$ENV{HOME}/.rix/include>
)

# Build unit tests
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")
    enable_testing()
    add_executable(node_test
        test/node_test.cpp
    )
    target_link_libraries(node_test GTest::gtest_main GTest::gmock rix)
    add_executable(mediator_test
        test/mediator_test.cpp
    )
    target_link_libraries(mediator_test GTest::gtest_main GTest::gmock rix)
    add_executable(message_test
        test/message_test.cpp
    )
    target_link_libraries(message_test GTest::gtest_main GTest::gmock rix)

    add_executable(tf_tests
        test/tf/frame_graph.cpp
        test/tf/joint.cpp
        test/tf/robot_model_rx200.cpp
        test/tf/robot_model_simple_bot.cpp
        test/tf/kinematics_solver.cpp
        test/tf/transform_buffer.cpp)
    target_link_libraries(tf_tests GTest::Main tf rix)
endif()

# Configure rixConfig.cmake
if(MULTITHREADED)
    set(THREADS_FIND_PACKAGE "find_package(Threads REQUIRED)")
else()
    set(THREADS_FIND_PACKAGE "")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/rixConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/rixConfig.cmake
    @ONLY
)

# Install library and headers
install(TARGETS rix DESTINATION lib)
install(TARGETS tf DESTINATION lib)
install(DIRECTORY include/rix DESTINATION include)

install(TARGETS rix tf EXPORT rixTargets DESTINATION lib)
install(EXPORT rixTargets FILE rixTargets.cmake NAMESPACE rix:: DESTINATION lib/cmake/rix)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rixConfig.cmake" DESTINATION lib/cmake/rix)

# Build RIXHub
add_executable(rixhub src/rixhub.cpp)
target_link_libraries(rixhub rix)
install(TARGETS rixhub DESTINATION bin)
