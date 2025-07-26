cmake_minimum_required(VERSION 3.14)
project(ThreadSafeKV)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -pthread")

add_subdirectory(shard_node)

#enable testing
enable_testing()

# Download and add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)

# Add client_cli executable
add_executable(client_cli client_cli/main.cpp)

# Add router_service executable
add_executable(router_service router_service/main.cpp)



include_directories(../shared)