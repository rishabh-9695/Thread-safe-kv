#pragma once 
#include <string>
#include <unordered_map>

class KVStore {
    private:
        std::unordered_map<std::string, std::string> store; 
    public:
        KVStore() = default;
        ~KVStore() = default;

        void put(const std::string& key, const std::string& value) {
            store[key] = value;
        }

        std::string get(const std::string& key) {
            auto it = store.find(key);
            return it != store.end() ? it->second : ""; // Return empty string if key not found
        }

        void remove(const std::string& key) {
            store.erase(key);
        }
};