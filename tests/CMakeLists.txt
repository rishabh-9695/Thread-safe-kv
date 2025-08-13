find_package(GTest CONFIG REQUIRED)
add_executable(kvstore_test shard_node/kvstore_test.cpp)
target_link_libraries(kvstore_test PRIVATE GTest::gtest_main kvstore)
include(GoogleTest)
gtest_discover_tests(kvstore_test)

# Add WAL test
add_executable(wal_test shard_node/wal_test.cpp)
target_link_libraries(wal_test GTest::gtest_main kvstore)
include(GoogleTest)
gtest_discover_tests(wal_test)

# Add PartitionedKVStore test
add_executable(partitioned_kvstore_test shard_node/partitioned_kvstore_test.cpp)
target_link_libraries(partitioned_kvstore_test GTest::gtest_main kvstore)
include(GoogleTest)
gtest_discover_tests(partitioned_kvstore_test)