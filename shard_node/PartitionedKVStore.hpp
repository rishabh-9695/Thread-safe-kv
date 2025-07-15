#pragma once
#include "kvstore.hpp"
#include <vector>
#include <memory>
#include "wal.hpp"

class PartitionedKVStore {
    private:
        static constexpr size_t PARTITION_COUNT = 16; // Example partition count
        std::vector<std::unique_ptr<KVStore>> partitions;

        size_t getPartitionIndex(const std::string& key) const {
            return std::hash<std::string>{}(key) % PARTITION_COUNT;
        }
    public:
        PartitionedKVStore() : partitions(PARTITION_COUNT){
            for (size_t i = 0; i < PARTITION_COUNT; ++i) {
                partitions[i] = std::make_unique<KVStore>("WAL_partition_" + std::to_string(i) + ".log");
            }
        }
        
        void put(const std::string& key, const std::string& value) {
            size_t partitionIndex = getPartitionIndex(key);
            partitions[partitionIndex]->put(key, value);
        }

        std::optional<std::string> get(const std::string& key) {
            return partitions[getPartitionIndex(key)]->get(key);
        }

        void remove(const std::string& key) {
            partitions[getPartitionIndex(key)]->remove(key);
        }

        void shutdown() {
            // For future use, if needed
        }
};