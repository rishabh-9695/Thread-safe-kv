#include <iostream>
#include <cassert>
#include "kvstore.hpp"

int main() {
    std::cout << "Testing KVStore with Write Ahead Log..." << std::endl;
    {
        auto kvstore = KVStore::create("WAL.log"); // Create an instance of KVStore
        kvstore->put("key1", "value1");
        kvstore->put("key2", "value2");
        kvstore->remove("key1");
        std::this_thread::sleep_for(std::chrono::seconds(10)); // Allow some time for background threads to process
    }
    // Simulate crash and recovery
    {
        auto kvstore = KVStore::create("WAL.log"); // Create a new instance with WAL file
        auto value = kvstore->get("key1");
        assert(!value.has_value()); // key1 should be removed
        value = kvstore->get("key2");
        assert(value.has_value() && value.value() == "value2"); // key2 should still exist
    }
    return 0;
}