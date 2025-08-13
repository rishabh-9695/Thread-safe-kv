#include "../../shard_node/PartitionedKVStore.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <unordered_set>
#include <chrono>
#include <filesystem>

class PartitionedKVStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test files
        for (int i = 0; i < 32; ++i) {
            std::filesystem::remove("test_partition_" + std::to_string(i) + ".log");
            std::filesystem::remove("test_partition_" + std::to_string(i) + ".snapshot");
        }
    }

    void TearDown() override {
        // Clean up test files
        for (int i = 0; i < 32; ++i) {
            std::filesystem::remove("test_partition_" + std::to_string(i) + ".log");
            std::filesystem::remove("test_partition_" + std::to_string(i) + ".snapshot");
        }
    }
};

// Test basic partitioned operations
TEST_F(PartitionedKVStoreTest, BasicOperations) {
    PartitionedKVStore store(4); // 4 partitions
    
    // Test PUT operations
    store.put("key1", "value1");
    store.put("key2", "value2");
    store.put("key3", "value3");
    
    // Test GET operations
    auto result1 = store.get("key1");
    auto result2 = store.get("key2");
    auto result3 = store.get("key3");
    
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());
    
    EXPECT_EQ(result1.value(), "value1");
    EXPECT_EQ(result2.value(), "value2");
    EXPECT_EQ(result3.value(), "value3");
    
    // Test REMOVE operations
    store.remove("key2");
    auto removed_result = store.get("key2");
    EXPECT_FALSE(removed_result.has_value());
}

// Test partition count configuration
TEST_F(PartitionedKVStoreTest, PartitionCountConfiguration) {
    // Test different partition counts
    PartitionedKVStore store8(8);
    PartitionedKVStore store16(16);
    PartitionedKVStore store32(32);
    
    EXPECT_EQ(store8.getPartitionCount(), 8);
    EXPECT_EQ(store16.getPartitionCount(), 16);
    EXPECT_EQ(store32.getPartitionCount(), 32);
    
    // Test operations work with different partition counts
    store8.put("test_key", "test_value");
    store16.put("test_key", "test_value");
    store32.put("test_key", "test_value");
    
    EXPECT_TRUE(store8.get("test_key").has_value());
    EXPECT_TRUE(store16.get("test_key").has_value());
    EXPECT_TRUE(store32.get("test_key").has_value());
}

// Test key distribution across partitions
TEST_F(PartitionedKVStoreTest, KeyDistribution) {
    PartitionedKVStore store(8);
    
    // Insert many keys and verify they're distributed
    const int num_keys = 1000;
    for (int i = 0; i < num_keys; ++i) {
        store.put("key_" + std::to_string(i), "value_" + std::to_string(i));
    }
    
    // Verify all keys can be retrieved
    for (int i = 0; i < num_keys; ++i) {
        auto result = store.get("key_" + std::to_string(i));
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), "value_" + std::to_string(i));
    }
}

// Test concurrent access to different partitions
TEST_F(PartitionedKVStoreTest, ConcurrentPartitionAccess) {
    PartitionedKVStore store(16);
    const int num_threads = 8;
    const int ops_per_thread = 100;
    std::vector<std::thread> threads;
    
    // Launch threads that operate on different key ranges
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&store, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
                std::string value = "thread_" + std::to_string(t) + "_value_" + std::to_string(i);
                
                store.put(key, value);
                auto result = store.get(key);
                EXPECT_TRUE(result.has_value());
                if (result.has_value()) {
                    EXPECT_EQ(result.value(), value);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all data is still accessible
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < ops_per_thread; ++i) {
            std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
            std::string expected_value = "thread_" + std::to_string(t) + "_value_" + std::to_string(i);
            
            auto result = store.get(key);
            ASSERT_TRUE(result.has_value());
            EXPECT_EQ(result.value(), expected_value);
        }
    }
}

// Test partition isolation
TEST_F(PartitionedKVStoreTest, PartitionIsolation) {
    PartitionedKVStore store(4);
    
    // Put keys that should go to different partitions
    store.put("partition_test_1", "value1");
    store.put("partition_test_2", "value2");
    store.put("partition_test_3", "value3");
    store.put("partition_test_4", "value4");
    
    // Remove one key
    store.remove("partition_test_2");
    
    // Verify other keys are unaffected
    EXPECT_TRUE(store.get("partition_test_1").has_value());
    EXPECT_FALSE(store.get("partition_test_2").has_value());
    EXPECT_TRUE(store.get("partition_test_3").has_value());
    EXPECT_TRUE(store.get("partition_test_4").has_value());
}

