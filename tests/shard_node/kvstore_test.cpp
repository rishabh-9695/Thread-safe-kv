#include "../../shard_node/kvstore.hpp"
#include <gtest/gtest.h>

TEST(KVStoreTest, BasicPutGet) {
    KVStore store;
    store.put("alpha", "42");
    auto result = store.get("alpha");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "42");
}

TEST(KVStoreTest, PutWithTTL) {
    KVStore store;
    store.put("beta", "100", 1000); // 1 second TTL
    auto result = store.get("beta");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "100");

    std::this_thread::sleep_for(std::chrono::milliseconds(1100)); // Wait for TTL to expire
    result = store.get("beta");
    EXPECT_FALSE(result.has_value());
}

TEST(KVStoreTest, RemoveKey){
    KVStore store;
    store.put("gamma", "200");
    auto result = store.get("gamma");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "200");
    store.remove("gamma");
    result = store.get("gamma");
    EXPECT_FALSE(result.has_value());
}

TEST(KVStoreTest, WALRecovery) {
    {
        KVStore store("test_wal.log");
        store.put("foo", "bar");
        store.remove("foo");
    }

    // Simulate restart
    KVStore recovered("test_wal.log");
    auto value = recovered.get("foo");
    EXPECT_FALSE(value.has_value());
}

TEST(KVStoreTest, ConcurrentPut) {
    KVStore store;
    std::thread t1([&]() { store.put("key", "A"); });
    std::thread t2([&]() { store.put("key", "B"); });
    t1.join();
    t2.join();

    // Should not crash, value should be A or B (last writer wins)
    auto value = store.get("key");
    EXPECT_TRUE(value.has_value());
}