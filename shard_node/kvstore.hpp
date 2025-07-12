#pragma once 
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <mutex>

class KVStore {
    private:
        std::unordered_map<std::string, std::string> store; 
        mutable std::shared_mutex mutex; // Mutex for thread-safe access
    public:
        KVStore() = default;
        ~KVStore() = default;

        void put(const std::string& key, const std::string& value) {
            std::unique_lock lock(mutex); // Lock for writing
            store[std::move(key)] = std::move(value);
        }

        std::string get(const std::string& key) {
            std::shared_lock lock(mutex); // Lock for reading
            auto it = store.find(key);
            return it != store.end() ? it->second : ""; // Return empty string if key not found
        }

        void remove(const std::string& key) {
            std::unique_lock lock(mutex); // Lock for writing
            store.erase(key);
        }
};