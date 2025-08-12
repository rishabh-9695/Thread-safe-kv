#pragma once
#include "kvstore.hpp"
#include <vector>
#include <memory>
#include "wal.hpp"

class PartitionedKVStore {
    private:
        size_t partitionCount;
        std::vector<std::unique_ptr<KVStore>> partitions;

        size_t getPartitionIndex(const std::string& key) const {
            return std::hash<std::string>{}(key) % partitionCount;
        }
    public:
        // Constructor with configurable partition count
        PartitionedKVStore(size_t numPartitions = 16) : partitionCount(numPartitions), partitions(numPartitions) {
            for (size_t i = 0; i < partitionCount; ++i) {
                partitions[i] = KVStore::create("WAL_partition_" + std::to_string(i) + ".log");
            }
        }
        
        // Get current partition count
        size_t getPartitionCount() const { return partitionCount; }
        
        void put(const std::string& key, const std::string& value) {
            size_t partitionIndex = getPartitionIndex(key);
            partitions[partitionIndex]->put(key, value);
        }

        void put(const std::string& key, const std::string& value, int ttl_ms) {
            size_t partitionIndex = getPartitionIndex(key);
            partitions[partitionIndex]->put(key, value, ttl_ms);
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