#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <condition_variable>

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

    std::thread cleaner;
    std::thread snapshotThread;
    std::atomic<bool> stopFlag = false;

    std::unordered_map<std::string, Value> store;
    mutable std::shared_mutex mutex;
    mutable std::mutex snapshotMutex;
    mutable std::mutex cleanerMutex;
    std::condition_variable snapshotCV;
    std::condition_variable cleanerCV;
    std::unique_ptr<WriteAheadLog> wal;
    std::string snapshotFileName;

    size_t snapshotIntervalSeconds = 2;

    void recoverFromWAL(const std::string& filename);
    void snapshot(const std::string& filename);
    void loadSnapshot(const std::string& filename);
    void cleanup_expired_keys();    
    void startBackgroundThreads();
    KVStore(const std::string& logFile);
public:
    KVStore();
    ~KVStore();
    static std::unique_ptr<KVStore> create(const std::string& logFile);
    void put(const std::string& key, const std::string& value);
    void put(const std::string& key, const std::string& value, int ttl_ms);
    std::optional<std::string> get(const std::string& key);
    void remove(const std::string& key);
    void shutdown();
};
