#include "../../shard_node/kvstore.hpp"
#include <gtest/gtest.h>

TEST(KVStoreTest, BasicPutGet) {
    auto store = KVStore::create("test_wal.log");
    store->put("alpha", "42");
    auto result = store->get("alpha");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "42");
}

TEST(KVStoreTest, PutWithTTL) {
    auto store = KVStore::create("test_wal.log");
    store->put("beta", "100", 1000); // 1 second TTL
    auto result = store->get("beta");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "100");

    std::this_thread::sleep_for(std::chrono::milliseconds(1100)); // Wait for TTL to expire
    result = store->get("beta");
    EXPECT_FALSE(result.has_value());
}

TEST(KVStoreTest, RemoveKey){
    auto store = KVStore::create("test_wal.log");
    store->put("gamma", "200");
    auto result = store->get("gamma");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "200");
    store->remove("gamma");
    result = store->get("gamma");
    EXPECT_FALSE(result.has_value());
}

TEST(KVStoreTest, WALRecovery) {
    {
        auto store = KVStore::create("test_wal.log");
        store->put("foo", "bar");
        store->remove("foo");
    }

    // Simulate restart
    auto store = KVStore::create("test_wal.log");
    auto value = store->get("foo");
    EXPECT_FALSE(value.has_value());
}

TEST(KVStoreTest, ConcurrentPut) {
    auto store = KVStore::create("test_wal.log");
    std::thread t1([&]() { store->put("key", "A"); });
    std::thread t2([&]() { store->put("key", "B"); });
    t1.join();
    t2.join();

    // Should not crash, value should be A or B (last writer wins)
    auto value = store->get("key");
    EXPECT_TRUE(value.has_value());
}