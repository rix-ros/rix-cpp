cmake_minimum_required(VERSION 3.16)
project(rix)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_TESTS "Build unit tests" OFF)
option(MULTITHREADED "Enable multi-threaded support" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")

# Set RPATH settings for proper dynamic library loading
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

find_package(Threads REQUIRED)

add_library(rix SHARED
    src/rix/core/action_client.cpp
    src/rix/core/action.cpp
    src/rix/core/mediator.cpp
    src/rix/core/node.cpp
    src/rix/core/publisher.cpp
    src/rix/core/service_client.cpp
    src/rix/core/service.cpp
    src/rix/core/subscriber.cpp
    src/rix/core/timer_callback.cpp
    src/rix/core/component_factory.cpp
    src/rix/core/mediator_client.cpp
    src/rix/ipc/endpoint.cpp
    src/rix/ipc/stream.cpp
    src/rix/ipc/tcp_acceptor.cpp
    src/rix/ipc/tcp_stream.cpp
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
target_link_libraries(rix PUBLIC Threads::Threads)


add_library(tf SHARED
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
target_link_libraries(tf PUBLIC rix)
target_include_directories(tf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:$ENV{HOME}/.rix/include>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:$ENV{HOME}/.rix/include>
)

# Build RIXHub
add_executable(rixhub src/rixhub.cpp)
target_link_libraries(rixhub rix)

# Build unit tests
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Building unit tests")

    add_executable(node_test
        tests/node_test.cpp
    )
    target_link_libraries(node_test GTest::gtest_main GTest::gmock rix)

    add_executable(mediator_test
        tests/mediator_test.cpp
    )
    target_link_libraries(mediator_test GTest::gtest_main GTest::gmock rix)

    add_executable(tf_tests
        tests/tf/frame_graph.cpp
        tests/tf/joint.cpp
        tests/tf/robot_model_rx200.cpp
        tests/tf/robot_model_simple_bot.cpp
        tests/tf/kinematics_solver.cpp
        tests/tf/transform_buffer.cpp)
    target_link_libraries(tf_tests GTest::Main tf rix)
endif()

# Install libaries
install(TARGETS rix tf EXPORT rixTargets DESTINATION lib)

# Install rixTargets.cmake 
install(EXPORT rixTargets FILE rixTargets.cmake NAMESPACE rix:: DESTINATION lib/cmake/rix)

# Install rixConfig.cmake
install(FILES rixConfig.cmake DESTINATION lib/cmake/rix)

# Install headers
install(DIRECTORY include/rix DESTINATION include)

# Install RIXHub executable
install(TARGETS rixhub DESTINATION bin)