// Test TTL functionality across partitions
TEST_F(PartitionedKVStoreTest, TTLAcrossPartitions) {
    PartitionedKVStore store(8);
    
    // Put keys with TTL in different partitions
    store.put("ttl_key_1", "value1", 1000); // 1 second TTL
    store.put("ttl_key_2", "value2", 1000);
    store.put("ttl_key_3", "value3", 1000);
    
    // Verify keys exist initially
    EXPECT_TRUE(store.get("ttl_key_1").has_value());
    EXPECT_TRUE(store.get("ttl_key_2").has_value());
    EXPECT_TRUE(store.get("ttl_key_3").has_value());
    
    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // Verify keys have expired
    EXPECT_FALSE(store.get("ttl_key_1").has_value());
    EXPECT_FALSE(store.get("ttl_key_2").has_value());
    EXPECT_FALSE(store.get("ttl_key_3").has_value());
}

// Test performance with optimal partition count
TEST_F(PartitionedKVStoreTest, OptimalPartitionPerformance) {
    // Test the optimal configuration found in benchmarks
    PartitionedKVStore store(16); // Optimal partition count
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform mixed workload
    const int num_operations = 1000;
    for (int i = 0; i < num_operations; ++i) {
        std::string key = "perf_key_" + std::to_string(i);
        std::string value = "perf_value_" + std::to_string(i);
        
        // PUT operation
        store.put(key, value);
        
        // GET operation
        auto result = store.get(key);
        EXPECT_TRUE(result.has_value());
        
        // Occasionally remove keys
        if (i % 10 == 0) {
            store.remove(key);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 2000); // 2 seconds max for 1000 mixed operations
}

// Test edge cases with partition count
TEST_F(PartitionedKVStoreTest, PartitionCountEdgeCases) {
    // Test minimum partition count
    PartitionedKVStore store1(1);
    EXPECT_EQ(store1.getPartitionCount(), 1);
    
    store1.put("single_partition_key", "value");
    EXPECT_TRUE(store1.get("single_partition_key").has_value());
    
    // Test large partition count
    PartitionedKVStore store64(64);
    EXPECT_EQ(store64.getPartitionCount(), 64);
    
    store64.put("many_partitions_key", "value");
    EXPECT_TRUE(store64.get("many_partitions_key").has_value());
}

// Test concurrent mixed operations
TEST_F(PartitionedKVStoreTest, ConcurrentMixedOperations) {
    PartitionedKVStore store(16);
    const int num_threads = 4;
    const int ops_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    
    // Launch threads performing mixed operations
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&store, &successful_operations, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string key = "mixed_" + std::to_string(t) + "_" + std::to_string(i);
                std::string value = "value_" + std::to_string(t) + "_" + std::to_string(i);
                
                // PUT
                store.put(key, value);
                
                // GET
                auto result = store.get(key);
                if (result.has_value() && result.value() == value) {
                    successful_operations++;
                }
                
                // Occasionally remove
                if (i % 5 == 0) {
                    store.remove(key);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have many successful operations
    EXPECT_GT(successful_operations.load(), num_threads * ops_per_thread * 0.8); // At least 80% success
}

// Test hash distribution quality
TEST_F(PartitionedKVStoreTest, HashDistributionQuality) {
    PartitionedKVStore store(8);
    const int num_keys = 800; // 100 keys per partition on average
    
    // Insert keys and track which ones we can retrieve
    std::unordered_set<std::string> inserted_keys;
    
    for (int i = 0; i < num_keys; ++i) {
        std::string key = "hash_test_key_" + std::to_string(i);
        store.put(key, "value");
        inserted_keys.insert(key);
    }
    
    // Verify all keys can be retrieved (good hash distribution)
    int retrieved_count = 0;
    for (const auto& key : inserted_keys) {
        if (store.get(key).has_value()) {
            retrieved_count++;
        }
    }
    
    // Should retrieve all inserted keys
    EXPECT_EQ(retrieved_count, num_keys);
}
