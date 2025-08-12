# Find required packages for gRPC
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Benchmark executables
add_executable(kvstore_benchmark 
    kvstore_benchmark.cpp
    ../shard_node/kvstore.pb.cc
    ../shard_node/kvstore.grpc.pb.cc
)

add_executable(partition_benchmark 
    partition_benchmark.cpp
    ../shard_node/kvstore.pb.cc
    ../shard_node/kvstore.grpc.pb.cc
)

# Include directories
target_include_directories(kvstore_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shard_node
)

target_include_directories(partition_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shard_node
)

# Link libraries
target_link_libraries(kvstore_benchmark
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf    # Protobuf library
)

target_link_libraries(partition_benchmark
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf    # Protobuf library
)
