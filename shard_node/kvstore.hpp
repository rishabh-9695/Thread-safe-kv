#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <memory>
#include <chrono>

class WriteAheadLog;

class KVStore {
private:
    struct Value {
        std::string value;
        std::optional<std::chrono::steady_clock::time_point> expiration;
        Value();
        Value(const std::string& val);
        Value(const std::string& val, std::chrono::steady_clock::time_point exp);
        bool isExpired() const;
    };

    std::unordered_map<std::string, Value> store;
    mutable std::shared_mutex mutex;
    std::unique_ptr<WriteAheadLog> wal;
    std::string snapshotFileName;

    void recoverFromWAL(const std::string& filename);
    void snapshot(const std::string& filename);
    void loadSnapshot(const std::string& filename);
    void cleanup_expired_keys();

public:
    KVStore();
    ~KVStore();
    KVStore(const std::string& logFile);

    void put(const std::string& key, const std::string& value);
    void put(const std::string& key, const std::string& value, int ttl_ms);
    std::optional<std::string> get(const std::string& key);
    void remove(const std::string& key);
    void shutdown();
};
