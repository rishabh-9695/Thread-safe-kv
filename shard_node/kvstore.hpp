#pragma once 
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <mutex>
#include <memory>
#include <fstream>
#include "wal.hpp"

class KVStore {
    private:
        std::unordered_map<std::string, std::string> store; 
        mutable std::shared_mutex mutex; // Mutex for thread-safe access
        std::unique_ptr<WriteAheadLog> wal; // Optional: for Write Ahead Log integration
        void recoverFromWAL(const std::string& logFileName) {
            std::ifstream walFile(logFileName);
            if(!walFile.is_open()) {
                throw std::runtime_error("Failed to open WAL file: " + logFileName);
            }
            std::string op, key, value;
            while (walFile >> op >> key) {
                if (op == "PUT") {
                    walFile >> value;
                    store[key] = value; // Recover PUT operation
                } else if (op == "REMOVE") {
                    store.erase(key); // Recover REMOVE operation
                }
            }
        }
    public:
        KVStore() = default;
        ~KVStore() = default;
        KVStore(const std::string& logFile) : wal(std::make_unique<WriteAheadLog>(logFile)) {
            recoverFromWAL(logFile); // Recover state from WAL on initialization
        }
        void put(const std::string& key, const std::string& value) {
            std::unique_lock lock(mutex); // Lock for writing
            store[std::move(key)] = std::move(value);
            wal->append("PUT " + key + " " + value); // Log the operation
        }

        std::optional<std::string> get(const std::string& key) {
            std::shared_lock lock(mutex); // Lock for reading
            auto it = store.find(key);
            if(it != store.end())
                return it->second;
            return std::nullopt;
        }

        void remove(const std::string& key) {
            std::unique_lock lock(mutex); // Lock for writing
            wal->append("REMOVE " + key); // Log the operation
            store.erase(key);
        }
};