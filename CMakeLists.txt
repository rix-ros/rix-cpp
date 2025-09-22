cmake_minimum_required(VERSION 3.16)
project(rix)
set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_CLANG_TIDY clang-tidy 
#   -checks=cppcoreguidelines-*
# )

if(WIN32)
  set(SOCKET_SRC src/rix/ipc/windows_socket.cpp)
  set(SIGNAL_SRC src/rix/ipc/windows_signal.cpp)
elseif(UNIX)
  set(SOCKET_SRC src/rix/ipc/posix_socket.cpp)
  set(SIGNAL_SRC src/rix/ipc/posix_signal.cpp)
else()
  message(FATAL_ERROR "Unsupported OS")
endif()

find_package(GTest REQUIRED)

add_library(rix
  src/rix/core/mediator.cpp
  src/rix/core/node.cpp
  src/rix/core/publisher.cpp
  src/rix/core/service_client.cpp
  src/rix/core/service.cpp
  src/rix/core/subscriber.cpp
  src/rix/core/timer.cpp
  src/rix/ipc/endpoint.cpp
  ${SOCKET_SRC}
  ${SIGNAL_SRC}
  src/rix/util/argument_parser.cpp
  src/rix/util/time.cpp
)
target_include_directories(rix PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:$ENV{HOME}/.rix/include>
  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:$ENV{HOME}/.rix/include>
)

# Build examples
add_executable(parameter_server examples/parameter_server.cpp)
target_link_libraries(parameter_server rix)

add_executable(simple_publisher examples/simple_publisher.cpp)
target_link_libraries(simple_publisher rix)

add_executable(simple_subscriber examples/simple_subscriber.cpp)
target_link_libraries(simple_subscriber rix)

add_executable(simple_service examples/simple_service.cpp)
target_link_libraries(simple_service rix)

add_executable(simple_service_client examples/simple_service_client.cpp)
target_link_libraries(simple_service_client rix)

# Build unit tests
enable_testing()
add_executable(node_registration_tests
  unit_tests/node_registration_tests.cpp
)
target_link_libraries(node_registration_tests GTest::gtest_main GTest::gmock rix)

add_executable(mediator_registration_tests
  unit_tests/mediator_registration_tests.cpp
)
target_link_libraries(mediator_registration_tests GTest::gtest_main GTest::gmock rix)

# Install library and headers
install(TARGETS rix DESTINATION lib)
install(DIRECTORY include/rix DESTINATION include)

install(TARGETS rix EXPORT rixTargets DESTINATION lib)
install(EXPORT rixTargets FILE rixTargets.cmake NAMESPACE rix:: DESTINATION lib/cmake/rix)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/rixConfig.cmake" DESTINATION lib/cmake/rix)

# Build RixHub
add_executable(rixhub src/rixhub.cpp)
target_link_libraries(rixhub rix)
install(TARGETS rixhub DESTINATION bin)

# Build RixInfo
add_executable(rixinfo src/rixinfo.cpp)
target_link_libraries(rixinfo rix)
install(TARGETS rixinfo DESTINATION bin)
