cmake_minimum_required(VERSION 3.10)
project(ThreadSafeKV)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -pthread")

# Add client_cli executable
add_executable(client_cli client_cli/main.cpp)

# Add router_service executable
add_executable(router_service router_service/main.cpp)

# Add shard_node executable
add_executable(shard_node shard_node/main.cpp)

include_directories(..\shared)