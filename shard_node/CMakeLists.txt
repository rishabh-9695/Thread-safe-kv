add_library(kvstore STATIC
    kvstore.cpp
    wal.cpp
)

target_include_directories(kvstore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared/threadpool
)

add_executable(shard_node main.cpp
kvstore.cpp
wal.cpp)

target_link_libraries(shard_node kvstore)
target_include_directories(shard_node PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/shared/threadpool
)