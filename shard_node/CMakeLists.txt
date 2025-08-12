add_library(kvstore STATIC
    kvstore.cpp
    wal.cpp
)

target_include_directories(kvstore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared/threadpool
)

# Find required packages for gRPC
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Add the gRPC server executable target
add_executable(kvstore_server
    server.cpp          # Entry point (starts the gRPC server)
    service.cpp         # Implements KVStoreServiceImpl
    kvstore.pb.cc       # Generated protobuf sources
    kvstore.grpc.pb.cc  # Generated gRPC sources
)

target_include_directories(kvstore_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared/threadpool
)

target_link_libraries(kvstore_server
    PRIVATE
        kvstore           # Link our KV store library
        gRPC::grpc++      # gRPC C++ library
        gRPC::grpc++_reflection  # For reflection support
        protobuf::libprotobuf    # Protobuf library
)

# Optionally keep old CLI-only "shard_node" for non-gRPC use
add_executable(shard_node main.cpp kvstore.cpp wal.cpp)
target_link_libraries(shard_node kvstore)
target_include_directories(shard_node PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared/threadpool
)
